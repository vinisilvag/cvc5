#define DEBUG

/*
  TODO
  - function naming
  - think about pairs of classes, etc... (many data structures, and field names are not great)
    c[0] = c[1] c[2] would maybe be better c[0]c[1]=c[2] or c[0]c[1]=u  
  - find all solutions
  - examples examples examples
  - automate examples (read from input file)
  - write an SMT solver around the library ^^
*/

/*
  --------------------------------------------------------------
  CCFV computation module
  --------------------------------------------------------------
*/

/* An HOCCFV problem is a set of equalities and inequalities E between
   ground terms and a conjunctive set of literals L.
   The solution is a substitution sigma such that
   E |= L sigma

   The interface assumes that congruence classes of ground terms in E and L have
   been previously computed by the user, and that the user can associate a small
   integer to each of those classes.  We also assume that a small integer is
   associated to each variable and (non-ground) term in L.  All those integers
   must be distinct, and they can serve as array index (hence they must be
   small).

   (Within veriT, one can use, e.g., the DAG id of any representative of a
   class, and the DAG id of non-ground terms and variables).

   A signature is an abstraction of a term in a class.  If a term (t_1 ... t_n)
   belongs to some congruence class, the signature for this term is
   (c_1 ... c_n) where c_i is the class of t_i.  Since the exact equations in E
   do not really matter, it is rather specified to this module using sets of
   signatures, all signatures in each set being equal according to E.

   A signature is specified either as
   - the application of one class on another
   (u v)
   - the application of one class on several others
   (u_0 ... u_n)
   which actually stands for
   (... ((u_0 u_1) u_2) ... u_n)

   A class might have a type, which is again a small integer number.  By default
   this number is 0.  This is kept abstract (i.e., there is no association
   between the type of a function and the type of the application of this
   function to another class).

   The CCFV solution is such that a non-ground term will never be associated to
   a class of different type, and in particular, a variable will never be
   assigned to a class of different type, because of the substitution.
*/

/*
  PF: remark.  Currently all functions are authored by Pascal Fontaine: it is
  not that I want to assess my authorship from the beginning, but it is just
  that I find convenient to know who wrote a function.  Don't be offended ;-) ! */

#include <vector>

#include "SAT.hh"

/**
   \author Pascal Fontaine & Sophie Tourret
   \brief type to denote a class (congruence class of ground terms,
     class with non-ground terms, or variable)
     */
typedef unsigned TCCFV_class;

/**
   \author Pascal Fontaine & Sophie Tourret
   \brief type to denote a (abstract) type */
typedef unsigned TCCFV_type;

/**
   \author Pascal Fontaine & Sophie Tourret
   \brief structure to store equalities u0 = u1 u2, either in E or L */
struct TCCFV_pos {
   TCCFV_class c[3];
   TCCFV_pos(TCCFV_class u0, TCCFV_class u1, TCCFV_class u2)
     { c[0] = u0; c[1] = u1; c[2] = u2; };
};

/**
   \author Pascal Fontaine & Sophie Tourret
   \brief structure to store disequalities u0 != u1, either in E or L */
struct TCCFV_neg {
   TCCFV_class c[2];
   TCCFV_neg(TCCFV_class u0, TCCFV_class u1) { c[0] = u0; c[1] = u1; };
};

/**
   \author Pascal Fontaine & Sophie Tourret
   \brief structure used in an array indexed by class c to get SAT variable p corresponding to P_{x,c} */
struct Tvar_pvar {
   TCCFV_class x;
   CCFV_SAT::Tvar p;
   Tvar_pvar(TCCFV_class x, CCFV_SAT::Tvar p) :
      x{x}, p{p} {}
};

/**
   \author Pascal Fontaine & Sophie Tourret
   \brief structure used in an array indexed by SAT vars to get associated P_{x,[c]} or Q_x
   \note 0 is reserved when occurring as c to indicate Q instead of P*/
