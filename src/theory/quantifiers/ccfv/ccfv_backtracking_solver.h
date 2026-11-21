/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Backtracking CCFV solver for E-ground (dis)unification.
 */

#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_BACKTRACKING_SOLVER_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_BACKTRACKING_SOLVER_H

#include <vector>

#include "expr/node.h"
#include "smt/env_obj.h"
#include "theory/quantifiers/ccfv/ccfv_indices.h"
#include "util/statistics_registry.h"

namespace cvc5::internal {
namespace theory {

namespace eq {
class EqualityEngine;
}

namespace quantifiers {

/**
 * Backtracking CCFV solver (depth-first search over the E-graph).
 *
 * Given a set of equational literals L containing free variables and an
 * equality engine E, finds substitutions sigma such that E |= L sigma.
 */
class CcfvBacktrackingSolver : protected EnvObj
{
 public:
  CcfvBacktrackingSolver(Env& env);
  ~CcfvBacktrackingSolver() = default;

  /**
   * @param freeVars The free variables in L (the quantified variables).
   * @param lits The set of equational literals L to satisfy.
   * @param ee The equality engine representing the ground context E.
   * @param indices Indices over E.
   * @param substitutions Output: solutions, index-by-index with `freeVars`.
   * @return True if at least one solution was found, false otherwise.
   */
  bool solve(const std::vector<Node>& freeVars,
             const std::vector<Node>& lits,
             eq::EqualityEngine* ee,
             const ccfv::CcfvIndices& indices,
             std::vector<std::vector<Node>>& substitutions);

  /** Number of search nodes (solveRec calls) of the last call to solve. */
  uint64_t getLastNodeCount() const { return d_lastNodes; }

 private:
  struct Statistics
  {
    Statistics(StatisticsRegistry& sr);
    /** Number of solutions found (complete branches). */
    IntStat d_solutions;
    /** Deepest recursion reached (literals processed along a path). */
    IntStat d_maxDepth;
    /** Depth of every leaf (a solution or a failed branch). */
    AverageStat d_leafDepth;
    /** Branches failed on a literal outside the calculus or with no rule. */
    IntStat d_failUnsupported;
    /** Branches failed on an empty candidate list (reps/apps/diseqs). */
    IntStat d_failNoCandidates;
    /** Branches failed on a ground equality that does not hold in E. */
    IntStat d_failGroundEq;
    /** Branches failed on a violated disequality (s != s, or ground t1 != t2
     * that is not disequal in E). */
    IntStat d_failDiseq;
    /** Branches failed on ground terms missing from E with no rule left. */
    IntStat d_failNotInEe;
  };
  Statistics d_stats;

  /** Search nodes of the current/last call to solve. */
  uint64_t d_lastNodes = 0;

  /** Records a failed leaf at `depth` under `reason`; returns no solutions. */
  std::vector<std::vector<Node>> fail(size_t depth, IntStat& reason);

  size_t selectLiteral(const std::vector<Node>& lits,
                       eq::EqualityEngine* ee,
                       const ccfv::CcfvIndices& indices);

  std::vector<std::vector<Node>> solveRec(
      const std::vector<Node>& freeVars,
      std::vector<Node> lits,
      eq::EqualityEngine* ee,
      const ccfv::CcfvIndices& indices,
      std::vector<Node> currentSubstitution,
      size_t depth);

  /**
   * Rewrites predicate literals P(t) and (not P(t)) as equalities
   * P(t) = true and P(t) = false, so that the solver only sees
   * (dis)equalities.
   */
  Node normalizePredicate(TNode lit);
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
