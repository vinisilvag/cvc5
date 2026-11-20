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

 private:
  size_t selectLiteral(const std::vector<Node>& lits,
                       eq::EqualityEngine* ee,
                       const ccfv::CcfvIndices& indices);

  std::vector<std::vector<Node>> solveRec(
      const std::vector<Node>& freeVars,
      std::vector<Node> lits,
      eq::EqualityEngine* ee,
      const ccfv::CcfvIndices& indices,
      std::vector<Node> currentSubstitution);

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