struct Tvar_class {
   TCCFV_class x;
   TCCFV_class c;
   Tvar_class(TCCFV_class x, TCCFV_class c) :
      x{x}, c{c} {}
};

/**
 * \author Sophie Tourret
 * \brief structure used in an array indexed by variables to represent the images of the solution substitution.
 * \note 0 is reserved when occurring as t1 to indicate x |-> x and when occurring as t2 to indicate x |-> t1.
 */
struct Tclass_class {
   TCCFV_class t1;
   TCCFV_class t2;
   Tclass_class(TCCFV_class t1 = 0, TCCFV_class t2 = 0) :
      t1{t1}, t2{t2} {}
};

class CCCFV_solver {
public:
  CCCFV_solver(void);

  /**
     \author Pascal Fontaine & Sophie Tourret
     \brief mark the class as representing a variable
     \param c the class */
  void class_is_var(TCCFV_class c);

  /**
     \author Pascal Fontaine & Sophie Tourret
     \brief adds the signature (u v) to c
     \param c0 destination class
     \param c1 function class
     \param c2 argument class */
  void class_sig(TCCFV_class c0, TCCFV_class c1, TCCFV_class c2);

  /**
     \author Pascal Fontaine
     \brief adds the signature (u0 u1 ... un) to c
     \param c destination class
     \param n number of arguments
     \param u0 function class
     \param u array of argument classes */
  void class_sig(TCCFV_class c, unsigned n, TCCFV_class u0, TCCFV_class * u);

  /**
     \author Pascal Fontaine
     \brief adds disequality between two classes
     \param c0 destination class
     \param c1 destination class */
  void class_neq(TCCFV_class c0, TCCFV_class c1);

  /**
     \author Pascal Fontaine
     \brief sets type to class
     \param class destination class */
  void set_class_type(TCCFV_class c, TCCFV_type type);

  /**
     \author Pascal Fontaine & Sophie Tourret
     \brief adds a constraint c = u v to the CCFV problem
     \param u0 variable or class
     \param u1 variable or class
     \param u2 variable or class */
  void constraint_pos(TCCFV_class u0, TCCFV_class u1, TCCFV_class u2);

  /**
     \author Pascal Fontaine & Sophie Tourret
     \brief adds a disequality constraint to the CCFV problem
     \param u0 first variable
     \param u1 second variable or class */
  void constraint_neg(TCCFV_class u0, TCCFV_class u1);

  /**
     \author Pascal Fontaine & Sophie Tourret
     \brief solves the CCFV problem, or get the next CCFV solution
     \return false iff the problem is unsolvable */
  bool solve(void);

  /**
     \author Pascal Fontaine
     \brief gets the substitution
     \param var the variable for which we want the substituted class
     \return the substituted class */
  Tclass_class &operator[](TCCFV_class var);

  /**
   * @brief resets the CCFV solver, but preserves all buffered memory */
  void reset(void);

  /**
     \author Pascal Fontaine
     \brief resets the CCFV solver and reclaims memory */
  void hard_reset(void);

#ifdef DEBUG  
  /**
   * @author Sophie Tourret
   * @brief adds a correspondance from a class c to a representative symbol s
   * 
   * @param c 
   * @param s 
   */
  void debug_symbol(TCCFV_class c, std::string s);

  /**
   * @author Pascal Fontaine
   * @brief prints the solution with symbols set by debug_symbol
   */  
  void debug_solution(void);
  void debug_P_Q_to_x(void);

  void debug_CNF(void);
  void debug_model(void);
#endif

private:
  CCFV_SAT::Csolver SAT_solver;

  std::vector<TCCFV_pos> Epos;
  std::vector<TCCFV_neg> Eneg;
  std::vector<TCCFV_pos> Lpos;
  std::vector<TCCFV_neg> Lneg;

  unsigned solution_count = 0;
  
