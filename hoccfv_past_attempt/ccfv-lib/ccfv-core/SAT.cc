#include "SAT.hh"

namespace CCFV_SAT {

Csolver::Csolver(void)
{
  var_max = 0;
}

Csolver::~Csolver(void)
{

}

void
Csolver::add(Tlit lit_or_zero)
{
  if (lit_var(lit_or_zero) > var_max)
    var_max = lit_var(lit_or_zero);
  solver.add((lit_or_zero & 1)?(lit_or_zero>>1):-(lit_or_zero>>1));
#ifdef DEBUG
  static bool newclause = true;
  if (newclause)
    {
      debug_CNF.resize(debug_CNF.size() + 1);
      newclause = false;
    }
  if (lit_or_zero)
    debug_CNF.back().push_back(lit_or_zero);
  else
    newclause = true;
#endif
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
  int res = solver.solve();
  if (res == 10) {
  model.resize(0);
  for (unsigned i = 1; i < var_max; i++)
    if (solver.val(i) > 0)
      model.push_back(i);
  }
  return res;
}

}
