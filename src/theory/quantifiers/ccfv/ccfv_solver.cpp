/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Implementation of Congruence Closure with Free Variables (CCFV) solver for
 * E-ground (dis)unification.
 */

#include "theory/quantifiers/ccfv/ccfv_solver.h"

#include <cstdlib>

#include "base/exception.h"
#include "base/output.h"
#include "expr/node_algorithm.h"
#include "prop/cnf_stream.h"
#include "prop/minisat/minisat.h"
#include "prop/registrar.h"
#include "prop/sat_solver.h"
#include "prop/sat_solver_factory.h"
#include "prop/sat_solver_types.h"
#include "theory/quantifiers/ccfv/ccfv_utils.h"
#include "theory/uf/equality_engine.h"
#include "theory/uf/equality_engine_iterator.h"
#include "theory/uf/theory_uf_rewriter.h"
#include "util/statistics_registry.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

void debugPrintEE(eq::EqualityEngine* ee)
{
  std::cout << "\n=================== EQUALITY ENGINE ==================="
            << std::endl;
  std::vector<Node> reps;
  for (eq::EqClassesIterator eqcs_i(ee); !eqcs_i.isFinished(); ++eqcs_i)
  {
    reps.push_back(*eqcs_i);
  }

  for (size_t i = 0; i < reps.size(); ++i)
  {
    Node rep = reps[i];
    std::cout << "[" << i << "] Rep: " << rep << " : " << rep.getType()
              << std::endl;

    std::cout << "     Members: [";
    bool first = true;
    for (eq::EqClassIterator eqc_i(rep, ee); !eqc_i.isFinished(); ++eqc_i)
    {
      if (!first) std::cout << ", ";
      std::cout << *eqc_i;
      first = false;
    }
    std::cout << "]" << std::endl;

    std::cout << "     Disequal to: [";
    first = true;
    for (size_t j = 0; j < reps.size(); ++j)
    {
      if (i != j && ee->areDisequal(rep, reps[j], false))
      {
        if (!first) std::cout << ", ";
        std::cout << reps[j];
        first = false;
      }
    }
    std::cout << "]" << std::endl;
  }
  std::cout << "=======================================================\n"
            << std::endl;
}

CcfvSolver::CcfvSolver(Env& env) : EnvObj(env) {}

bool CcfvSolver::solve(options::CcfvMode mode,
                       const std::vector<Node>& freeVars,
                       const std::vector<Node>& lits,
                       eq::EqualityEngine* ee,
                       std::vector<std::vector<Node>>& substitutions)
{
  Trace("ccfv-solver") << "CcfvSolver::solve: " << freeVars.size()
                       << " free vars, " << lits.size() << " literals in L"
                       << std::endl;
  if (TraceIsOn("ccfv-solver-debug"))
  {
    Trace("ccfv-solver-debug") << "  Vars: " << freeVars << std::endl;
    Trace("ccfv-solver-debug") << "  L: " << lits << std::endl;
  }

  if (mode == options::CcfvMode::DECISION)
  {
    Trace("ccfv-solver") << "  Dispatching to: Decision Procedure backend"
                         << std::endl;
    return solveProcedural(freeVars, lits, ee, substitutions);
  }
  else if (mode == options::CcfvMode::SAT)
  {
    Trace("ccfv-solver") << "  Dispatching to: SAT Encoding backend"
                         << std::endl;
    return solveSat(freeVars, lits, ee, substitutions);
  }
  return false;
}

