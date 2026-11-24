/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * SAT-encoded CCFV solver for E-ground (dis)unification.
 */

#include "theory/quantifiers/ccfv/ccfv_sat_solver.h"

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

CcfvSatSolver::CcfvSatSolver(Env& env) : EnvObj(env) {}

Node toHoApply(TNode n)
{
  if (n.getNumChildren() == 0 || !expr::hasBoundVar(n))
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

bool CcfvSatSolver::isPreprocessed(const std::vector<Node>& lits,
                                   eq::EqualityEngine* ee)
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
            && expr::hasBoundVar(l))
        {
          u0 = r;
          u1 = l[0];
          u2 = l[1];
        }
        else if (r.getKind() == Kind::HO_APPLY && r.getNumChildren() == 2
                 && expr::hasBoundVar(r))
        {
          u0 = l;
          u1 = r[0];
          u2 = r[1];
        }
        else
        {
          return false;
        }

        if (!expr::hasBoundVar(u0)
            && (!ee->hasTerm(u0) || ee->getRepresentative(u0) != u0))
        {
          return false;
        }

        if (!expr::hasBoundVar(u1)
            && (!ee->hasTerm(u1) || ee->getRepresentative(u1) != u1))
        {
          return false;
        }

        if (!expr::hasBoundVar(u2)
            && (!ee->hasTerm(u2) || ee->getRepresentative(u2) != u2))
        {
          return false;
        }

        if ((u0.getKind() != Kind::BOUND_VARIABLE && expr::hasBoundVar(u0))
            || (u1.getKind() != Kind::BOUND_VARIABLE && expr::hasBoundVar(u1))
            || (u2.getKind() != Kind::BOUND_VARIABLE && expr::hasBoundVar(u2))
            || (!expr::hasBoundVar(u1) && !expr::hasBoundVar(u2)))
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

        if (!expr::hasBoundVar(x)
            && (!ee->hasTerm(x) || ee->getRepresentative(x) != x))
        {
          return false;
        }

        if (!expr::hasBoundVar(y)
            && (!ee->hasTerm(y) || ee->getRepresentative(y) != y))
        {
          return false;
        }

        if ((x.getKind() == Kind::BOUND_VARIABLE
             && y.getKind() == Kind::BOUND_VARIABLE)
            || (x.getKind() == Kind::BOUND_VARIABLE && !expr::hasBoundVar(y))
            || (y.getKind() == Kind::BOUND_VARIABLE && !expr::hasBoundVar(x)))
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

bool CcfvSatSolver::normalize(std::vector<Node>& lits, eq::EqualityEngine* ee)
{
}

bool CcfvSatSolver::flatten(std::vector<Node>& lits) {}

bool CcfvSatSolver::removeGroundLiterals(std::vector<Node>& lits) {}

bool CcfvSatSolver::removeTrivialAssignments(std::vector<Node>& lits) {}

void CcfvSatSolver::preprocess(std::vector<Node>& lits, eq::EqualityEngine* ee)
{
  std::cout << "preprocessing" << std::endl;
  exit(1);
  bool changed = true;
  while (changed)
  {
    changed = false;
    // changed |= normalize(lits, ee);
    // changed |= removeGroundLiterals(lits);
    // changed |= flatten(lits);
    // changed |= removeTrivialAssignments(lits);
  }

  Assert(isPreprocessed(lits, ee));
}

