/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * TODO: add later
 */

#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_UTILS_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_UTILS_H

#include <vector>

#include "expr/node.h"
#include "theory/quantifiers/quantifiers_rewriter.h"

namespace cvc5::internal {
namespace theory {

namespace eq {
class EqualityEngine;
}

namespace quantifiers {
namespace ccfv {

inline bool isLiteral(Node n) { return QuantifiersRewriter::isLiteral(n); }

inline bool isClause(Node n)
{
  if (isLiteral(n) || n.isConst()) return true;
  if (n.getKind() == Kind::OR)
  {
    for (const Node& c : n)
    {
      if (!isLiteral(c) && !c.isConst()) return false;
    }
    return true;
  }
  return false;
}

inline bool isCnf(Node n)
{
  if (isClause(n)) return true;
  if (n.getKind() == Kind::AND)
  {
    for (const Node& c : n)
    {
      if (!isClause(c)) return false;
    }
    return true;
  }
  return false;
}

std::vector<Node> toCnf(NodeManager* nm, Node body);

// TODO: implement it later
class CcfvIndices
{
 public:
  CcfvIndices(eq::EqualityEngine* ee);

 private:
};

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
