/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Backtracking CCFV solver for E-ground (dis)unification: a depth-first
 * search for substitutions sigma such that E |= L sigma.
 */

#include "theory/quantifiers/ccfv/ccfv_backtracking_solver.h"

#include <utility>

#include "base/output.h"
#include "expr/node_algorithm.h"
#include "theory/quantifiers/ccfv/ccfv_indices.h"
#include "theory/quantifiers/ccfv/ccfv_utils.h"
#include "theory/uf/equality_engine.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

CcfvBacktrackingSolver::Statistics::Statistics(StatisticsRegistry& sr)
    : d_solutions(sr.registerInt("ccfv::solver::solutions")),
      d_maxDepth(sr.registerInt("ccfv::solver::maxDepth")),
      d_leafDepth(sr.registerAverage("ccfv::solver::leafDepth")),
      d_failUnsupported(sr.registerInt("ccfv::solver::failUnsupported")),
      d_failNoCandidates(sr.registerInt("ccfv::solver::failNoCandidates")),
      d_failGroundEq(sr.registerInt("ccfv::solver::failGroundEq")),
      d_failDiseq(sr.registerInt("ccfv::solver::failDiseq")),
      d_failNotInEe(sr.registerInt("ccfv::solver::failNotInEe"))
{
}

CcfvBacktrackingSolver::CcfvBacktrackingSolver(Env& env)
    : EnvObj(env), d_stats(statisticsRegistry())
{
}

std::vector<std::vector<Node>> CcfvBacktrackingSolver::fail(size_t depth,
                                                            IntStat& reason)
{
  ++reason;
  d_stats.d_leafDepth << static_cast<double>(depth);
  d_stats.d_maxDepth.maxAssign(depth);
  return {};
}

Node CcfvBacktrackingSolver::normalizePredicate(TNode lit)
{
  if (lit.getKind() == Kind::APPLY_UF)
  {
    return lit.eqNode(nodeManager()->mkConst(true));
  }
  if (lit.getKind() == Kind::NOT && lit[0].getKind() == Kind::APPLY_UF)
  {
    return lit[0].eqNode(nodeManager()->mkConst(false));
  }
  return lit;
}

