/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Congruence Closure with Free Variables (CCFV) solver for E-ground
 * (dis)unification.
 */

#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_SOLVER_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_SOLVER_H

#include <vector>

#include "expr/node.h"
#include "options/quantifiers_options.h"
#include "smt/env_obj.h"

namespace cvc5::internal {
namespace theory {

namespace eq {
class EqualityEngine;
}

namespace quantifiers {

/**
 * Solver for E-ground (dis)unification using Congruence Closure with Free
 * Variables (CCFV).
 *
 * Given a set of equational literals L containing free variables and an
 * equality engine E (representing ground equalities/disequalities), CCFV solves
 * the problem of finding substitutions sigma such that E |= L sigma.
 *
 * Supports two backend solving modes:
 * 1. Procedural decision procedure (backtracking search over the E-graph)
 * 2. SAT-encoded procedure (reduction to propositional SAT)
 */
class CcfvSolver : protected EnvObj
{
 public:
  CcfvSolver(Env& env);
  ~CcfvSolver() = default;

  /**
   * Solves the E-ground (dis)unification problem: finds substitutions sigma
   * such that E |= L sigma.
   *
   * @param mode Whether to use the decision procedure or SAT encoding.
   * @param freeVars The free variables in L (the quantified variables).
   * @param lits The set of equational literals L to satisfy.
   * @param ee The equality engine representing the ground context E.
   * @param substitutions Output: vector of solutions, where each solution is a
   *                  vector of terms corresponding index-by-index to `vars`.
   * @return True if at least one solution was found, false otherwise.
   */
  bool solve(options::CcfvMode mode,
             const std::vector<Node>& freeVars,
             const std::vector<Node>& lits,
             eq::EqualityEngine* ee,
             std::vector<std::vector<Node>>& substitutions);

 private:
  std::vector<std::vector<Node>> solveProceduralRec(
      const std::vector<Node> lits,
      eq::EqualityEngine* ee,
      std::vector<std::vector<Node>> substitutions);

  /** Solves using the decision procedure. */
  bool solveProcedural(const std::vector<Node>& freeVars,
                       const std::vector<Node>& lits,
                       eq::EqualityEngine* ee,
                       std::vector<std::vector<Node>>& substitutions);

  /** TODO: add comments */
  std::vector<std::vector<Node>> solveProceduralRec();

  /** Solves using the SAT encoding procedure. */
  bool solveSat(const std::vector<Node>& freeVars,
                const std::vector<Node>& lits,
                eq::EqualityEngine* ee,
                std::vector<std::vector<Node>>& substitutions);
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
