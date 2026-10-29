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

#include "base/output.h"
#include "expr/node_algorithm.h"
#include "theory/quantifiers/ccfv/ccfv_utils.h"
#include "theory/uf/equality_engine.h"
#include "theory/uf/equality_engine_iterator.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

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
      Node l = c[0], r = c[1];

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
        Node variable = (l.getKind() == Kind::BOUND_VARIABLE ? l : r);
        Node term = (l.getKind() == Kind::BOUND_VARIABLE ? r : l);

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
            Trace("ccfv") << "      CcfvEngine: " << term << " is an UF"
                          << std::endl;

            Node op = term.getOperator();

            for (const Node& t : indices.getReps(op))
            {
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
        Node fTerm = ee->hasTerm(r) ? l : r;
        Node t = ee->hasTerm(r) ? r : l;
        Node op = fTerm.getOperator();

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
        return {};
      }

      // Ground disequality
      // t1 != t2
      bool lIsGround = !expr::hasFreeVar(l), rIsGround = !expr::hasFreeVar(r);
      if (lIsGround && rIsGround)
      {
        if (ee->hasTerm(l) && ee->hasTerm(r))
        {
          if (!ee->areDisequal(l, r, false))
          {
            return {};
          }
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

        for (const Node& t : indices.getDiseqs(term))
        {
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
        Node t = ee->hasTerm(l) ? r : l;

        for (const Node& tt : indices.getDiseqs(t))
        {
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

        for (const Node& t : indices.getReps(term.getOperator()))
        {
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
        for (const Node& t : indices.getReps(l.getOperator()))
        {
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
        for (eq::EqClassesIterator eqcs_i(ee); !eqcs_i.isFinished(); ++eqcs_i)
        {
          Node rep = *eqcs_i;
          if (rep.getType() != l.getType())
          {
            continue;
          }

          for (const Node& t : indices.getDiseqs(rep))
          {
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

bool CcfvSolver::solveSat(const std::vector<Node>& freeVars,
                          const std::vector<Node>& lits,
                          eq::EqualityEngine* ee,
                          std::vector<std::vector<Node>>& solutions)
{
  // TODO: Implement CCFV SAT Encoding
  return false;
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
