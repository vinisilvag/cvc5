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

#include <iostream>

#include "base/output.h"
#include "theory/quantifiers/ccfv/ccfv_indices.h"
#include "theory/uf/equality_engine.h"
#include "theory/uf/equality_engine_iterator.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

CcfvSolver::CcfvSolver(Env& env)
    : EnvObj(env), d_backtrackingSolver(env), d_satSolver(env)
{
}

bool CcfvSolver::solve(options::CcfvMode mode,
                       const std::vector<Node>& freeVars,
                       const std::vector<Node>& lits,
                       eq::EqualityEngine* ee,
                       const ccfv::CcfvIndices& indices,
                       std::vector<std::vector<Node>>& substitutions)
{
  Trace("ccfv-debug") << "[solver] free vars: " << freeVars << std::endl;
  Trace("ccfv-debug") << "[solver] L: " << lits << std::endl;

  bool solved = false;
  if (mode == options::CcfvMode::DECISION)
  {
    solved =
        d_backtrackingSolver.solve(freeVars, lits, ee, indices, substitutions);
    d_lastNodes = d_backtrackingSolver.getLastNodeCount();
  }
  else if (mode == options::CcfvMode::SAT)
  {
    solved = d_satSolver.solve(freeVars, lits, ee, substitutions);
    d_lastNodes = 0;
  }

  Trace("ccfv") << "[solver] "
                << (mode == options::CcfvMode::DECISION ? "backtracking"
                                                        : "sat")
                << ": " << freeVars.size() << " free vars, " << lits.size()
                << " literals, " << substitutions.size() << " solution(s)"
                << std::endl;
  return solved;
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
