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

inline bool isLiteral(Node n)
{
  if (n.getKind() == Kind::NOT)
  {
    n = n[0];
    if (n.getKind() == Kind::NOT)
    {
      return false;
    }
  }
  if (n.getKind() == Kind::FORALL || n.getKind() == Kind::EXISTS)
  {
    return false;
  }
  return QuantifiersRewriter::isLiteral(n);
}

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
  while (n.getKind() == Kind::FORALL)
  {
    n = n[1];
  }
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

Node computePrenex(NodeManager* nm, Node n, std::vector<Node>& vars);

std::vector<std::vector<Node>> toCnf(Node n);

bool computeQuantCnf(Node q,
                     std::vector<Node>& vars,
                     std::vector<std::vector<Node>>& clauses);

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