std::vector<std::vector<Node>> CcfvSolver::solveProceduralRec(
    const std::vector<Node>& freeVars,
    const std::vector<Node> lits,
    eq::EqualityEngine* ee,
    const ccfv::CcfvIndices& indices,
    std::vector<Node> currentSubstitution)
{
  if (lits.empty())
  {
    return {currentSubstitution};
  }

  // TODO: apply more sophisticated selection later
  // Currently its just a stack
  Node c = lits.back();
  std::vector<Node> newLits(lits.begin(), lits.end() - 1);
  std::vector<std::vector<Node>> sols;

  Trace("ccfv") << "  CcfvEngine: L " << lits << std::endl;
  Trace("ccfv") << "  CcfvEngine: Processing " << c << std::endl;
  Trace("ccfv") << "  CcfvEngine: Remaining L " << newLits << std::endl;

  // Handling predicate literals
  NodeManager* nm = nodeManager();
  if (c.getKind() == Kind::APPLY_UF)
  {
    c = c.eqNode(nm->mkConst(true));
  }
  if (c.getKind() == Kind::NOT && c[0].getKind() == Kind::APPLY_UF)
  {
    c = c[0].eqNode(nm->mkConst(false));
  }

  switch (c.getKind())
  {
    case Kind::EQUAL:
    {
      TNode l = c[0], r = c[1];

      // Syntactic equality
      // s = s
      if (l == r)
      {
        Trace("ccfv") << "    CcfvEngine: syntactic equality (" << l << " = "
                      << r << ")" << std::endl;
        Trace("ccfv") << "    CcfvEngine: removing from C" << std::endl;
        return solveProceduralRec(
            freeVars, newLits, ee, indices, currentSubstitution);
      }

      // Ground equality
      // t1 = t2
      bool lIsGround = !expr::hasFreeVar(l), rIsGround = !expr::hasFreeVar(r);
      if (lIsGround && rIsGround)
      {
        Trace("ccfv") << "    CcfvEngine: ground equality (" << l << " = " << r
                      << ")" << std::endl;
        if (ee->hasTerm(l) && ee->hasTerm(r))
        {
          if (ee->areEqual(l, r))
          {
            Trace("ccfv") << "      CcfvEngine: they are qual, removing from C"
                          << std::endl;
            return solveProceduralRec(
                freeVars, newLits, ee, indices, currentSubstitution);
          }
          return {};
        }
        else if (l.hasOperator() && r.hasOperator()
                 && l.getOperator() == r.getOperator()
                 && l.getNumChildren() == r.getNumChildren())
        {
          Trace("ccfv")
              << "      CcfvEngine: they are UF with the same operator"
              << std::endl;
          for (size_t i = 0, n = l.getNumChildren(); i < n; ++i)
          {
            Trace("ccfv") << "      CcfvEngine: adding " << l[i].eqNode(r[i])
                          << "to C" << std::endl;
            newLits.push_back(l[i].eqNode(r[i]));
          }
          return solveProceduralRec(
              freeVars, newLits, ee, indices, currentSubstitution);
        }
      }

      // Assignmnet
      // x = s or s = x
      if (l.getKind() == Kind::BOUND_VARIABLE
          || r.getKind() == Kind::BOUND_VARIABLE)
      {
        TNode variable = (l.getKind() == Kind::BOUND_VARIABLE ? l : r);
        TNode term = (l.getKind() == Kind::BOUND_VARIABLE ? r : l);

        Trace("ccfv") << "    CcfvEngine: assignment (" << variable << " = "
                      << term << ")" << std::endl;

        if (!expr::hasSubterm(term, variable))
        {
          Trace("ccfv") << "      CcfvEngine: adding " << variable << " = "
                        << term << " to the solution" << std::endl;
          for (Node& t : currentSubstitution)
          {
            t = t.substitute(TNode(variable), TNode(term));
          }
          Trace("ccfv") << "      CcfvEngine: applying (" << variable
                        << " mapsto " << term << ") to terms that have "
                        << variable << " in " << newLits << std::endl;
          for (Node& lit : newLits)
          {
            lit = lit.substitute(TNode(variable), TNode(term));
          }
          return solveProceduralRec(
              freeVars, newLits, ee, indices, currentSubstitution);
        }
        else
        {
          if (term.hasOperator())
          {
            Node op = term.getOperator();
            Trace("ccfv") << "      CcfvEngine: Uvar (" << variable
                          << " occurs in " << term << " with op = " << op << ")"
                          << std::endl;
            for (const Node& t : indices.getReps(op))
            {
              Trace("ccfv")
                  << "      CcfvEngine: adding " << term.eqNode(t) << " and "
                  << variable.eqNode(t) << " to C" << std::endl;
              std::vector<Node> branchLits = newLits;
              branchLits.push_back(term.eqNode(t));
              branchLits.push_back(variable.eqNode(t));
              std::vector<std::vector<Node>> branchSols = solveProceduralRec(
                  freeVars, branchLits, ee, indices, currentSubstitution);
              sols.insert(sols.end(), branchSols.begin(), branchSols.end());
            }
          }
        }

        return sols;
      }

      // E-matching
      // f(u_n) = t or t = f(u_n)
      if ((!ee->hasTerm(l) && l.hasOperator() && rIsGround && ee->hasTerm(r))
          || (!ee->hasTerm(r) && r.hasOperator() && lIsGround
              && ee->hasTerm(l)))
      {
        TNode fTerm = ee->hasTerm(r) ? l : r;
        TNode t = ee->hasTerm(r) ? r : l;
        TNode op = fTerm.getOperator();

        Trace("ccfv") << "    CcfvEngine: E-Matching UF with ground term ("
                      << fTerm << " = " << t << ")" << std::endl;

        Trace("ccfv") << "      CcfvEngine: E-Matching all UF terms with op = "
                      << op << " in the equivalence class of " << t
                      << std::endl;
        for (const Node& app : indices.getApps(op, t))
        {
          Assert(fTerm.getNumChildren() == app.getNumChildren());

          Trace("ccfv") << "      CcfvEngine: E-Matching " << app << " with "
                        << t << " and adding it to C" << std::endl;
          std::vector<Node> branchLits = newLits;
          for (size_t i = 0, n = fTerm.getNumChildren(); i < n; ++i)
          {
            Trace("ccfv") << "      CcfvEngine: adding "
                          << fTerm[i].eqNode(app[i]) << " to C" << std::endl;
            branchLits.push_back(fTerm[i].eqNode(app[i]));
          }
          std::vector<std::vector<Node>> branchSols = solveProceduralRec(
              freeVars, branchLits, ee, indices, currentSubstitution);
          sols.insert(sols.end(), branchSols.begin(), branchSols.end());
        }

        return sols;
      }

      // E-matching
      // f(u_n) = g(u'_n) or g(u'_n) = f(u_n)
      if (l.hasOperator() && r.hasOperator())
      {
        Trace("ccfv") << "    CcfvEngine: E-Matching with UF (" << l << " = "
                      << r << ")" << std::endl;

        if (l.getOperator() == r.getOperator())
        {
          Assert(l.getNumChildren() == r.getNumChildren());
          Trace("ccfv") << "      CcfvEngine: both UF has the same operator"
                        << std::endl;

          std::vector<Node> branchLits = newLits;
          for (size_t i = 0, n = l.getNumChildren(); i < n; ++i)
          {
            Trace("ccfv") << "      CcfvEngine: unifying arguments: "
                          << l[i].eqNode(r[i]) << std::endl;
            branchLits.push_back(l[i].eqNode(r[i]));
          }
          std::vector<std::vector<Node>> branchSols = solveProceduralRec(
              freeVars, branchLits, ee, indices, currentSubstitution);
          sols.insert(sols.end(), branchSols.begin(), branchSols.end());
        }

        for (const Node& t : indices.getReps(l.getOperator()))
        {
          std::vector<Node> branchLits = newLits;
          Trace("ccfv") << "      CcfvEngine: E-Matching them with ground "
                           "terms within the same "
                           "equivalence class "
                        << l.eqNode(t) << " and " << r.eqNode(t) << std::endl;
          branchLits.push_back(l.eqNode(t));
          branchLits.push_back(r.eqNode(t));
          std::vector<std::vector<Node>> branchSols = solveProceduralRec(
              freeVars, branchLits, ee, indices, currentSubstitution);
          sols.insert(sols.end(), branchSols.begin(), branchSols.end());
        }

        return sols;
      }

      break;
    }
    case Kind::NOT:
    {
      if (c[0].getKind() != Kind::EQUAL) return {};

      Node eq = c[0];
      Node l = eq[0], r = eq[1];

      if (l == r)
      {
        Trace("ccfv") << "    CcfvEngine: syntactic disequality (" << l
                      << " != " << r << "), failing branch" << std::endl;
        return {};
      }

      // Ground disequality
      // t1 != t2
      bool lIsGround = !expr::hasFreeVar(l), rIsGround = !expr::hasFreeVar(r);
      if (lIsGround && rIsGround)
      {
        Trace("ccfv") << "    CcfvEngine: ground disequality (" << l
                      << " != " << r << ")" << std::endl;
        if (ee->hasTerm(l) && ee->hasTerm(r))
        {
          if (!ee->areDisequal(l, r, false))
          {
            Trace("ccfv")
                << "      CcfvEngine: they are not disequal, failing branch"
                << std::endl;
            return {};
          }
          Trace("ccfv")
              << "      CcfvEngine: they are disequal, removing from C"
              << std::endl;
          return solveProceduralRec(
              freeVars, newLits, ee, indices, currentSubstitution);
        }
      }

      // Assignment into a disequal term
      // x != t or t != x
      if ((l.getKind() == Kind::BOUND_VARIABLE && rIsGround && ee->hasTerm(r))
          || (r.getKind() == Kind::BOUND_VARIABLE && lIsGround
              && ee->hasTerm(l)))
      {
        Node variable = (l.getKind() == Kind::BOUND_VARIABLE ? l : r);
        Node term = (l.getKind() == Kind::BOUND_VARIABLE ? r : l);

        Trace("ccfv") << "    CcfvEngine: assignment into a disequal term ("
                      << variable << " != " << term << ")" << std::endl;

        for (const Node& t : indices.getDiseqs(term))
        {
          Trace("ccfv") << "      CcfvEngine: assigning " << variable
                        << " to disequal rep " << t << " ("
                        << variable.eqNode(t) << ")" << std::endl;
          std::vector<Node> branchLits = newLits;
          branchLits.push_back(variable.eqNode(t));
          std::vector<std::vector<Node>> branchSols = solveProceduralRec(
              freeVars, branchLits, ee, indices, currentSubstitution);
          sols.insert(sols.end(), branchSols.begin(), branchSols.end());
        }

        return sols;
      }

      // E-matching
      // f(u_n) != t or t != f(u_n)
      if ((!ee->hasTerm(l) && l.hasOperator() && rIsGround && ee->hasTerm(r))
          || (!ee->hasTerm(r) && r.hasOperator() && lIsGround
              && ee->hasTerm(l)))
      {
        Node fTerm = ee->hasTerm(r) ? l : r;
        Node t = ee->hasTerm(r) ? r : l;

        Trace("ccfv") << "    CcfvEngine: E-Matching UF with disequal ground "
                         "term ("
                      << fTerm << " != " << t << ")" << std::endl;

        for (const Node& tt : indices.getDiseqs(t))
        {
          Trace("ccfv") << "      CcfvEngine: E-Matching " << fTerm
                        << " with disequal rep " << tt << " and adding "
                        << fTerm.eqNode(tt) << " to C" << std::endl;
          std::vector<Node> branchLits = newLits;
          branchLits.push_back(fTerm.eqNode(tt));
          std::vector<std::vector<Node>> branchSols = solveProceduralRec(
              freeVars, branchLits, ee, indices, currentSubstitution);
          sols.insert(sols.end(), branchSols.begin(), branchSols.end());
        }

        return sols;
      }

      // DFapp
      // x != f(u_n) or f(u_n) != x
      if ((l.getKind() == Kind::BOUND_VARIABLE && r.hasOperator())
          || (l.hasOperator() && r.getKind() == Kind::BOUND_VARIABLE))
      {
        Node variable = (l.getKind() == Kind::BOUND_VARIABLE ? l : r);
        Node term = (l.getKind() == Kind::BOUND_VARIABLE ? r : l);

        Trace("ccfv") << "    CcfvEngine: DFapp (" << variable << " != " << term
                      << ")" << std::endl;

        for (const Node& t : indices.getReps(term.getOperator()))
        {
          Trace("ccfv") << "      CcfvEngine: adding " << term.eqNode(t)
                        << " and " << variable.eqNode(t).negate() << " to C"
                        << std::endl;
          std::vector<Node> branchLits = newLits;
          branchLits.push_back(term.eqNode(t));
          branchLits.push_back(variable.eqNode(t).negate());
          std::vector<std::vector<Node>> branchSols = solveProceduralRec(
              freeVars, branchLits, ee, indices, currentSubstitution);
          sols.insert(sols.end(), branchSols.begin(), branchSols.end());
        }

        return sols;
      }

      // E-matching
      // f(u_n) != g(u'_n) or g(u'_n) != f(u_n)
      if (l.hasOperator() && r.hasOperator())
      {
        Trace("ccfv") << "    CcfvEngine: E-Matching into disequal terms (" << l
                      << " != " << r << ")" << std::endl;
        for (const Node& t : indices.getReps(l.getOperator()))
        {
          Trace("ccfv") << "      CcfvEngine: adding " << l.eqNode(t) << " and "
                        << r.eqNode(t).negate() << " to C" << std::endl;
          std::vector<Node> branchLits = newLits;
          branchLits.push_back(l.eqNode(t));
          branchLits.push_back(r.eqNode(t).negate());
          std::vector<std::vector<Node>> branchSols = solveProceduralRec(
              freeVars, branchLits, ee, indices, currentSubstitution);
          sols.insert(sols.end(), branchSols.begin(), branchSols.end());
        }

        return sols;
      }

      // DVar
      // x != y
      if (l.getKind() == Kind::BOUND_VARIABLE
          && r.getKind() == Kind::BOUND_VARIABLE)
      {
        Trace("ccfv") << "    CcfvEngine: DVar (" << l << " != " << r << ")"
                      << std::endl;
        for (eq::EqClassesIterator eqcs_i(ee); !eqcs_i.isFinished(); ++eqcs_i)
        {
          Node rep = *eqcs_i;
          if (rep.getType() != l.getType())
          {
            continue;
          }

          for (const Node& t : indices.getDiseqs(rep))
          {
            Trace("ccfv") << "      CcfvEngine: adding " << l.eqNode(rep)
                          << " and " << r.eqNode(t) << " to C" << std::endl;
            std::vector<Node> branchLits = newLits;
            branchLits.push_back(l.eqNode(rep));
            branchLits.push_back(r.eqNode(t));
            std::vector<std::vector<Node>> branchSols = solveProceduralRec(
                freeVars, branchLits, ee, indices, currentSubstitution);
            sols.insert(sols.end(), branchSols.begin(), branchSols.end());
          }
        }

        return sols;
      }

      break;
    }
    default: return {};
  }

  return sols;
}

