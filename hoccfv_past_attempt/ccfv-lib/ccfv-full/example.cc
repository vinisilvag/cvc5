#include <iostream>

#include "ccfv-full.hh"

class Tterm_mgr term_mgr;

int example_2(void)
{
  Tsymb apply = term_mgr.mk_symb((char*)"@", 0);

  /*same simple example as in ccfv-core/example.cc */
  Tterm a = term_mgr.mk_term(term_mgr.mk_symb((char*)"a", 0));
  Tterm b = term_mgr.mk_term(term_mgr.mk_symb((char*)"b", 0));
  Tterm f = term_mgr.mk_term(term_mgr.mk_symb((char*)"f", 0));
  Tterm g = term_mgr.mk_term(term_mgr.mk_symb((char*)"g", 0));
  Tterm h = term_mgr.mk_term(term_mgr.mk_symb((char*)"h", 0));
  Tterm c = term_mgr.mk_term(term_mgr.mk_symb((char*)"c", 0));
  Tterm x = term_mgr.mk_term(term_mgr.mk_symb((char*)"x", 0));
  Tterm y = term_mgr.mk_term(term_mgr.mk_symb((char*)"y", 0));

  Tterm fa = term_mgr.mk_term(apply, {f, a});

  // TODO create literals for E, L
  // TODO init CCFV_wrapper
  // TODO insert E, then insert L into CCFV_wrapper

  // CCCFV_wrapper solver = CCCFV_wrapper();
  return 0;
}


int main(void)
{
  return example_2();
}
