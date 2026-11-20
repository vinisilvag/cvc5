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
    return d_backtrackingSolver.solve(
        freeVars, lits, ee, indices, substitutions);
  }
  else if (mode == options::CcfvMode::SAT)
  {
    Trace("ccfv-solver") << "  Dispatching to: SAT Encoding backend"
                         << std::endl;
    return d_satSolver.solve(freeVars, lits, ee, substitutions);
  }
  return false;
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