bool CcfvSolver::solveProcedural(const std::vector<Node>& freeVars,
                                 const std::vector<Node>& lits,
                                 eq::EqualityEngine* ee,
                                 std::vector<std::vector<Node>>& substitutions)
{
  ccfv::CcfvIndices indices(ee);
  std::vector<Node> currentSubstitution(freeVars.begin(), freeVars.end());
  substitutions =
      solveProceduralRec(freeVars, lits, ee, indices, currentSubstitution);

  // If any free variable is left
  for (std::vector<Node>& sub : substitutions)
  {
    for (size_t i = 0; i < freeVars.size(); ++i)
    {
      if (freeVars[i] == sub[i])
      {
        TypeNode tn = freeVars[i].getType();
        Node groundTerm;

        // Math the free variable with a ground term of the same type that is a
        // representative of some equivalence class
        for (eq::EqClassesIterator eqcs_i(ee); !eqcs_i.isFinished(); ++eqcs_i)
        {
          if ((*eqcs_i).getType() == tn)
          {
            groundTerm = *eqcs_i;
            break;
          }
        }

        // If no such term is found, use the NodeManager API to generate a term
        // with the same type
        if (groundTerm.isNull())
        {
          groundTerm = NodeManager::mkGroundTerm(tn);
        }

        Trace("ccfv") << "  CcfvEngine: Grounding remaining free variable "
                      << freeVars[i] << " with " << groundTerm << std::endl;
        // Substitute the variable with the ground term
        for (Node& t : sub)
        {
          t = t.substitute(TNode(freeVars[i]), TNode(groundTerm));
        }
      }
    }
  }

  return !substitutions.empty();
}

