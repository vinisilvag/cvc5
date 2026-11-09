#ifndef SAT_HH
#define SAT_HH

#include <vector>

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
private:
  void * solver;
};

}

#endif
