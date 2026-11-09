/*
  --------------------------------------------------------------
  CCFV preprocessing module
  --------------------------------------------------------------
*/

#include <algorithm>
#include <cassert>
#include <climits>
#include <iostream>
#include <vector>

#include "ccfv_preproc.hh"
#include "general.h"
extern "C"{
#include "CC.h"
}


CCCFV_preprocessor::CCCFV_preprocessor(std::vector<bool> vars, unsigned max_index)
{
  // CC_done must be called once the CCFV_preprocessor is destructed
  Ecc = CC_init();
  L_pre = std::vector<TCC_literal>();
  is_var = vars;
  next_new_term = max_index + 1;
  index_to_term = std::vector<TCCFVterm>(max_index + 1, 0);
}


CCCFV_preprocessor::~CCCFV_preprocessor(void)
{
  CC_done(Ecc);
}


void CCCFV_preprocessor::E_update(Tliteral e)
{
  CC_add_term(e.lhs);
  CC_add_term(e.rhs);
  if(e.polarity){
    CC_assert_eq(Ecc, (TCC_term) e.lhs.index, (TCC_term) e.rhs.index, (TCC_label) UINT_MAX);
  }
  else {
    CC_assert_neq(Ecc, (TCC_term) e.lhs.index, (TCC_term) e.rhs.index, (TCC_label) UINT_MAX);
  }
}


bool CCCFV_preprocessor::L_update(Tliteral l) {
  TCC_term t0 = CC_add_ground_subterms_in_nonground_term_and_return_normalized(l.lhs);
  TCC_term t1 = CC_add_ground_subterms_in_nonground_term_and_return_normalized(l.rhs);
  if ((is_ground.find(t0) != is_ground.end() && is_ground.find(t1) != is_ground.end())) {
    if (l.polarity) {
      return (CC_get_class(Ecc, t0) == CC_get_class(Ecc, t1));
    } else {
      return (CC_get_class(Ecc, t0) != CC_get_class(Ecc, t1));
    }
  }
  TCC_literal l_norm;
  l_norm.polarity = l.polarity;
  l_norm.lhs =  t0;
  l_norm.rhs =  t1;
  L_pre.push_back(l_norm);
  return true;
}

std::vector<TCC_literal> CCCFV_preprocessor::flatten_literal(TCC_literal l) {
  std::vector<TCC_literal> flat_literals = std::vector<TCC_literal>();
  assert(!(is_ground.find(l.lhs) != is_ground.end() && is_ground.find(l.rhs) != is_ground.end()));

  if (is_var[l.lhs] && (is_var[l.rhs] || is_ground.find(l.rhs) != is_ground.end())) {
    flat_literals.push_back(l);
  }
  else if (is_ground.find(l.lhs) != is_ground.end() && is_var[l.rhs]) {
    flat_literals.push_back(swap_sides(l));
  }
  else if (! l.polarity) {
    TCCterm_litlist tll = flatten_term(l.lhs);
    TCCterm_litlist tlr = flatten_term(l.rhs);
    TCC_literal l_flat;
    l_flat.polarity = l.polarity;
    if (is_var[tll.t]) {
      l_flat.lhs = tll.t;
      l_flat.rhs = tlr.t;
    } else {
      l_flat.lhs = tlr.t;
      l_flat.rhs = tll.t;
    };
    flat_literals.push_back(l_flat);
    flat_literals.insert(flat_literals.end(), tll.l_list.begin(), tll.l_list.end());
    flat_literals.insert(flat_literals.end(), tlr.l_list.begin(), tlr.l_list.end());
  }
  else {
    if (is_ground.find(l.lhs) != is_ground.end() || is_var[l.lhs]) {
      TCCterm_litlist tlr = flatten_subterms(l.rhs);
      TCC_literal l_flat;
      l_flat.polarity = l.polarity;
      l_flat.lhs = l.lhs;
      l_flat.rhs = tlr.t;
      flat_literals.push_back(l_flat);
      flat_literals.insert(flat_literals.end(), tlr.l_list.begin(), tlr.l_list.end());
    } else if (is_ground.find(l.rhs) != is_ground.end() || is_var[l.rhs]) {
      TCCterm_litlist tll = flatten_subterms(l.lhs);
      TCC_literal l_flat;
      l_flat.polarity = l.polarity;
      l_flat.lhs = l.rhs;
      l_flat.rhs = tll.t;
      flat_literals.push_back(l_flat);
      flat_literals.insert(flat_literals.end(), tll.l_list.begin(), tll.l_list.end());
    } else {
      TCCFVterm v_fresh = get_fresh_variable();

      TCCterm_litlist tlr = flatten_subterms(l.rhs);
      TCC_literal l_flat;
      l_flat.polarity = l.polarity;
      l_flat.lhs = (TCC_term)v_fresh.index;
      l_flat.rhs = tlr.t;
      flat_literals.push_back(l_flat);
      flat_literals.insert(flat_literals.end(), tlr.l_list.begin(),
                           tlr.l_list.end());
    };
  }
  return flat_literals;
}


TCCterm_litlist CCCFV_preprocessor::flatten_term(TCC_term t) {
  if (is_ground.find(t) != is_ground.end() || is_var[t]) {
    TCCterm_litlist res;
    res.t = t;
    res.l_list = std::vector<TCC_literal>();
    return res;
  } else {
    TCCFVterm v_fresh = get_fresh_variable();

    TCC_literal l;
    l.polarity = true;
    l.lhs = (TCC_term) v_fresh.index;
    l.rhs = t;

    TCCterm_litlist res;
    res.t = (TCC_term) v_fresh.index;
    res.l_list = flatten_literal(l);
    return res;
  }
}

