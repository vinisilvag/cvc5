#define DEBUG

/*
  --------------------------------------------------------------
  CCFV preprocessing module
  --------------------------------------------------------------
*/

/* An HOCCFV problem is a set of equalities and inequalities E between
   ground terms and a conjunctive set of literals L.

   L must be composed of literals of the form x != y, x != t or u0 = u1 u2,
   where at least one of u1 and u2 is a variable, but the solver wants the
   solution for generic literals so a transformation is needed before calling
   CCFV.

   In addition, these transformations must be done on the data-structures of the
   solver, so this preprocessing module will be specific to it (and that is why
   it is kept separated from the main CCFV module).
 */

#include <vector>
#include <unordered_set>

#include "term.hh"
extern "C" {
#include "CC.h"
}

/**
   \author Sophie Tourret
   \brief structure to store terms
 */
typedef class TCCFVterm
{
public:
  unsigned index;

  operator unsigned() const { return index; }

  TCCFVterm(Tterm) {}

  TCCFVterm(unsigned i): index(i) {}

  Tsymb symb();

  unsigned arity();

  Tterm operator[](unsigned i);
} TCCFVterm ;

struct TCCFVliteral {
  bool polarity;
  TCCFVterm lhs;
  TCCFVterm rhs;
};

/**
   \author Sophie Tourret
   \brief structure to store literals
 */
struct Tliteral {
  bool polarity;
  Tterm lhs;
  Tterm rhs;
};

struct Tvar_term {
  bool is_ground;
  TCC_term t;
};

struct TCC_literal {
  bool polarity;
  TCC_term lhs;
  TCC_term rhs;
};

struct TCCterm_litlist {
  TCC_term t;
  std::vector<TCC_literal> l_list;
};

class CCCFV_preprocessor {
public:
  /**
     \author Sophie Tourret
     \brief initializes a preprocessor
     \param L array of terms (representing literals)
     \param vars array of terms identifying the variables
   */
  CCCFV_preprocessor(std::vector<bool> vars, unsigned max_index);
  /*
   * Since the congruence closure will also be used to initialize the CCFV
   * problem, it may be better to compute it before the preprocessing than
   * during it once all pieces are assembled.
   */

  /**
     \author Sophie Tourret
     \brief destroys a preprocessor
   */
  ~CCCFV_preprocessor(void);

  /**
     \author Sophie Tourret
     \brief adds a ground literal to the congruence closure Ecc
     \param e the literal to add
   */
  void E_update(Tliteral e);

  /**
     \author Sophie Tourret
     \brief adds a literal to the set of literals to preprocess
     \param l the literal to add
     \return false if the literal was ground and is not satisfied by the
     congruence closure, and true otherwise
   */
  bool L_update(Tliteral l);

  /**
     \author Sophie Tourret
     \brief flattens a literal
     \param l the literal to flatten
     \return a vector of the flattening result
   */
  std::vector<TCC_literal> flatten_literal(TCC_literal l);

  /**
     \author Sophie Tourret
     \brief computes the preprocessing
     \return the preprocessed literals to use in CCFV
   */
  std::vector<Tliteral> run(void);

  /**
     \author Sophie Tourre
     \brief undo the preprocessing for the solution of the ccfv problem
     \param subst the substitution solving the preprocessed problem
     \return the substitution solving the original problem
  */
  std::vector<Tterm> undo(std::vector<Tterm> subst);

private:
  /**
     \author Sophie Tourret
     \brief the congruence closure built from E
   */
  CC_STATE Ecc;

  /**
     \author Sophie Tourret
     \brief the set of literals being preprocessed, they will be modified during
     the preprocessing
   */
  std::vector<TCC_literal> L_pre;

  /**
     \author Sophie Tourret
     \brief indicates whether a term is a variable
   */
  std::vector<bool> is_var;

  /**
     \author Sophie Tourret
     \brief indicates whether a term is ground
   */
  std::unordered_set<unsigned> is_ground;

  /**
     \author Sophie Tourret
     \brief stores TCCFVterms by their indices
     \remark at some point, this should be replaced by a global access to TCCFVterms (?)
     (or not, global should be avoided)
   */
  std::vector<TCCFVterm> index_to_term;

  /**
     \author Sophie Tourret
     \brief indicates how many occurrences of a variable are in L_pre
   */
  std::vector<int> var_occurrences;

  /**
     \author Sophie Tourret
     \brief provides a fresh index for creating new variables,
     \remark incremented by one after each new creation (introduced variables have contiguous indices)
   */
  unsigned next_new_term;

  /**
     \author Sophie Tourret
     \brief normalizes L based on Ecc
   */
  void normalize(void);

  Tterm normalize_term(Tterm t);

  void remove_ground_literals(void);

  void flatten(void);

  void solve_trivial_assignments(void);

  /**
   \brief adds a ground term to CC
  */
  void CC_add_term(Tterm t);

  TCC_term CC_add_ground_subterms_in_nonground_term_and_return_normalized(Tterm t);

  TCC_literal swap_sides(TCC_literal l);

  TCCterm_litlist flatten_term(TCC_term t);

  TCCterm_litlist flatten_subterms(TCC_term t);

  TCCFVterm get_fresh_variable();

  /** \brief builds the term t1 t2 from t1 and t2*/
  TCCFVterm get_applied_term(TCCFVterm t1, TCCFVterm t2);

  void sort_L_pre(void);
};
