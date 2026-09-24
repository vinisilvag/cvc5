#include "theory/quantifiers/ccfv/ccfv_solver.h"

#include "theory/uf/equality_engine.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

CcfvSolver::CcfvSolver(Env& env) : EnvObj(env) {}

bool CcfvSolver::solve(options::CcfvMode mode,
                       const std::vector<Node>& vars,
                       const std::vector<Node>& lits,
                       eq::EqualityEngine* ee,
                       std::vector<std::vector<Node>>& solutions)
{
  if (mode == options::CcfvMode::DECISION)
  {
    return solveProcedural(vars, lits, ee, solutions);
  }
  else if (mode == options::CcfvMode::SAT)
  {
    return solveSat(vars, lits, ee, solutions);
  }
  return false;
}

bool CcfvSolver::solveProcedural(const std::vector<Node>& vars,
                                 const std::vector<Node>& lits,
                                 eq::EqualityEngine* ee,
                                 std::vector<std::vector<Node>>& solutions)
{
  // TODO: Implement CCFV Decision Procedure
  return false;
}

bool CcfvSolver::solveSat(const std::vector<Node>& vars,
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
