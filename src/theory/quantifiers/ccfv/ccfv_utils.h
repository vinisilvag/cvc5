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

struct CnfCount
{
  size_t clauses = 0;
  size_t totalLiterals = 0;
};

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

Node computeNnf(NodeManager* nm, Node n, bool pol);

Node computePrenex(NodeManager* nm, Node n, bool pol);

std::vector<std::vector<Node>> toCnf();

// TODO: implement it later
class CcfvIndices
{
 public:
  CcfvIndices(eq::EqualityEngine* ee);
  std::vector<Node> getDiseqs(Node t);
  std::vector<Node> getReps(Node f);
  std::vector<Node> getApps(Node f, Node t);

 private:
};

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
