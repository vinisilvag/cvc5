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
bool computeQuantCnf(TNode q,
                     std::vector<Node>& vars,
                     std::vector<std::vector<Node>>& clauses);

bool isSupportedTriggerTerm(TNode n);

class CcfvIndices
{
 public:
  CcfvIndices(eq::EqualityEngine* ee);
  const std::vector<Node>& getDiseqs(TNode t) const;
  const std::vector<Node>& getReps(TNode f) const;
  const std::vector<Node>& getApps(TNode f, TNode t) const;

 private:
  eq::EqualityEngine* d_ee;
  std::unordered_map<Node, std::vector<Node>> diseqs;
  std::unordered_map<std::pair<Node, Node>, std::vector<Node>, NodePairHash>
      apps;
  std::unordered_map<Node, std::vector<Node>> reps;

  static const std::vector<Node> s_empty;
};

class FloatersUnionFind
{
 public:
  FloatersUnionFind() = default;
  FloatersUnionFind(const std::vector<Node>& xs);

  Node find(TNode x);
  void merge(TNode x, TNode y, std::vector<std::pair<Node, Node>>& toMerge);
  bool areEquivalent(TNode x, TNode y);

  Node getChild(TNode rep, size_t index) const;
  void setChild(TNode rep, size_t index, TNode x);
  bool hasChildren(TNode rep) const;

  void debugPrint(std::ostream& out = std::cout);

 private:
  std::map<Node, Node> d_representative;
  std::map<Node, Node> d_child[2];
};

class HoccfvUtils
{
 public:
  HoccfvUtils(const std::vector<Node>& freeVars, const std::vector<Node>& lits);
  ~HoccfvUtils() = default;

  void computeFloaterEquivalences();
  void applyGroundOrTypeConstraints();
  void computeFloaterDependencyGraph();

  const FloatersUnionFind* getDsu() const { return d_dsu.get(); }
  FloatersUnionFind* getDsu() { return d_dsu.get(); }

  void debugPrintFloaterEquivalences() const
  {
    if (d_dsu)
    {
      d_dsu->debugPrint();
    }
  }

 private:
  std::unique_ptr<FloatersUnionFind> d_dsu;

  std::vector<Node> d_freeVars;
  std::vector<Node> d_lits;
  std::map<Node, bool> d_floatable;
};

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
