/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * CCFV clausal normal form (CNF) preprocessing of asserted quantified
 * formulas: NNF, skolemization, prenexing and distributive CNF expansion.
 */

#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_CNF_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_CNF_H

#include <vector>

#include "expr/node.h"
#include "theory/quantifiers/quantifiers_rewriter.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {
namespace ccfv {

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
inline bool isLiteral(TNode n)
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
inline bool isClause(TNode n)
{
  if (isLiteral(n) || n.isConst()) return true;
  if (n.getKind() == Kind::OR)
  {
    for (TNode c : n)
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
inline bool isCnf(TNode n)
{
  while (n.getKind() == Kind::FORALL)
  {
    n = n[1];
  }
  if (isClause(n)) return true;
  if (n.getKind() == Kind::AND)
  {
    for (TNode c : n)
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
Node computeNnf(NodeManager* nm, TNode n, bool pol);

Node skolemize(NodeManager* nm, TNode n, std::vector<Node>& freeVars);

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
Node computePrenex(NodeManager* nm, TNode n, std::vector<Node>& vars);

std::vector<std::vector<Node>> toCnf(TNode n);

/** The result of converting a quantified formula into CNF. */
enum class CnfStatus
{
  /** The quantifier was converted. */
  SUCCESS,
  /** The quantifier has constructs that are not supported. */
  UNSUPPORTED,
  /** The CNF would exceed the literal limit, so it was not built. */
  TOO_LARGE
};

/**
 * Full preprocessing pipeline converting an asserted quantified formula into
 * CNF for CCFV.
 *
 * Executes the three-stage transformation:
 *   1. NNF conversion via computeNnf;
 *   2. Universal variable lifting via computePrenex;
 *   3. Clausal conversion via toCnf.
 *
 * @param q The asserted quantified formula.
 * @param vars Output vector receiving all universal free variables.
 * @param clauses Output collection receiving the generated CNF clauses.
 * @param cnfLiteralLimit The maximum number of literals of the CNF (0 for no
 * limit). It is checked before the distributivity, so a CNF that would exceed
 * it is never built.
 * @return SUCCESS if the quantifier was converted, UNSUPPORTED if it has
 * constructs the conversion does not handle, TOO_LARGE if its CNF would exceed
 * the literal limit.
 */
CnfStatus computeQuantCnf(TNode q,
                          std::vector<Node>& vars,
                          std::vector<std::vector<Node>>& clauses,
                          uint64_t cnfLiteralLimit);

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
