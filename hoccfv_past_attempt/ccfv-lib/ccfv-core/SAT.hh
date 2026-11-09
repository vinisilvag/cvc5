#ifndef SAT_HH
#define SAT_HH

#define DEBUG

#include <vector>

#include "cadical.hpp"

namespace CCFV_SAT {

/**
   \author Pascal Fontaine & Sophie Tourret
   \brief type to denote a propositional variable
   \remark variable 0 is reserved */
typedef unsigned Tvar;

/**
   \author Pascal Fontaine & Sophie Tourret
   \brief type to denote a propositional literal
   \remark literals with var 0 denote false and true constants, depending on polarity */
typedef unsigned Tlit;

inline Tlit var_to_lit(Tvar var, bool pol = true)
{ return (var << 1) + pol; }

inline Tlit lit_var(Tlit lit)
{ return lit >> 1; }

inline bool lit_pol(Tlit lit)
{ return lit & 1; }

inline int lit_DIMACS(Tlit lit)
{ return lit_pol(lit)?lit_var(lit):-lit_var(lit); }

#define lit_true var_to_lit(0, true)
#define lit_false var_to_lit(0, false)

class Csolver {
public:
  std::vector<Tvar> model;
  Csolver(void);
  ~Csolver(void);
  void add(Tlit lit_or_zero);
  void add_clause(Tlit lit);
  void add_clause(Tlit lit0, Tlit lit1);
  void add_clause(Tlit lit0, Tlit lit1, Tlit lit2);
  int solve(void);
#ifdef DEBUG
  std::vector<std::vector<Tlit>> debug_CNF;
#endif
private:
  Tvar var_max = 0;
  CaDiCaL::Solver solver;
};

}

#endif
