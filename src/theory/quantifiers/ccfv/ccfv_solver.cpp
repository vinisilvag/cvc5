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

bool CcfvSolver::solveProcedural(const std::vector<Node>& freeVars,
                                 const std::vector<Node>& lits,
                                 eq::EqualityEngine* ee,
                                 std::vector<std::vector<Node>>& substitutions)
{
  // TODO: Implement CCFV Decision Procedure
  return false;
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