void CcfvSatSolver::registerGroundTermsFromL(const std::vector<Node>& lits,
                                             eq::EqualityEngine* ee)
{
  auto addIfGround = [ee](TNode t) {
    if (!expr::hasBoundVar(t) && !ee->hasTerm(t))
    {
      Trace("ccfv-debug") << "[sat]     registering ground term " << t
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
      TNode u0 =
          (l.getKind() == Kind::HO_APPLY && expr::hasBoundVar(l)) ? r : l;
      TNode app =
          (l.getKind() == Kind::HO_APPLY && expr::hasBoundVar(l)) ? l : r;

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
CcfvSatSolver::computeSignatures(eq::EqualityEngine* ee)
{
  std::map<Node, std::set<std::pair<Node, Node>>> uniqueSignatures = {};

  for (eq::EqClassesIterator eqcs_i(ee); !eqcs_i.isFinished(); ++eqcs_i)
  {
    TNode rep0 = *eqcs_i;
    eq::EqClassIterator eqc_i(rep0, ee);
    while (!eqc_i.isFinished())
    {
      Node hoTerm = (*eqc_i).getKind() == Kind::APPLY_UF
                        ? uf::TheoryUfRewriter::getHoApplyForApplyUf(*eqc_i)
                        : Node(*eqc_i);
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

bool CcfvSatSolver::solve(const std::vector<Node>& freeVars,
                          const std::vector<Node>& lits,
                          eq::EqualityEngine* ee,
                          std::vector<std::vector<Node>>& substitutions)
{
  Trace("ccfv-debug") << "[sat]   solveSat with " << freeVars.size()
                      << " free variables, " << lits.size() << " literals"
                      << std::endl;
  Trace("ccfv-debug") << "[sat]   freeVars = " << freeVars << std::endl;
  Trace("ccfv-debug") << "[sat]   L = " << lits << std::endl;

  // TODO: With --simplification (which is enabled by default in cvc5), the
  // Equality Engine state changes substantially for some problems. It is good
  // to check if this breaks some of the operations done by preprocessing +
  // encoding or it is ok.

  // TODO: By default, cvc5 canonizes fully applied higher-order applications
  // (HO_APPLY) into standard first-order applications (APPLY_UF). To simplify
  // subsequent checks and keep the encoding purely binary/curried, we
  // normalize all literals in L using `toHoApply`. Once the full
  // preprocessing pipeline is implemented, this normalization may become
  // redundant and can be removed.
  std::vector<Node> hoLits = {};
  for (const Node& lit : lits)
  {
    hoLits.push_back(toHoApply(lit));
  }

  Trace("ccfv-debug") << "[sat]   EE before preprocessing:\n"
                      << ee->debugPrintEqc() << std::endl;

  if (!isPreprocessed(hoLits, ee))
  {
    preprocess(hoLits, ee);
  }

  Trace("ccfv-debug") << "[sat]   L (after preprocessing) = " << hoLits
                      << std::endl;

  // Ground terms that appear only in L have to be registered to be explicit
  // in the Equality Engine reasoning
  registerGroundTermsFromL(hoLits, ee);
  ccfv::HoccfvUtils hoccfvUtils(freeVars, hoLits);

  // Floater Equivalence
  Trace("ccfv-debug") << "[sat]     computing floater equivalences"
                      << std::endl;
  hoccfvUtils.computeFloaterEquivalences();

  // Ground or Type Constraints for floaters
  Trace("ccfv-debug")
      << "[sat]     solving ground and type constraints for floaters"
      << std::endl;
  hoccfvUtils.applyGroundOrTypeConstraints();

  // Cycle Detection
  Trace("ccfv-debug") << "[sat]     computing the floater-dependency graph to "
                         "detect cycle variables"
                      << std::endl;
  hoccfvUtils.computeFloaterDependencyGraph();

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
      // Since we already guarded the format of literals in L, if
      // lit.getKind()
      // != Kind::EQUAL then it is, for sure, Kind::NOT and lit[0].getKind()
      // is Kind::EQUAL
      negativeConstraints.push_back(lit);
    }
  }

  Trace("ccfv-debug") << "[sat]     " << positiveConstraints.size()
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
  std::vector<Node> satClauses;

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
  Trace("ccfv-debug") << "[sat]     starting to generate the core encoding"
                      << std::endl;
  Trace("ccfv-debug") << "[sat]     encoding negative constraints" << std::endl;
  for (TNode negConstraint : negativeConstraints)
  {
    Trace("ccfv-debug") << "[sat]       diseq " << negConstraint << std::endl;
    TNode x = (negConstraint[0][0].getKind() == Kind::BOUND_VARIABLE)
                  ? negConstraint[0][0]
                  : negConstraint[0][1];
    TNode u = (negConstraint[0][0].getKind() == Kind::BOUND_VARIABLE)
                  ? negConstraint[0][1]
                  : negConstraint[0][0];

    std::vector<Node> disjuncts = {};

    if (!expr::hasBoundVar(u))
    {
      // If u is a ground term, then x can only be terms that their
      // representatives are different than the representativa of u. We create
      // a disjunction of every P_x_[t] for all [t] != [u]
      Node repU = ee->getRepresentative(u);
      for (TNode rep : allReps)
      {
        if (rep.getType() == x.getType() && ee->areDisequal(rep, repU, false))
        {
          disjuncts.push_back(getPVar(x, rep));
        }
      }
    }
    else
    {
      // If both terms are variables (x != y), they must be assigned to two
      // distinct congruence classes [t1] and [t2] known to be disequal in
      // E^cc (E |= t1 != t2). We add a disjunct (P_{x, [t1]} /\ P_{y, [t2]})
      // for every compatible disequal pair of representatives
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

    // unsat directly
    if (disjuncts.empty())
    {
      return false;
    }
    satClauses.push_back(nm->mkOr(disjuncts));
  }

  Trace("ccfv-debug") << "[sat]     encoding positive constraints" << std::endl;
  for (TNode posConstraint : positiveConstraints)
  {
    Trace("ccfv-debug") << "[sat]       eq " << posConstraint << std::endl;
    TNode l = posConstraint[0], r = posConstraint[1];
    TNode app = (l.getKind() == Kind::HO_APPLY && expr::hasBoundVar(l)) ? l : r;
    TNode u0 = (l.getKind() == Kind::HO_APPLY && expr::hasBoundVar(l)) ? r : l,
          u1 = app[0], u2 = app[1];

    std::vector<Node> disjuncts = {};

    if (u0.getKind() == Kind::BOUND_VARIABLE)
    {
      // If u0 is a variable, it may be a floater (i.e. assigned outside of
      // E^cc)
      if (hoccfvUtils.isFloatable(u0))
      {
        disjuncts.push_back(getQVar(hoccfvUtils.getDsu()->find(u0)));
      }

      // Otherwise, u0 is mapped to some class [t0] in E^cc of compatible type
      for (TNode rep0 : allReps)
      {
        if (u0.getType() == rep0.getType())
        {
          auto it = signatures.find(rep0);
          if (it == signatures.end()) continue;

          // For each signature [t1][t2] in [[t0]], check if u1 and u2 can
          // match
          for (const auto& [rep1, rep2] : it->second)
          {
            if (u1.getType() != rep1.getType()
                || u2.getType() != rep2.getType())
            {
              continue;
            }

            // Prune branches where ground terms do not match the signature
            // classes
            if (!expr::hasBoundVar(u1) && ee->getRepresentative(u1) != rep1)
              continue;
            if (!expr::hasBoundVar(u2) && ee->getRepresentative(u2) != rep2)
              continue;

            std::vector<Node> conjuncts;
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
      if (it == signatures.end())
      {
        Trace("ccfv-debug") << "[sat]       no signatures for ground class "
                            << rep0 << ", unsatisfiable" << std::endl;
        return false;
      }

      for (const auto& [rep1, rep2] : it->second)
      {
        // types cannot be different
        if (u1.getType() != rep1.getType() || u2.getType() != rep2.getType())
          continue;

        if (!expr::hasBoundVar(u1) && ee->getRepresentative(u1) != rep1)
          continue;
        if (!expr::hasBoundVar(u2) && ee->getRepresentative(u2) != rep2)
          continue;

        std::vector<Node> conjuncts;
        if (u1.getKind() == Kind::BOUND_VARIABLE)
          conjuncts.push_back(getPVar(u1, rep1));
        if (u2.getKind() == Kind::BOUND_VARIABLE)
          conjuncts.push_back(getPVar(u2, rep2));

        disjuncts.push_back(nm->mkAnd(conjuncts));
      }
    }

    // unsat directly
    if (disjuncts.empty())
    {
      return false;
    }
    satClauses.push_back(nm->mkOr(disjuncts));
  }

  // Floater Injectivity
  std::vector<Node> V = {};
  std::map<Node, std::set<Node>> children[2];
  std::map<Node, std::set<Node>> gRange;

  for (TNode posConstraint : positiveConstraints)
  {
    TNode l = posConstraint[0], r = posConstraint[1];
    TNode u0 = (l.getKind() == Kind::HO_APPLY && expr::hasBoundVar(l)) ? r : l;
    TNode u = (l.getKind() == Kind::HO_APPLY && expr::hasBoundVar(l)) ? l : r;

    if (u0.getKind() == Kind::BOUND_VARIABLE)
    {
      Node root = hoccfvUtils.getDsu()->find(u0);
      V.push_back(root);
      for (size_t i = 0; i < 2; ++i)
      {
        if (u[i].getKind() == Kind::BOUND_VARIABLE)
        {
          children[i][root].insert(u[i]);
        }
      }
    }
  }

  for (TNode x : freeVars)
  {
    for (TNode rep : allReps)
    {
      auto occurs = d_pNodes.find({x, rep});
      if (occurs != d_pNodes.end())
      {
        gRange[hoccfvUtils.getDsu()->find(x)].insert(rep);
      }
    }
  }

  std::sort(V.begin(), V.end());
  V.erase(std::unique(V.begin(), V.end()), V.end());

  for (TNode x : V)
  {
    if (hoccfvUtils.isFloatable(x))
    {
      for (size_t i = 0; i < 2; ++i)
      {
        if (!hoccfvUtils.getGround(x, i).isNull())
        {
          for (TNode y : children[i][x])
          {
            satClauses.push_back(nm->mkNode(
                Kind::OR,
                getQVar(x).negate(),
                getPVar(y,
                        ee->getRepresentative(hoccfvUtils.getGround(x, i)))));
          }
        }
        else
        {
          for (TNode y1 : children[i][x])
          {
            for (TNode y2 : children[i][x])
            {
              if (y1 < y2)
              {
                for (TNode rep : gRange[hoccfvUtils.getDsu()->find(y1)])
                {
                  satClauses.push_back(nm->mkNode(
                      Kind::OR,
                      getQVar(x).negate(),
                      nm->mkNode(
                          Kind::EQUAL, getPVar(y1, rep), getPVar(y2, rep))));
                }
              }
            }
          }
        }
      }
    }
  }

  // Mutual Exclusion
  Trace("ccfv-debug") << "[sat]     encoding mutual exclusion constraints"
                      << std::endl;
  for (TNode var : freeVars)
  {
    std::vector<Node> pVars = {};
    for (TNode rep : allReps)
    {
      auto occurs = d_pNodes.find({var, rep});
      if (occurs != d_pNodes.end())
      {
        pVars.push_back(occurs->second);
      }
    }

    if (hoccfvUtils.isFloatable(var))
    {
      Node qRep = hoccfvUtils.getDsu()->find(var);
      auto qIt = d_qNodes.find(qRep);
      if (qIt != d_qNodes.end())
      {
        pVars.push_back(qIt->second);
      }
    }

    for (size_t i = 0; i < pVars.size(); ++i)
    {
      for (size_t j = i + 1; j < pVars.size(); ++j)
      {
        satClauses.push_back(
            nm->mkNode(Kind::OR, pVars[i].negate(), pVars[j].negate()));
      }
    }
  }

  Trace("ccfv-debug") << "[sat]     generated " << satClauses.size()
                      << " SAT clauses" << std::endl;
  if (TraceIsOn("ccfv-debug"))
  {
    Trace("ccfv-debug") << "[sat] === SAT encoding (" << satClauses.size()
                        << " clauses) ===" << std::endl;
    for (size_t i = 0; i < satClauses.size(); ++i)
    {
      Trace("ccfv-debug") << "  [" << i << "] " << satClauses[i] << std::endl;
    }
    Trace("ccfv-debug") << "--- FULL FORMULA (AND) ---" << std::endl;
    Trace("ccfv-debug") << nm->mkAnd(satClauses) << std::endl;
    Trace("ccfv-debug") << "=========================" << std::endl;
  }

  for (const Node& clause : satClauses)
  {
    cnfStream.convertAndAssert(clause, false, false);
  }

  Trace("ccfv-debug") << "[sat]     invoking SAT solver" << std::endl;
  prop::SatValue result = satSolver->solve();
  Trace("ccfv-debug") << "[sat]     SAT solver returned " << result
                      << std::endl;
  if (result == prop::SAT_VALUE_TRUE)
  {
    Trace("ccfv-debug") << "[sat]     SAT solution found" << std::endl;

    Trace("ccfv-debug") << std::endl << "[sat] === E^cc ===" << std::endl;
    for (eq::EqClassesIterator eqcs_i(ee); !eqcs_i.isFinished(); ++eqcs_i)
    {
      Node rep = *eqcs_i;
      Trace("ccfv-debug") << "  Eqc [" << rep << "] : { ";
      for (eq::EqClassIterator eqc_i(rep, ee); !eqc_i.isFinished(); ++eqc_i)
      {
        Trace("ccfv-debug") << *eqc_i << " ";
      }
      Trace("ccfv-debug") << "}" << std::endl;
      auto sigIt = signatures.find(rep);
      if (sigIt != signatures.end() && !sigIt->second.empty())
      {
        Trace("ccfv-debug") << "    Signatures: ";
        for (const auto& [rep1, rep2] : sigIt->second)
        {
          Trace("ccfv-debug") << "([" << rep1 << "], [" << rep2 << "]) ";
        }
        Trace("ccfv-debug") << std::endl;
      }
    }
    Trace("ccfv-debug") << "===============================" << std::endl;

    Trace("ccfv-debug") << std::endl << "[sat] === model ===" << std::endl;
    for (const auto& [var, qNode] : d_qNodes)
    {
      if (cnfStream.hasLiteral(qNode))
      {
        prop::SatLiteral lit = cnfStream.getLiteral(qNode);
        if (satSolver->modelValue(lit) == prop::SAT_VALUE_TRUE)
        {
          Trace("ccfv-debug") << "  " << qNode << " = TRUE  (variable " << var
                              << " is a FLOATER)" << std::endl;
        }
      }
    }
    for (const auto& [key, pNode] : d_pNodes)
    {
      if (cnfStream.hasLiteral(pNode))
      {
        prop::SatLiteral lit = cnfStream.getLiteral(pNode);
        if (satSolver->modelValue(lit) == prop::SAT_VALUE_TRUE)
        {
          Trace("ccfv-debug") << "  " << pNode << " = TRUE  (" << key.first
                              << " -> [" << key.second << "])" << std::endl;
        }
      }
    }
    Trace("ccfv-debug") << "=================" << std::endl << std::endl;

    // Reconstructing the substitution
    std::vector<Node> sub(freeVars.begin(), freeVars.end());

    for (const auto& [key, pNode] : d_pNodes)
    {
      if (cnfStream.hasLiteral(pNode))
      {
        prop::SatLiteral lit = cnfStream.getLiteral(pNode);
        if (satSolver->modelValue(lit) == prop::SAT_VALUE_TRUE)
        {
          auto it = std::find(freeVars.begin(), freeVars.end(), key.first);
          if (it != freeVars.end())
          {
            size_t idx = std::distance(freeVars.begin(), it);
            sub[idx] = key.second;
          }
        }
      }
    }

    auto resolveTerm = [&](TNode t) -> Node {
      auto it = std::find(freeVars.begin(), freeVars.end(), t);
      if (it != freeVars.end())
      {
        return sub[std::distance(freeVars.begin(), it)];
      }
      return Node(t);
    };

    for (size_t i = 0; i < freeVars.size(); ++i)
    {
      if (sub[i] != freeVars[i])
      {
        continue;
      }

      Node rep = hoccfvUtils.getDsu()->find(freeVars[i]);
      if (freeVars[i] != rep)
      {
        sub[i] = resolveTerm(rep);
      }
      else
      {
        Node g0 = hoccfvUtils.getGround(rep, 0);
        Node c0 = hoccfvUtils.getDsu()->getChild(rep, 0);
        Node t1 =
            !g0.isNull() ? g0 : (!c0.isNull() ? resolveTerm(c0) : Node::null());

        Node g1 = hoccfvUtils.getGround(rep, 1);
        Node c1 = hoccfvUtils.getDsu()->getChild(rep, 1);
        Node t2 =
            !g1.isNull() ? g1 : (!c1.isNull() ? resolveTerm(c1) : Node::null());

        if (!t1.isNull() && !t2.isNull())
        {
          sub[i] = nm->mkNode(Kind::HO_APPLY, t1, t2);
        }
      }
    }

    std::map<TypeNode, Node> defaultTermForType;
    for (size_t i = 0; i < freeVars.size(); ++i)
    {
      if (sub[i] == freeVars[i])
      {
        TypeNode tn = freeVars[i].getType();
        auto it = defaultTermForType.find(tn);
        if (it != defaultTermForType.end())
        {
          sub[i] = it->second;
        }
        else
        {
          Node groundTerm;
          for (eq::EqClassesIterator eqcs_i(ee); !eqcs_i.isFinished(); ++eqcs_i)
          {
            if ((*eqcs_i).getType() == tn)
            {
              groundTerm = *eqcs_i;
              break;
            }
          }
          if (groundTerm.isNull())
          {
            groundTerm = nm->mkGroundTerm(tn);
          }

          defaultTermForType[tn] = groundTerm;
          sub[i] = groundTerm;
        }
      }
    }

    // Normalizing substitution
    bool changed = true;
    while (changed)
    {
      changed = false;
      for (size_t j = 0; j < freeVars.size(); ++j)
      {
        if (expr::hasBoundVar(sub[j]))
        {
          Node newSub = sub[j].substitute(
              freeVars.begin(), freeVars.end(), sub.begin(), sub.end());
          if (newSub != sub[j])
          {
            sub[j] = newSub;
            changed = true;
          }
        }
      }
    }

    substitutions.push_back(sub);
    return true;
  }

  Trace("ccfv-debug") << "[sat]     SAT returned unsat, no solution"
                      << std::endl;
  return false;
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
