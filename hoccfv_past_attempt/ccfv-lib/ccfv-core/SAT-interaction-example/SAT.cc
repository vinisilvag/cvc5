#include "SAT.hh"

namespace CCFV_SAT {

Csolver::Csolver(void)
{
  solver = ipasir_init();
}

Csolver::~Csolver(void)
{
  ipasir_release(solver);
}

void
Csolver::add(Tlit lit_or_zero)
{
  ipasir_add(solver, (lit_or_zero & 1)?(lit_or_zero>>1):-(lit_or_zero>>1));
}

void
Csolver::add_clause(Tlit lit)
{
  add(lit); add(0);
}

void
Csolver::add_clause(Tlit lit0, Tlit lit1)
{
  add(lit0); add(lit1); add(0);
}

void
Csolver::add_clause(Tlit lit0, Tlit lit1, Tlit lit2)
{
  add(lit0); add(lit1); add(lit2); add(0);
}

int
Csolver::solve(void)
{
  int res = ipasir_solve(solver);
  for (unsigned i = 1; i < var_max; i++)
    if (ipasir_val(solver, i) > 0)
      model.push_back(i);
  return res;
}

}