  std::vector<bool> is_var;       /*<< states if a TCCFV_class is a variable */
  std::vector<TCCFV_type> class_type;   /*<< type of class */
  std::vector<TCCFV_class> vars;  /*<< index on the variables for iteration */
  std::vector<bool> floatable;    /*<< states if a variable is a floatable */
  std::vector<TCCFV_class> representative; /*<< gets the representative of a variable is a floatable */

  /* There are several data structures to link propositional variables and variables in the constraints */
  /* Q_x states whether x is floating: note that Q_x only exists for variables s.t. x=Find(x) */
  /* P_{x,c} states whether x is set to class c */
  std::vector<CCFV_SAT::Tvar> x_to_Q; /*<< maps the variable x to Q_{Find(x)} */
  std::vector<std::vector<Tvar_pvar>> x_c_to_P; /*<< maps the pair variable-constant (x,c) to P_{x,c}: indexed by c, gives a list of pairs (x,P) */
  std::vector<Tvar_class> P_Q_to_x; /*<< maps the propositional variables P and Q to (x,c) where c=0 means Q: indexed by CCFV_SAT::Tvar */

  /* data structures for computing floater injectivity */
  std::vector<TCCFV_class> ground[2];
  std::vector<TCCFV_class> child[2];  

  /* data structures for cycle detection */
  std::vector<TCCFV_class> index;
  std::vector<TCCFV_class> low;
  std::vector<bool> onstack;
  unsigned counter;
  std::vector<TCCFV_class> S_stack;

  /* data structures for solution reconstruction */
  std::vector<Tclass_class> solution;
  std::vector<bool> normalized;

  /* data structure for pretty printing */
  std::vector<std::string> c_to_string;

  /**
   * @brief ensures that all data structures are sufficiently large to store information about some class or variable
   * @author Pascal Fontaine
   * @param c the TCCFV_class to accommodate
   */
  void expand(TCCFV_class c);

  /**
   * @brief computes the floater-equivalence classes of variables
   * @author Pascal Fontaine & Sophie Tourret
   */
  void FCompute(void);

  /**
   * @brief finds the representative of the floater equivalence class 
   * @param x 
   * @return TCCFV_class 
   * @author Sophie Tourret
   */
  TCCFV_class FFind(TCCFV_class x);

  /**
   * @brief updates floatable with constraints due to different types or a constant occuring in the same floater class.
   *        TODO3: only ground constraints are currently handled, type counstraints must be added later.
   *
   * @author Sophie Tourret 
   */
  void Ground_or_Type_Constraints(void);

  /**
   * @brief detects cycles in the floater dependency graph 
   * @author Sophie Tourret
   */
  void Cycle_Detection(void);
  
  /**
   * @author Pascal Fontaine & Sophie Tourret
   * @brief Tarjan's algorithm for strongly connected components computation
   * 
   * @param x 
   */
  void strongconnect(TCCFV_class x);

  /**
    \author Pascal Fontaine & Sophie Tourret
    \brief solves the CCFV problem, or get the next CCFV solution
    \return false iff the problem is unsolvable */
  void Generate_Constraints(void);

  /**
   * @brief builds a substitution (possibly non-ground) from a propositional model
   * 
   * @author Sophie Tourret
   */
  void Substitution_Reconstruction(void);

  /**
   * @brief subprocedure of Substitution_Normalize, that normalizes a variable and the ones it depends on recursively
   *  
   * @author Sophie Tourret
   */
  void normalize_var(TCCFV_class x);

  /**
   * @brief normalizes the substitution built by Substitution_Reconstruction
   *  
   * @author Sophie Tourret
   */
  void Substitution_Normalize(void);

  /**
   * @brief returns the Q propositional variable associated to a variable x, creates it if it doesn't exist
   * 
   * @param x 
   * @return CCFV_SAT::Tvar 
   * @author Sophie Tourret
   */
  CCFV_SAT::Tvar get_Q(TCCFV_class x);
  CCFV_SAT::Tlit build_P(TCCFV_class u0, TCCFV_class u1);
  CCFV_SAT::Tvar get_P(TCCFV_class x, TCCFV_class c);

};