// Idea: pick literals based on how much branching it will generate
// Level 0: no branching
// Level 1: branching depending on the size
// Level 2: desiqualities
size_t CcfvBacktrackingSolver::selectLiteral(const std::vector<Node>& lits,
                                             eq::EqualityEngine* ee,
                                             const ccfv::CcfvIndices& indices)
{
  auto classify = [&](TNode lit) -> std::pair<int, size_t> {
    switch (lit.getKind())
    {
      case Kind::EQUAL:
      {
        TNode l = lit[0], r = lit[1];

        // Syntactic equality
        // s = s
        if (l == r)
        {
          return {0, 0};
        }

        // Ground equality
        // t1 = t2
        bool lIsGround = !expr::hasBoundVar(l),
             rIsGround = !expr::hasBoundVar(r);
        if (lIsGround && rIsGround)
        {
          if (ee->hasTerm(l) && ee->hasTerm(r))
          {
            return {0, 0};
          }
          else if (l.hasOperator() && r.hasOperator()
                   && l.getOperator() == r.getOperator()
                   && l.getNumChildren() == r.getNumChildren())
          {
            return {0, 0};
          }
        }

        // Assignmnet
        // x = s or s = x
        if (l.getKind() == Kind::BOUND_VARIABLE
            || r.getKind() == Kind::BOUND_VARIABLE)
        {
          TNode variable = (l.getKind() == Kind::BOUND_VARIABLE ? l : r);
          TNode term = (l.getKind() == Kind::BOUND_VARIABLE ? r : l);

          if (!expr::hasSubterm(term, variable))
          {
            return {0, 0};
          }
          else
          {
            if (term.hasOperator())
            {
              Node op = term.getOperator();
              size_t listSize = indices.getReps(op).size();
              return {listSize == 0 ? 0 : 1, listSize};
            }
            else
            {
              return {0, 0};
            }
          }
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
          size_t listSize = indices.getApps(op, t).size();
          return {listSize == 0 ? 0 : 1, listSize};
        }

        // E-matching
        // f(u_n) = g(u'_n) or g(u'_n) = f(u_n)
        if (l.hasOperator() && r.hasOperator())
        {
          size_t listSize = indices.getReps(l.getOperator()).size();
          if (l.getOperator() == r.getOperator())
          {
            return {1, listSize + 1};
          }
          return {listSize == 0 ? 0 : 1, listSize};
        }

        return {0, 0};
      }
      case Kind::NOT:
      {
        if (lit[0].getKind() != Kind::EQUAL) return {0, 0};
        Node eq = lit[0];
        Node l = eq[0], r = eq[1];

        // lit != lit
        if (l == r)
        {
          return {0, 0};
        }

        // Ground disequality
        // t1 != t2
        bool lIsGround = !expr::hasBoundVar(l),
             rIsGround = !expr::hasBoundVar(r);
        if (lIsGround && rIsGround)
        {
          if (ee->hasTerm(l) && ee->hasTerm(r))
          {
            return {0, 0};
          }
        }

        // An empty candidate list makes the branch fail immediately, so such
        // literals get level 0 (they should be processed first)

        // Assignment into a disequal term
        // x != t or t != x
        if ((l.getKind() == Kind::BOUND_VARIABLE && rIsGround && ee->hasTerm(r))
            || (r.getKind() == Kind::BOUND_VARIABLE && lIsGround
                && ee->hasTerm(l)))
        {
          TNode term = (l.getKind() == Kind::BOUND_VARIABLE ? r : l);
          size_t listSize = indices.getDiseqs(term).size();
          return {listSize == 0 ? 0 : 2, listSize};
        }

        // E-matching
        // f(u_n) != t or t != f(u_n)
        if ((!ee->hasTerm(l) && l.hasOperator() && rIsGround && ee->hasTerm(r))
            || (!ee->hasTerm(r) && r.hasOperator() && lIsGround
                && ee->hasTerm(l)))
        {
          TNode t = ee->hasTerm(r) ? r : l;
          size_t listSize = indices.getDiseqs(t).size();
          return {listSize == 0 ? 0 : 2, listSize};
        }

        // DFapp
        // x != f(u_n) or f(u_n) != x
        if ((l.getKind() == Kind::BOUND_VARIABLE && r.hasOperator())
            || (l.hasOperator() && r.getKind() == Kind::BOUND_VARIABLE))
        {
          TNode term = (l.getKind() == Kind::BOUND_VARIABLE ? r : l);
          size_t listSize = indices.getReps(term.getOperator()).size();
          return {listSize == 0 ? 0 : 2, listSize};
        }

        // E-matching
        // f(u_n) != g(u'_n) or g(u'_n) != f(u_n)
        if (l.hasOperator() && r.hasOperator())
        {
          size_t listSize = indices.getReps(l.getOperator()).size();
          return {listSize == 0 ? 0 : 2, listSize};
        }

        // DVar
        // x != y
        if (l.getKind() == Kind::BOUND_VARIABLE
            && r.getKind() == Kind::BOUND_VARIABLE)
        {
          size_t lSize = indices.getRepsOfType(l.getType()).size();
          size_t rSize = indices.getRepsOfType(r.getType()).size();
          size_t listSize = lSize > rSize ? lSize : rSize;
          return {listSize == 0 ? 0 : 2, listSize};
        }

        return {0, 0};
      }
      default: return {0, 0};
    }
  };

  size_t best = lits.size() - 1;
  int bestLevel = 3;
  size_t bestCost = SIZE_MAX;

  for (size_t i = lits.size(); i > 0; --i)
  {
    auto [level, cost] = classify(lits[i - 1]);

    if (level == 0)
    {
      return i - 1;
    }
    if (level < bestLevel || (level == bestLevel && cost < bestCost))
    {
      best = i - 1;
      bestLevel = level;
      bestCost = cost;
    }
  }

  return best;
}