Node toHoApply(TNode n)
{
  if (n.getNumChildren() == 0)
  {
    return n;
  }

  bool changed = false;
  std::vector<Node> children;
  for (TNode child : n)
  {
    Node newChild = toHoApply(child);
    if (newChild != child)
    {
      changed = true;
    }
    children.push_back(newChild);
  }

  NodeManager* nm = n.getNodeManager();

  if (n.getKind() == Kind::APPLY_UF)
  {
    Node curr = n.getOperator();
    for (const Node& child : children)
    {
      curr = nm->mkNode(Kind::HO_APPLY, curr, child);
    }
    return curr;
  }

  if (!changed)
  {
    return n;
  }

  return nm->mkNode(n.getKind(), children);
}

bool CcfvSolver::isPreprocessed(const std::vector<Node>& lits)
{
  for (TNode eq : lits)
  {
    switch (eq.getKind())
    {
      case Kind::EQUAL:
      {
        TNode l = eq[0], r = eq[1];
        TNode u0, u1, u2;

        if (l.getKind() == Kind::HO_APPLY && l.getNumChildren() == 2
            && expr::hasFreeVar(l))
        {
          u0 = r;
          u1 = l[0];
          u2 = l[1];
        }
        else if (r.getKind() == Kind::HO_APPLY && r.getNumChildren() == 2
                 && expr::hasFreeVar(r))
        {
          u0 = l;
          u1 = r[0];
          u2 = r[1];
        }
        else
        {
          return false;
        }

        if (u0.getKind() != Kind::BOUND_VARIABLE && expr::hasFreeVar(u0))
        {
          return false;
        }
        if (u1.getKind() != Kind::BOUND_VARIABLE && expr::hasFreeVar(u1))
        {
          return false;
        }
        if (u2.getKind() != Kind::BOUND_VARIABLE && expr::hasFreeVar(u2))
        {
          return false;
        }

        if (!expr::hasFreeVar(u1) && !expr::hasFreeVar(u2))
        {
          return false;
        }

        continue;
      }
      case Kind::NOT:
      {
        if (eq[0].getKind() != Kind::EQUAL)
        {
          return false;
        }

        TNode x = eq[0][0], y = eq[0][1];

        if (x.getKind() == Kind::BOUND_VARIABLE
            && y.getKind() == Kind::BOUND_VARIABLE)
        {
          continue;
        }

        if (x.getKind() == Kind::BOUND_VARIABLE && !expr::hasFreeVar(y))
        {
          continue;
        }
        if (y.getKind() == Kind::BOUND_VARIABLE && !expr::hasFreeVar(x))
        {
          continue;
        }

        return false;
      }
      default:
      {
        return false;
      }
    }
  }

  return true;
}

