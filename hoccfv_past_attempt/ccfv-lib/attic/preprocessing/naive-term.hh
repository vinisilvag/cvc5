#include <assert.h>
#include <vector>

typedef unsigned Tsymb;

#define SYMB_APPLY ((Tsymb) 0)

typedef class Tterm
{
public:
  unsigned index;
  /** \brief cast operator so that Tterm usable as index */
  operator unsigned() const { return index; }
  /** \brief topmost symbol, or SYMB_APPLY */
  Tsymb symb();
  /** \brief number of sub-terms */

  operator char *() const { return nullptr; } // TODO

  unsigned arity();
  /** \brief sub-terms */
  Tterm operator[](unsigned i);

  Tterm(Tsymb s);
  Tterm(unsigned i, Tterm c1, Tterm c2);

private:
  std::vector<Tterm> children;

} Tterm;

/** \brief builds the term t1 t2 from t1 and t2*/
Tterm get_applied_term(Tterm t1, Tterm t2); // TODO