std::vector<std::vector<Node>> CcfvBacktrackingSolver::solveRec(
    const std::vector<Node>& freeVars,
    std::vector<Node> lits,
    eq::EqualityEngine* ee,
    const ccfv::CcfvIndices& indices,
    std::vector<Node> currentSubstitution,
    size_t depth)
{
  ++d_lastNodes;
  if (lits.empty())
  {
    ++d_stats.d_solutions;
    d_stats.d_leafDepth << static_cast<double>(depth);
    d_stats.d_maxDepth.maxAssign(depth);
    return {currentSubstitution};
  }

  // Smart literal selection
  size_t pick = selectLiteral(lits, ee, indices);
  std::swap(lits[pick], lits.back());
  Node c = lits.back();
  lits.pop_back();
  std::vector<Node>& newLits = lits;

  std::vector<std::vector<Node>> sols;

  Trace("ccfv-debug") << "[backtracking]   L " << lits << std::endl;
  Trace("ccfv-debug") << "[backtracking]   Processing " << c << std::endl;
  Trace("ccfv-debug") << "[backtracking]   Remaining L " << newLits
                      << std::endl;

  switch (c.getKind())
  {
    case Kind::EQUAL:
    {
      TNode l = c[0], r = c[1];

      // Syntactic equality
      // s = s
      if (l == r)
      {
        Trace("ccfv-debug") << "[backtracking]     syntactic equality (" << l
                            << " = " << r << ")" << std::endl;
        Trace("ccfv-debug")
            << "[backtracking]     removing from C" << std::endl;
        return solveRec(freeVars, newLits, ee, indices, currentSubstitution, depth + 1);
      }

      // Ground equality
      // t1 = t2
      bool lIsGround = !expr::hasBoundVar(l), rIsGround = !expr::hasBoundVar(r);
      if (lIsGround && rIsGround)
      {
        Trace("ccfv-debug") << "[backtracking]     ground equality (" << l
                            << " = " << r << ")" << std::endl;
        if (ee->hasTerm(l) && ee->hasTerm(r))
        {
          if (ee->areEqual(l, r))
          {
            Trace("ccfv-debug")
                << "[backtracking]       they are equal, removing from C"
                << std::endl;
            return solveRec(
                freeVars, newLits, ee, indices, currentSubstitution, depth + 1);
          }
          return fail(depth, d_stats.d_failGroundEq);
        }
        else if (l.hasOperator() && r.hasOperator()
                 && l.getOperator() == r.getOperator()
                 && l.getNumChildren() == r.getNumChildren())
        {
          Trace("ccfv-debug")
              << "[backtracking]       they are UF with the same operator"
              << std::endl;
          for (size_t i = 0, n = l.getNumChildren(); i < n; ++i)
          {
            Trace("ccfv-debug") << "[backtracking]       adding "
                                << l[i].eqNode(r[i]) << "to C" << std::endl;
            newLits.push_back(l[i].eqNode(r[i]));
          }
          return solveRec(freeVars, newLits, ee, indices, currentSubstitution, depth + 1);
        }
      }

      // Assignmnet
      // x = s or s = x
      if (l.getKind() == Kind::BOUND_VARIABLE
          || r.getKind() == Kind::BOUND_VARIABLE)
      {
        TNode variable = (l.getKind() == Kind::BOUND_VARIABLE ? l : r);
        TNode term = (l.getKind() == Kind::BOUND_VARIABLE ? r : l);

        Trace("ccfv-debug") << "[backtracking]     assignment (" << variable
                            << " = " << term << ")" << std::endl;

        if (!expr::hasSubterm(term, variable))
        {
          Trace("ccfv-debug")
              << "[backtracking]       adding " << variable << " = " << term
              << " to the solution" << std::endl;
          for (Node& t : currentSubstitution)
          {
            t = t.substitute(TNode(variable), TNode(term));
          }
          Trace("ccfv-debug") << "[backtracking]       applying (" << variable
                              << " mapsto " << term << ") to terms that have "
                              << variable << " in " << newLits << std::endl;
          for (Node& lit : newLits)
          {
            lit = lit.substitute(TNode(variable), TNode(term));
          }
          return solveRec(freeVars, newLits, ee, indices, currentSubstitution, depth + 1);
        }
        else
        {
          if (term.hasOperator())
          {
            Node op = term.getOperator();
            Trace("ccfv-debug")
                << "[backtracking]       Uvar (" << variable << " occurs in "
                << term << " with op = " << op << ")" << std::endl;
            const auto& cands = indices.getReps(op);
            if (cands.empty())
            {
              return fail(depth, d_stats.d_failNoCandidates);
            }
            for (const Node& t : cands)
            {
              Trace("ccfv-debug")
                  << "[backtracking]       adding " << term.eqNode(t) << " and "
                  << variable.eqNode(t) << " to C" << std::endl;
              std::vector<Node> branchLits = newLits;
              branchLits.push_back(term.eqNode(t));
              branchLits.push_back(variable.eqNode(t));
              std::vector<std::vector<Node>> branchSols = solveRec(
                  freeVars, branchLits, ee, indices, currentSubstitution, depth + 1);
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

        Trace("ccfv-debug")
            << "[backtracking]     E-Matching UF with ground term (" << fTerm
            << " = " << t << ")" << std::endl;

        Trace("ccfv-debug")
            << "[backtracking]       E-Matching all UF terms with op = " << op
            << " in the equivalence class of " << t << std::endl;
        const auto& cands = indices.getApps(op, t);
        if (cands.empty())
        {
          return fail(depth, d_stats.d_failNoCandidates);
        }
        for (const Node& app : cands)
        {
          Assert(fTerm.getNumChildren() == app.getNumChildren());

          Trace("ccfv-debug")
              << "[backtracking]       E-Matching " << app << " with " << t
              << " and adding it to C" << std::endl;
          std::vector<Node> branchLits = newLits;
          for (size_t i = 0, n = fTerm.getNumChildren(); i < n; ++i)
          {
            Trace("ccfv-debug")
                << "[backtracking]       adding " << fTerm[i].eqNode(app[i])
                << " to C" << std::endl;
            branchLits.push_back(fTerm[i].eqNode(app[i]));
          }
          std::vector<std::vector<Node>> branchSols =
              solveRec(freeVars, branchLits, ee, indices, currentSubstitution, depth + 1);
          sols.insert(sols.end(), branchSols.begin(), branchSols.end());
        }

        return sols;
      }

      // E-matching
      // f(u_n) = g(u'_n) or g(u'_n) = f(u_n)
      if (l.hasOperator() && r.hasOperator())
      {
        Trace("ccfv-debug") << "[backtracking]     E-Matching with UF (" << l
                            << " = " << r << ")" << std::endl;

        if (l.getOperator() == r.getOperator())
        {
          Assert(l.getNumChildren() == r.getNumChildren());
          Trace("ccfv-debug")
              << "[backtracking]       both UF has the same operator"
              << std::endl;

          std::vector<Node> branchLits = newLits;
          for (size_t i = 0, n = l.getNumChildren(); i < n; ++i)
          {
            Trace("ccfv-debug") << "[backtracking]       unifying arguments: "
                                << l[i].eqNode(r[i]) << std::endl;
            branchLits.push_back(l[i].eqNode(r[i]));
          }
          std::vector<std::vector<Node>> branchSols =
              solveRec(freeVars, branchLits, ee, indices, currentSubstitution, depth + 1);
          sols.insert(sols.end(), branchSols.begin(), branchSols.end());
        }

        const auto& cands = indices.getReps(l.getOperator());
        if (cands.empty() && l.getOperator() != r.getOperator())
        {
          return fail(depth, d_stats.d_failNoCandidates);
        }
        for (const Node& t : cands)
        {
          std::vector<Node> branchLits = newLits;
          Trace("ccfv-debug")
              << "[backtracking]       E-Matching them with ground "
                 "terms within the same "
                 "equivalence class "
              << l.eqNode(t) << " and " << r.eqNode(t) << std::endl;
          branchLits.push_back(l.eqNode(t));
          branchLits.push_back(r.eqNode(t));
          std::vector<std::vector<Node>> branchSols =
              solveRec(freeVars, branchLits, ee, indices, currentSubstitution, depth + 1);
          sols.insert(sols.end(), branchSols.begin(), branchSols.end());
        }

        return sols;
      }

      // No rule applies
      return fail(depth,
                  lIsGround && rIsGround ? d_stats.d_failNotInEe
                                         : d_stats.d_failUnsupported);
    }
    case Kind::NOT:
    {
      if (c[0].getKind() != Kind::EQUAL)
      {
        return fail(depth, d_stats.d_failUnsupported);
      }

      Node eq = c[0];
      Node l = eq[0], r = eq[1];

      if (l == r)
      {
        Trace("ccfv-debug") << "[backtracking]     syntactic disequality (" << l
                            << " != " << r << "), failing branch" << std::endl;
        return fail(depth, d_stats.d_failDiseq);
      }

      // Ground disequality
      // t1 != t2
      bool lIsGround = !expr::hasBoundVar(l), rIsGround = !expr::hasBoundVar(r);
      if (lIsGround && rIsGround)
      {
        Trace("ccfv-debug") << "[backtracking]     ground disequality (" << l
                            << " != " << r << ")" << std::endl;
        if (ee->hasTerm(l) && ee->hasTerm(r))
        {
          if (!ee->areDisequal(l, r, false))
          {
            Trace("ccfv-debug")
                << "[backtracking]       they are not disequal, failing branch"
                << std::endl;
            return fail(depth, d_stats.d_failDiseq);
          }
          Trace("ccfv-debug")
              << "[backtracking]       they are disequal, removing from C"
              << std::endl;
          return solveRec(freeVars, newLits, ee, indices, currentSubstitution, depth + 1);
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

        Trace("ccfv-debug")
            << "[backtracking]     assignment into a disequal term ("
            << variable << " != " << term << ")" << std::endl;

        const auto& cands = indices.getDiseqs(term);
        if (cands.empty())
        {
          return fail(depth, d_stats.d_failNoCandidates);
        }
        for (const Node& t : cands)
        {
          Trace("ccfv-debug") << "[backtracking]       assigning " << variable
                              << " to disequal rep " << t << " ("
                              << variable.eqNode(t) << ")" << std::endl;
          std::vector<Node> branchLits = newLits;
          branchLits.push_back(variable.eqNode(t));
          std::vector<std::vector<Node>> branchSols =
              solveRec(freeVars, branchLits, ee, indices, currentSubstitution, depth + 1);
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

        Trace("ccfv-debug")
            << "[backtracking]     E-Matching UF with disequal ground "
               "term ("
            << fTerm << " != " << t << ")" << std::endl;

        const auto& cands = indices.getDiseqs(t);
        if (cands.empty())
        {
          return fail(depth, d_stats.d_failNoCandidates);
        }
        for (const Node& tt : cands)
        {
          Trace("ccfv-debug") << "[backtracking]       E-Matching " << fTerm
                              << " with disequal rep " << tt << " and adding "
                              << fTerm.eqNode(tt) << " to C" << std::endl;
          std::vector<Node> branchLits = newLits;
          branchLits.push_back(fTerm.eqNode(tt));
          std::vector<std::vector<Node>> branchSols =
              solveRec(freeVars, branchLits, ee, indices, currentSubstitution, depth + 1);
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

        Trace("ccfv-debug") << "[backtracking]     DFapp (" << variable
                            << " != " << term << ")" << std::endl;

        const auto& cands = indices.getReps(term.getOperator());
        if (cands.empty())
        {
          return fail(depth, d_stats.d_failNoCandidates);
        }
        for (const Node& t : cands)
        {
          Trace("ccfv-debug")
              << "[backtracking]       adding " << term.eqNode(t) << " and "
              << variable.eqNode(t).negate() << " to C" << std::endl;
          std::vector<Node> branchLits = newLits;
          branchLits.push_back(term.eqNode(t));
          branchLits.push_back(variable.eqNode(t).negate());
          std::vector<std::vector<Node>> branchSols =
              solveRec(freeVars, branchLits, ee, indices, currentSubstitution, depth + 1);
          sols.insert(sols.end(), branchSols.begin(), branchSols.end());
        }

        return sols;
      }

      // E-matching
      // f(u_n) != g(u'_n) or g(u'_n) != f(u_n)
      if (l.hasOperator() && r.hasOperator())
      {
        Trace("ccfv-debug")
            << "[backtracking]     E-Matching into disequal terms (" << l
            << " != " << r << ")" << std::endl;
        const auto& cands = indices.getReps(l.getOperator());
        if (cands.empty())
        {
          return fail(depth, d_stats.d_failNoCandidates);
        }
        for (const Node& t : cands)
        {
          Trace("ccfv-debug")
              << "[backtracking]       adding " << l.eqNode(t) << " and "
              << r.eqNode(t).negate() << " to C" << std::endl;
          std::vector<Node> branchLits = newLits;
          branchLits.push_back(l.eqNode(t));
          branchLits.push_back(r.eqNode(t).negate());
          std::vector<std::vector<Node>> branchSols =
              solveRec(freeVars, branchLits, ee, indices, currentSubstitution, depth + 1);
          sols.insert(sols.end(), branchSols.begin(), branchSols.end());
        }

        return sols;
      }

      // DVar
      // x != y
      if (l.getKind() == Kind::BOUND_VARIABLE
          && r.getKind() == Kind::BOUND_VARIABLE)
      {
        Trace("ccfv-debug") << "[backtracking]     DVar (" << l << " != " << r
                            << ")" << std::endl;

        const auto& cands = indices.getRepsOfType(l.getType());
        if (cands.empty())
        {
          return fail(depth, d_stats.d_failNoCandidates);
        }
        for (TNode rep : cands)
        {
          for (const Node& t : indices.getDiseqs(rep))
          {
            Trace("ccfv-debug")
                << "[backtracking]       adding " << l.eqNode(rep) << " and "
                << r.eqNode(t) << " to C" << std::endl;
            std::vector<Node> branchLits = newLits;
            branchLits.push_back(l.eqNode(rep));
            branchLits.push_back(r.eqNode(t));
            std::vector<std::vector<Node>> branchSols = solveRec(
                freeVars, branchLits, ee, indices, currentSubstitution, depth + 1);
            sols.insert(sols.end(), branchSols.begin(), branchSols.end());
          }
        }

        return sols;
      }

      // No rule applies
      return fail(depth,
                  lIsGround && rIsGround ? d_stats.d_failNotInEe
                                         : d_stats.d_failUnsupported);
    }
    default: return fail(depth, d_stats.d_failUnsupported);
  }

  return sols;
}

bool CcfvBacktrackingSolver::solve(
    const std::vector<Node>& freeVars,
    const std::vector<Node>& lits,
    eq::EqualityEngine* ee,
    const ccfv::CcfvIndices& indices,
    std::vector<std::vector<Node>>& substitutions)
{
  // Predicate literals are rewritten once, so the recursion only deals with
  // (dis)equalities
  std::vector<Node> normalizedLits;
  normalizedLits.reserve(lits.size());
  for (const Node& lit : lits)
  {
    normalizedLits.push_back(normalizePredicate(lit));
  }

  std::vector<Node> currentSubstitution(freeVars.begin(), freeVars.end());
  d_lastNodes = 0;
  substitutions = solveRec(freeVars,
                           std::move(normalizedLits),
                           ee,
                           indices,
                           currentSubstitution,
                           0);

  // If any free variable is left
  for (std::vector<Node>& sub : substitutions)
  {
    for (size_t i = 0; i < freeVars.size(); ++i)
    {
      if (freeVars[i] == sub[i])
      {
        Node groundTerm = indices.fallbackForType(freeVars[i].getType());

        Trace("ccfv-debug")
            << "[backtracking]   Grounding remaining free variable "
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

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
