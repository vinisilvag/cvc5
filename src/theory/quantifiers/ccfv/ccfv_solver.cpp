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
#include "theory/uf/equality_engine.h"

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

// TODO: check those empty returns
std::vector<std::vector<Node>> CcfvSolver::solveProceduralRec(
    const std::vector<Node> lits,
    eq::EqualityEngine* ee,
    std::vector<std::vector<Node>> substitutions)
{
  if (lits.empty())
  {
    return {substitutions};
  }

  // TODO: apply more sophisticated selection
  Node c = lits.front();
  std::vector<Node> newLits(lits.begin() + 1, lits.end());
  std::vector<std::vector<Node>> sols;

  Trace("ccfv") << "  CcfvEngine: Processing " << c << std::endl;
  Trace("ccfv") << "  CcfvEngine: Remaining L " << newLits << std::endl;

  switch (c.getKind())
  {
    case Kind::EQUAL:
    {
      Node l = c[0], r = c[1];

      // Syntactic equality
      if (l == r)
      {
        return solveProceduralRec(newLits, ee, substitutions);
      }

      // Ground equality

      // Assignmnet

      break;
    }
    case Kind::NOT:
    {
      if (c[0].getKind() != Kind::EQUAL) return {};

      Node eq = c[0];
      Node l = eq[0], r = eq[1];

      // Ground disequality

      // Assignment into a disequal term

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
  substitutions = solveProceduralRec(lits, ee, {});
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