std::vector<Node> preprocess(const std::vector<Node>& lits,
                             eq::EqualityEngine* ee)
{
  std::vector<Node> preprocessedLits(lits.begin(), lits.end());

  // Normalizing

  // Removing Ground Literals

  // Flattening

  // Trivial Assignments

  return preprocessedLits;
}

Node CcfvSolver::getPVar(TNode x, TNode rep)
{
  auto it = d_pNodes.find({x, rep});
  if (it != d_pNodes.end())
  {
    return it->second;
  }
  NodeManager* nm = nodeManager();
  Node v = nm->mkDummySkolem("P_" + x.toString() + "_" + rep.toString(),
                             nm->booleanType());
  d_pNodes[{x, rep}] = v;
  return v;
}

Node CcfvSolver::getQVar(TNode x)
{
  auto it = d_qNodes.find(x);
  if (it != d_qNodes.end())
  {
    return it->second;
  }
  NodeManager* nm = nodeManager();
  Node v = nm->mkDummySkolem("Q_" + x.toString(), nm->booleanType());
  d_qNodes[x] = v;
  return v;
}

void CcfvSolver::registerGroundTermsFromL(const std::vector<Node>& lits,
                                          eq::EqualityEngine* ee)
{
  auto addIfGround = [ee](TNode t) {
    if (!expr::hasFreeVar(t) && !ee->hasTerm(t))
    {
      Trace("ccfv") << "    CcfvEngine: registering ground term " << t
                    << " into EE" << std::endl;
      ee->addTerm(t);
    }
  };

  for (TNode lit : lits)
  {
    if (lit.getKind() == Kind::EQUAL)
    {
      // u0 = u1 u2
      TNode l = lit[0], r = lit[1];
      TNode u0 = (l.getKind() == Kind::HO_APPLY && expr::hasFreeVar(l)) ? r : l;
      TNode app =
          (l.getKind() == Kind::HO_APPLY && expr::hasFreeVar(l)) ? l : r;

      addIfGround(u0);
      if (app.getKind() == Kind::HO_APPLY)
      {
        addIfGround(app[0]);  // u1
        addIfGround(app[1]);  // u2
      }
    }
    else if (lit.getKind() == Kind::NOT && lit[0].getKind() == Kind::EQUAL)
    {
      // x != u
      TNode x = lit[0][0], u = lit[0][1];
      addIfGround(x);
      addIfGround(u);
    }
  }
}

