#define DEBUG

/*
  --------------------------------------------------------------
  CCFV pre- and post-processing module
  --------------------------------------------------------------
*/

/* An HOCCFV problem is a set of equalities and inequalities E between
   ground terms and a conjunctive set of literals L.

   L must be composed of literals of the form x != y, x != t or u0 = u1 u2,
   where at least one of u1 and u2 is a variable, but the solver wants the
   solution for generic literals so a transformation is needed before calling
   CCFV, as well as a means to produce substitutions in the form expected by the
   solver.

   In addition, these transformations must be done starting from the
   data-structures of the solver, so this preprocessing module will be specific
   to it (and that is why it is kept separated from the core CCFV module).

   /!\ (TODO) We assume that all Tterm terms are already currified, but this may
   not be true in practice! When dealing with this discrepancy, currification
   could be done in the functions where CC_build_term is called.


 */

#include <vector>

#include "term.hh"
extern "C" {
#include "CC.h"
}
#include "ccfv-core.hh"


class CCFV {
public:
  /* constructor(s) */
  CCFV(const std::vector<Tterm>& E);
  /* destructor(s) */
  ~CCFV(void);

  /** Add literals that need to be entailed. */
  void entail_L(const std::vector<Tterm>& L);

  /**
     \brief computes the next solution to the CCFV problem, if any
     \return whether there was a solution to be retrieved
   */
  bool get_next_solution(std::vector<Tterm>& solution);

private:
  /**
     \author Sophie Tourret
     \brief the core CCFV solver

     \remark the problem needs to be fully preprocessed to make use of it
   */
  CCCFV_solver core_solver;

  /**
     \author Sophie Tourret
     \brief the full preprocessing

     \remark all of E and L must have been added before
   */
  void preprocess(void);

  /**
     \author Sophie Tourret
     \brief the congruence closure built from E
  */
  CC_STATE d_ecc;

};
