/*
  --------------------------------------------------------------
  CCFV pre- and post-processing module
  --------------------------------------------------------------
*/

#include <algorithm>
#include <cassert>
#include <climits>
#include <iostream>
#include <vector>

#include "ccfv-full.hh"
#include "general.h"
extern "C" {
#include "CC.h"
}

CCCFV_wrapper::CCCFV_wrapper(std::vector<bool> vars)
{
  // CC_done must be called once the CCFV_preprocessor is destructed
  Ecc = CC_init();


  terms_bound = vars.size() - 1; //because cell numbers start at 0
  next_new_index = terms_bound + 1;

  // add init of terms_store when default Tterm can be created
  // terms_store.resize(max_term, (Tterm) 0);

  outer_substitution.resize(terms_bound + 1, (TCCFVterm) 0);

  terms_info.resize(terms_bound + 1);
  //to init using vars TODO

  var_occurrences.resize(terms_bound + 1);
}

CCCFV_wrapper::~CCCFV_wrapper(void) { CC_done(Ecc); }

bool CCCFV_wrapper::is_var(TCCFVterm t) { return terms_info[t].is_var; }
bool CCCFV_wrapper::is_ground(TCCFVterm t) { return terms_info[t].is_ground; }
bool CCCFV_wrapper::is_new(TCCFVterm t) { return terms_bound < t; }

void CCCFV_wrapper::add_lit_to_CC(Tliteral e)
{
  add_term_to_CC(e.lhs);
  add_term_to_CC(e.rhs);
  if (e.polarity) {
    CC_assert_eq(Ecc, (TCC_term)e.lhs.index, (TCC_term)e.rhs.index,
                 (TCC_label)UINT_MAX);
  } else {
    CC_assert_neq(Ecc, (TCC_term)e.lhs.index, (TCC_term)e.rhs.index,
                  (TCC_label)UINT_MAX);
  }
}

void CCCFV_wrapper::add_term_to_CC(Tterm t)
{
  if (t.symb() != 0) {
    CC_build_term(Ecc, t, 0, NULL);
  } else {
    add_term_to_CC(t[0]);
    add_term_to_CC(t[1]);
    TCC_term subterms[2] = {(TCC_term)t[0], (TCC_term)t[1]};
    CC_build_term(Ecc, t, 2, subterms);

    terms_store[t[0]] = t[0];
    terms_info[t[0]].is_ground = true;
    terms_store[t[1]] = t[1];
    terms_info[t[1]].is_ground = true;
  }

  terms_store[t] = t;
  terms_info[t].is_ground = true;
}


bool CCCFV_wrapper::add_lit_to_L(Tliteral l)
{
  add_ground_subterms_to_CC(l.lhs);
  add_ground_subterms_to_CC(l.rhs);
  if ((is_ground((TCCFVterm) l.lhs) && is_ground((TCCFVterm) l.rhs))) {
    // if the polarity is positive, lhs and rhs must belong to the same class
    // otherwise, they must belong to different classes
    return (l.polarity == (CC_get_class(Ecc, l.lhs) == CC_get_class(Ecc, l.rhs)));
  }

  //direct flattening into list of pairs or triplets?
  //or simple inclusion to L_pre?

  TCCFVlit l_cp;
  l_cp.polarity = l.polarity;
  std::vector<TCCFVterm> terms;
  terms.push_back(l.lhs);
  terms.push_back(l.rhs);
  l_cp.terms = terms;
  L_pre.push_back(l_cp);
  return true;
}


void CCCFV_wrapper::add_ground_subterms_to_CC(Tterm t)
{
  terms_store[t] = t;

  if (t.symb() != 0 && !(is_var(t))) {
      terms_info[t].is_ground = true;
      CC_build_term(Ecc, (TCC_term) t, 0, NULL);
  } else if (t.symb() == 0) {
    terms_store[t[0]] = t[0];
    terms_store[t[1]] = t[1];

    add_ground_subterms_to_CC(t[0]);
    add_ground_subterms_to_CC(t[1]);

    if (is_ground(t[0]) && is_ground(t[1])) {
      terms_info[t].is_ground = true;
      TCC_term subterms[2] = {(TCC_term) t[0], (TCC_term) t[1]};
      CC_build_term(Ecc, (TCC_term) t, 2, subterms);
    }
  }
}

void CCCFV_wrapper::preprocess()
{


}

void CCCFV_wrapper::init_solver()
{
  for (unsigned i = 0; i < terms_bound + 1; i++) {
    Tterm t = terms_store[i];
    core_solver.debug_symbol(i, (char *) t.index); //TODO: it should be t.symb() instead of t.index, but the cast doesn't work then
    if (is_var(i)) { core_solver.class_is_var(i); }
  }

  for (unsigned i = terms_bound; i < next_new_index; i++) {
    std::string symb = "@ccfv" + std::to_string(i);
    // TODO? the debug symbol could indicate if this is a constant or a variable
    core_solver.debug_symbol(i, symb);
    if (is_var(i)) {
      core_solver.class_is_var(i);
    }
  }

  TCC_term *classes = NULL;
  unsigned n;
  CC_get_all_classes(Ecc, &n, &classes);

  for (unsigned i = 0; i < n; i++) {
    TCC_term *sigs = NULL;
    unsigned m;
    CC_get_signatures(Ecc, classes[i], &m, &sigs);
    //assumption: the terms returned in sigs really are of arity 2 and
    //representatives of the different signatures in the class

    for (unsigned k = 0; k < m; k++) {
      Tterm t_sig = terms_store[sigs[k]];
      core_solver.class_sig(classes[i], t_sig[0], t_sig[1]);
    }

  }

  for (auto & l : L_pre) {
    if (l.polarity) {
      assert(l.terms.size() == 3);
      core_solver.constraint_pos(l.terms[0], l.terms[1], l.terms[2]);
    } else {
      assert(l.terms.size() == 2);
      core_solver.constraint_neg(l.terms[0], l.terms[1]);
    }
  }
}


std::vector<TCCFVterm> CCCFV_wrapper::compute_next_solution()
{

}