std::map<Node, std::vector<std::pair<Node, Node>>>
CcfvSolver::computeSignatures(eq::EqualityEngine* ee)
{
  std::map<Node, std::set<std::pair<Node, Node>>> uniqueSignatures = {};

  for (eq::EqClassesIterator eqcs_i(ee); !eqcs_i.isFinished(); ++eqcs_i)
  {
    TNode rep0 = *eqcs_i;
    eq::EqClassIterator eqc_i(rep0, ee);
    while (!eqc_i.isFinished())
    {
      Node hoTerm = toHoApply(*eqc_i);
      if (hoTerm.getKind() == Kind::HO_APPLY)
      {
        if (ee->hasTerm(hoTerm[0]) && ee->hasTerm(hoTerm[1]))
        {
          Node rep1 = ee->getRepresentative(hoTerm[0]);
          Node rep2 = ee->getRepresentative(hoTerm[1]);
          uniqueSignatures[rep0].insert({rep1, rep2});
        }
      }

      ++eqc_i;
    }
  }

  std::map<Node, std::vector<std::pair<Node, Node>>> signatures = {};
  for (auto& [rep0, signatureSet] : uniqueSignatures)
  {
    signatures[rep0].assign(signatureSet.begin(), signatureSet.end());
  }
  return signatures;
}

