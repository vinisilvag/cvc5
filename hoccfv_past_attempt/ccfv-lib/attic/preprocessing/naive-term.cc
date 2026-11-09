
#include <assert.h>
#include <iostream>
#include <ostream>

#include "naive-term.hh"


NaiveTterm::NaiveTterm(Tsymb s)
{
  index = s;
  std::cout << "creation of term " << s << std::endl;
}

NaiveTterm::NaiveTterm(Tsymb i, NaiveTterm c1, NaiveTterm c2)
{
  index = i;
  children.push_back(c1);
  children.push_back(c2);
  std::cout << "creation of applied term " << c1.index << " " << c2.index << std::endl;
}

Tsymb NaiveTterm::symb()
{
  return (Tsymb) (children.empty()?index:SYMB_APPLY);
}

unsigned NaiveTterm::arity()
{
  return (children.empty()?0:2);
}

Tterm NaiveTterm::operator[](unsigned i)
{
  assert(! children.empty() && i < 2);
  return children[i];
}

unsigned Tterm::arity() { return 0; }

Tterm Tterm::operator[](unsigned i) { return *this; }

Tsymb Tterm::symb() {
  return SYMB_APPLY;
}