TCCterm_litlist CCCFV_preprocessor::flatten_subterms(TCC_term t) {
  TCCFVterm tt = index_to_term[t];
  assert(tt.arity() == 2);
  assert(!(is_ground.find(tt.index) != is_ground.end()));

  TCC_term t0 = (TCC_term) tt[0].index;
  TCC_term t1 = (TCC_term)tt[1].index;
  TCCterm_litlist tl0 = flatten_term(t0);
  TCCterm_litlist tl1 = flatten_term(t1);

  TCCFVterm tt_flat = get_applied_term(index_to_term[tl0.t], index_to_term[tl1.t]);
  index_to_term[tt_flat.index] = tt_flat;

  TCCterm_litlist res;
  res.t = (TCC_term) tt_flat.index;
  res.l_list = std::vector<TCC_literal>();
  res.l_list.insert(res.l_list.end(), tl0.l_list.begin(), tl0.l_list.end());
  res.l_list.insert(res.l_list.end(), tl1.l_list.begin(), tl1.l_list.end());
  return res;
}

TCCFVterm CCCFV_preprocessor::get_applied_term(TCCFVterm t1, TCCFVterm t2) {
  // TODO: maybe see if the applied term already exists as a Tterm first instead of using next_new_term?
  TCCFVterm t(next_new_term++);
  index_to_term[t.index] = t;

  // /!\ TODO: fill the term to make it an applied term

  return t;
}

TCCFVterm CCCFV_preprocessor::get_fresh_variable(void) {
  assert(index_to_term.size() == next_new_term);
  TCCFVterm v_fresh(next_new_term++);
  index_to_term.emplace_back(v_fresh);
  is_var[v_fresh.index] = true;
}


void CCCFV_preprocessor::solve_trivial_assignments(void) {
  //sort L_pre with lexicographic order on terms in literals

  struct {
    bool operator()(TCC_literal l1, TCC_literal l2) const {
      std::vector<unsigned> a1 = {l1.polarity, l1.lhs, l1.rhs};
      std::vector<unsigned> a2 = {l2.polarity, l2.lhs, l2.rhs};
      return std::lexicographical_compare(a1.begin(), a1.end(), a2.begin(), a2.end());
    }
  } lexicographic_literal_compare;
  std::sort(L_pre.begin(), L_pre.end(), lexicographic_literal_compare);

  // TODO
  //create trivial array
  //find easy cases of trivial equations
  //fixpoint on terms that occur just once in L_pre (var_occurrences array) with dichotomic search in L_pre to find the corresponding literal: mark the ones on lhs of a lit as trivial and update all var occurrences from this lit
}

std::vector<Tliteral> CCCFV_preprocessor::run(void) {
  // flatten to call on each literal in L_pre
  // solve_trivial_assignments();
  return std::vector<Tliteral>();
}

void CCCFV_preprocessor::CC_add_term(Tterm t) {
  assert(t.index < index_to_term.size());

  if (index_to_term[t.index].index != 0) {
    assert(index_to_term[t.index].index == t.index);
    return;
  }

  if (t.symb() != 0) {
    CC_build_term(Ecc, t.index, 0, NULL);
  } else {
    CC_add_term(t[0]);
    CC_add_term(t[1]);
    TCC_term subterms[2] = {(TCC_term) t[0].index, (TCC_term) t[1].index};
    CC_build_term(Ecc, t.index, 2, subterms);

    index_to_term[t[0].index] = TCCFVterm(t[0]);
    index_to_term[t[1].index] = TCCFVterm(t[1]);
  }

  index_to_term[t.index] = t;
  is_ground.insert(t.index);
}


TCC_term CCCFV_preprocessor::CC_add_ground_subterms_in_nonground_term_and_return_normalized(Tterm t) {
  if (t.symb() != 0) {
    index_to_term[t.index] = t;

    if(!(is_var[t.index])){
      is_ground.insert(t.index);

      CC_build_term(Ecc, (TCC_term) t.index, 0, NULL);
      return (CC_get_class(Ecc, (TCC_term) t.index));
    } else {
      return ((TCC_term)t.index);
    }
  } else {
    index_to_term[t[0].index] = t[0];
    index_to_term[t[1].index] = t[1];

    TCC_term t0 =
        CC_add_ground_subterms_in_nonground_term_and_return_normalized(t[0]);
    TCC_term t1 =
        CC_add_ground_subterms_in_nonground_term_and_return_normalized(t[1]);

    TCCFVterm t_norm = get_applied_term(index_to_term[t0], index_to_term[t1]);
    index_to_term[t_norm.index] = t_norm;

    if (is_ground.find(t0) != is_ground.end() && is_ground.find(t1) != is_ground.end()) {
      is_ground.insert(t.index); // probably unneeded
      is_ground.insert(t_norm.index);
      TCC_term subterms[2] = {t0, t1};
      CC_build_term(Ecc, (TCC_term) t_norm.index, 2, subterms);
      return ((TCC_term) t_norm.index);
    }
    else {
      return ((TCC_term) t_norm.index);
    }
  }
}

TCC_literal CCCFV_preprocessor::swap_sides(TCC_literal l) {
  TCC_literal l2;
  l2.lhs = l.rhs;
  l2.rhs = l2.lhs;
  l2.polarity = l.polarity;
  return l2;
}