bool CcfvSolver::solveSat(const std::vector<Node>& freeVars,
                          const std::vector<Node>& lits,
                          eq::EqualityEngine* ee,
                          std::vector<std::vector<Node>>& substitutions)
{
  Trace("ccfv") << "  CcfvEngine: solveSat with " << freeVars.size()
                << " free variables, " << lits.size() << " literals"
                << std::endl;
  Trace("ccfv-debug") << "  CcfvEngine: freeVars = " << freeVars << std::endl;
  Trace("ccfv-debug") << "  CcfvEngine: L = " << lits << std::endl;

  // TODO: With --simplifications (which is enabled by default in cvc5), the
  // Equality Engine state changes substantially for some problems. It is good
  // to check if this breaks some of the operations done by preprocessing +
  // encoding or it is ok.

  // TODO: By default, cvc5 canonizes fully applied higher-order applications
  // (HO_APPLY) into standard first-order applications (APPLY_UF). To simplify
  // subsequent checks and keep the encoding purely binary/curried, we normalize
  // all literals in L using `toHoApply`. Once the full preprocessing pipeline
  // is implemented, this normalization may become redundant and can be removed.
  std::vector<Node> hoLits = {};
  for (const Node& lit : lits)
  {
    hoLits.push_back(toHoApply(lit));
  }

  Trace("ccfv-debug") << "  CcfvEngine: L (after normalization) = " << hoLits
                      << std::endl;

  if (!isPreprocessed(hoLits))
  {
    // TODO: preprocess instead of throwing an exception
    throw Exception("Equations in L are not in the expected form");
  }

  // Ground terms that appear only in L have to be registered to be explicit
  // in the Equality Engine reasoning
  registerGroundTermsFromL(hoLits, ee);
  ccfv::HoccfvUtils hoccfvUtils(freeVars, hoLits);

  // Floater Equivalence
  Trace("ccfv") << "    CcfvEngine: computing floater equivalences"
                << std::endl;
  hoccfvUtils.computeFloaterEquivalences();

  // Ground or Type Constraints for floaters
  Trace("ccfv")
      << "    CcfvEngine: solving ground and type constraints for floaters"
      << std::endl;
  hoccfvUtils.applyGroundOrTypeConstraints();

  // // Cycle Detection
  // Trace("ccfv") << "    CcfvEngine: computing the floater-dependency graph to
  // "
  //                  "detect cycle variables"
  //               << std::endl;
  // hoccfvUtils.computeFloaterDependencyGraph();

  return false;

  std::map<Node, std::vector<std::pair<Node, Node>>> signatures =
      computeSignatures(ee);

  std::vector<Node> negativeConstraints = {};
  std::vector<Node> positiveConstraints = {};
  for (const Node& lit : hoLits)
  {
    if (lit.getKind() == Kind::EQUAL)
    {
      positiveConstraints.push_back(lit);
    }
    else
    {
      // Since we already guarded the format of literals in L, if lit.getKind()
      // != Kind::EQUAL then it is, for sure, Kind::NOT and lit[0].getKind() is
      // Kind::EQUAL
      negativeConstraints.push_back(lit);
    }
  }

  Trace("ccfv") << "    CcfvEngine: " << positiveConstraints.size()
                << " equalities, " << negativeConstraints.size()
                << " disequalities" << std::endl;

  d_pNodes.clear();
  d_qNodes.clear();
  StatisticsRegistry localStats;
  std::unique_ptr<prop::SatSolver> satSolver(
      prop::SatSolverFactory::createSatSolver<
          options::BvSatSolverMode::CADICAL>(
          d_env, localStats, d_env.getResourceManager(), "ccfv::"));

  prop::NullRegistrar nullRegistrar;
  context::Context nullContext;
  prop::CnfStream cnfStream(d_env,
                            satSolver.get(),
                            &nullRegistrar,
                            &nullContext,
                            prop::FormulaLitPolicy::INTERNAL,
                            "ccfv_sat");

  NodeManager* nm = nodeManager();

  // Get disequal classes
  std::vector<Node> allReps = {};
  std::vector<std::pair<Node, Node>> diseqClasses = {};
  for (eq::EqClassesIterator eqcs_i(ee); !eqcs_i.isFinished(); ++eqcs_i)
  {
    allReps.push_back(*eqcs_i);
  }
  for (size_t i = 0; i < allReps.size(); ++i)
  {
    for (size_t j = i + 1; j < allReps.size(); ++j)
    {
      if (ee->areDisequal(allReps[i], allReps[j], false))
      {
        diseqClasses.push_back({allReps[i], allReps[j]});
      }
    }
  }

  // Core Encoding
  Trace("ccfv") << "    CcfvEngine: starting to generate the core encoding"
                << std::endl;
  Trace("ccfv") << "    CcfvEngine: encoding negative constraints" << std::endl;
  for (TNode negConstraint : negativeConstraints)
  {
    Trace("ccfv-debug") << "      CcfvEngine: diseq " << negConstraint
                        << std::endl;
    TNode x = (negConstraint[0][0].getKind() == Kind::BOUND_VARIABLE)
                  ? negConstraint[0][0]
                  : negConstraint[0][1];
    TNode u = (negConstraint[0][0].getKind() == Kind::BOUND_VARIABLE)
                  ? negConstraint[0][1]
                  : negConstraint[0][0];

    std::vector<Node> disjuncts = {};

    if (!expr::hasFreeVar(u))
    {
      // If u is a ground term, then x can only be terms that their
      // representatives are different than the representativa of u. We create
      // a disjunction of every P_x_[t] for all [t] != [u].
      Node repU = ee->getRepresentative(u);
      for (TNode rep : allReps)
      {
        if (rep.getType() == x.getType() && rep != repU)
        {
          disjuncts.push_back(getPVar(x, rep));
        }
      }
    }
    else
    {
      // If both terms are variables (x != y), they must be assigned to two
      // distinct congruence classes [t1] and [t2] known to be disequal in E^cc
      // (E |= t1 != t2). We add a disjunct (P_{x, [t1]} /\ P_{y, [t2]}) for
      // every compatible disequal pair of representatives
      for (auto [rep1, rep2] : diseqClasses)
      {
        if (rep1.getType() == x.getType() && rep2.getType() == u.getType())
        {
          disjuncts.push_back(
              nm->mkNode(Kind::AND, getPVar(x, rep1), getPVar(u, rep2)));
          disjuncts.push_back(
              nm->mkNode(Kind::AND, getPVar(x, rep2), getPVar(u, rep1)));
        }
      }
    }

    cnfStream.convertAndAssert(nm->mkOr(disjuncts), false, false);
  }

  Trace("ccfv") << "    CcfvEngine: encoding positive constraints" << std::endl;
  for (TNode posConstraint : positiveConstraints)
  {
    Trace("ccfv-debug") << "      CcfvEngine: eq " << posConstraint
                        << std::endl;
    TNode l = posConstraint[0], r = posConstraint[1];
    TNode app = (l.getKind() == Kind::HO_APPLY && expr::hasFreeVar(l)) ? l : r;
    TNode u0 = (l.getKind() == Kind::HO_APPLY && expr::hasFreeVar(l)) ? r : l,
          u1 = app[0], u2 = app[1];

    std::vector<Node> disjuncts = {};

    if (u0.getKind() == Kind::BOUND_VARIABLE)
    {
      // If u0 is a variable, it may be a floater (i.e. assigned outside of
      // E^cc)
      disjuncts.push_back(getQVar(u0));

      // Otherwise, u0 is mapped to some class [t0] in E^cc of compatible type
      for (TNode rep0 : allReps)
      {
        if (u0.getType() == rep0.getType())
        {
          auto it = signatures.find(rep0);
          if (it == signatures.end()) continue;

          // For each signature [t1][t2] in [[t0]], check if u1 and u2 can match
          for (const auto& [rep1, rep2] : it->second)
          {
            // Prune branches where ground terms do not match the signature
            // classes
            if (!expr::hasFreeVar(u1) && ee->getRepresentative(u1) != rep1)
              continue;
            if (!expr::hasFreeVar(u2) && ee->getRepresentative(u2) != rep2)
              continue;

            // Collect active P-predicates for remaining free variables
            std::vector<Node> conjuncts;
            if (u0.getKind() == Kind::BOUND_VARIABLE)
              conjuncts.push_back(getPVar(u0, rep0));
            if (u1.getKind() == Kind::BOUND_VARIABLE)
              conjuncts.push_back(getPVar(u1, rep1));
            if (u2.getKind() == Kind::BOUND_VARIABLE)
              conjuncts.push_back(getPVar(u2, rep2));

            disjuncts.push_back(nm->mkAnd(conjuncts));
          }
        }
      }
    }
    else
    {
      // If u0 is a ground term, it cannot be a floater (Q_u0 is false).
      // We only consider the signatures of the class [u0] in E^cc.
      Node rep0 = ee->getRepresentative(u0);
      auto it = signatures.find(rep0);
      if (it == signatures.end()) continue;

      for (const auto& [rep1, rep2] : it->second)
      {
        if (!expr::hasFreeVar(u1) && ee->getRepresentative(u1) != rep1)
          continue;
        if (!expr::hasFreeVar(u2) && ee->getRepresentative(u2) != rep2)
          continue;

        std::vector<Node> conjuncts;
        if (u1.getKind() == Kind::BOUND_VARIABLE)
          conjuncts.push_back(getPVar(u1, rep1));
        if (u2.getKind() == Kind::BOUND_VARIABLE)
          conjuncts.push_back(getPVar(u2, rep2));

        disjuncts.push_back(nm->mkAnd(conjuncts));
      }
    }

    cnfStream.convertAndAssert(nm->mkOr(disjuncts), false, false);
  }

  // Mutual Exclusion
  Trace("ccfv") << "    CcfvEngine: encoding mutual exclusion constraints"
                << std::endl;
  for (const Node& var : freeVars)
  {
    std::vector<Node> pVars = {};
    for (const Node& rep : allReps)
    {
      auto valid = d_pNodes.find({var, rep});
      if (valid != d_pNodes.end())
      {
        pVars.push_back(valid->second);
      }
    }

    auto qIt = d_qNodes.find(var);
    if (qIt != d_qNodes.end())
    {
      pVars.push_back(qIt->second);
    }

    for (size_t i = 0; i < pVars.size(); ++i)
    {
      for (size_t j = i + 1; j < pVars.size(); ++j)
      {
        cnfStream.convertAndAssert(
            nm->mkNode(Kind::OR, pVars[i].negate(), pVars[j].negate()),
            false,
            false);
      }
    }
  }

  // Floater Constraints
  // TODO:

  // Cycle-based Constraints
  // TODO:

  Trace("ccfv") << "    CcfvEngine: invoking SAT solver" << std::endl;
  prop::SatValue result = satSolver->solve();
  Trace("ccfv") << "    CcfvEngine: SAT solver returned " << result
                << std::endl;
  if (result == prop::SAT_VALUE_TRUE)
  {
    Trace("ccfv") << "    CcfvEngine: SAT solution found" << std::endl;
    // Extract the result
    // Populate the substitutions parameter
    return true;
  }

  Trace("ccfv") << "    CcfvEngine: SAT returned unsat, no solution"
                << std::endl;
  return false;
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
