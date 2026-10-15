/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Utility functions for CCFV (Congruence Closure with Free Variables),
 * defining the clausal normal form (CNF) preprocessing interface for
 * asserted quantified formulas.
 */

#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_UTILS_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_UTILS_H

#include <unordered_map>
#include <vector>

#include "expr/node.h"
#include "theory/quantifiers/quantifiers_rewriter.h"
#include "util/hash.h"

namespace cvc5::internal {
namespace theory {

namespace eq {
class EqualityEngine;
}

namespace quantifiers {
namespace ccfv {

using NodePairHash =
    PairHashFunction<Node, Node, std::hash<Node>, std::hash<Node>>;

/**
 * Checks whether a node is a first-order literal (an atom or its negation).
 *
 * Rejects boolean connectives (AND, OR, ITE, IMPLIES, XOR) as well as
 * quantified formulas (FORALL, EXISTS).
 *
 * @param n The node to inspect.
 * @return true if n is an atomic formula, a negated atomic formula, or a
 * boolean constant.
 */
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

/**
 * Checks whether a node is a clause (a single literal or a flat disjunction of
 * literals).
 *
 * @param n The node to inspect.
 * @return true if n is a literal or an OR node containing only literals.
 */
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

/**
 * Checks whether a formula is in Conjunctive Normal Form (CNF).
 *
 * Automatically inspects the formula beneath any leading universal quantifiers.
 *
 * @param n A quantified formula.
 * @return true if the formula is a single clause or an AND of clauses.
 */
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

/**
 * Computes the Negation Normal Form (NNF) of a formula.
 *
 * Normalizes all logical connectives: pushes negations to the leaves,
 * eliminates implications, bi-implications and conditionals, flattens
 * associative operators, and applies quantifier duality.
 *
 * @param nm Pointer to the NodeManager.
 * @param n The formula to normalize.
 * @param pol The polarity context (true for positive, false under odd
 * negations).
 * @return The normalized formula in NNF, or Node::null() if an unsupported node
 * is found.
 */
Node computeNnf(NodeManager* nm, Node n, bool pol);

/**
 * Prenexes an NNF formula by lifting all universal quantifiers to the outer
 * level.
 *
 * Strips all internal FORALL nodes, accumulates their bound variables into
 * `vars`, and reconstructs the quantifier-free matrix.
 *
 * @param nm Pointer to the NodeManager.
 * @param n The formula in NNF.
 * @param vars Output vector collecting all universal bound variables.
 * @return The quantifier-free boolean matrix, or Node::null() on error.
 */
Node computePrenex(NodeManager* nm, Node n, std::vector<Node>& vars);

std::vector<std::vector<Node>> toCnf(Node n);

/**
 * Full preprocessing pipeline converting an asserted quantifier into CNF for
 * CCFV.
 *
 * Executes the three-stage transformation:
 *   1. NNF conversion via computeNnf;
 *   2. Universal variable lifting via computePrenex;
 *   3. Clausal conversion via toCnf.
 *
 * @param q The asserted quantified formula.
 * @param vars Output vector receiving all universal free variables.
 * @param clauses Output collection receiving the generated CNF clauses.
 * @return true if the quantifier was successfully converted, false otherwise.
 */
bool computeQuantCnf(Node q,
                     std::vector<Node>& vars,
                     std::vector<std::vector<Node>>& clauses);

class CcfvIndices
{
 public:
  CcfvIndices(eq::EqualityEngine* ee);
  const std::vector<Node>& getDiseqs(Node t) const;
  const std::vector<Node>& getReps(Node f) const;
  const std::vector<Node>& getApps(Node f, Node t) const;

 private:
  std::unordered_map<Node, std::vector<Node>> diseqs;
  std::unordered_map<std::pair<Node, Node>, std::vector<Node>, NodePairHash>
      apps;
  std::unordered_map<Node, std::vector<Node>> reps;

  static const std::vector<Node> s_empty;
};

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
