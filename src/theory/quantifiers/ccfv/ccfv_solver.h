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
 * Variables (CCFV). Supports both:
 * 1. Decision procedure (backtracking/Breadth-first search over the E-graph)
 * 2. SAT-encoded procedure (reduction to SAT via propositional encoding)
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
   * @param vars The free variables in L (the quantified variables).
   * @param lits The set of equational literals L to satisfy.
   * @param ee The equality engine representing the ground context E.
   * @param substitutions Output: vector of solutions, where each solution is a
   *                  vector of terms corresponding index-by-index to `vars`.
   * @return True if at least one solution was found, false otherwise.
   */
  bool solve(options::CcfvMode mode,
             const std::vector<Node>& vars,
             const std::vector<Node>& lits,
             eq::EqualityEngine* ee,
             std::vector<std::vector<Node>>& substitutions);

 private:
  /** Solves using the decision procedure. */
  bool solveProcedural(const std::vector<Node>& vars,
                       const std::vector<Node>& lits,
                       eq::EqualityEngine* ee,
                       std::vector<std::vector<Node>>& solutions);

  /** Solves using the SAT encoding procedure. */
  bool solveSat(const std::vector<Node>& vars,
                const std::vector<Node>& lits,
                eq::EqualityEngine* ee,
                std::vector<std::vector<Node>>& solutions);
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
