/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Utility functions for CCFV (Congruence Closure with Free Variables):
 * trigger term checks, groundness test and higher-order floater utilities.
 */

#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_UTILS_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_UTILS_H

#include <map>
#include <memory>
#include <vector>

#include "expr/node.h"

namespace cvc5::internal {
namespace theory {

namespace eq {
class EqualityEngine;
}

namespace quantifiers {
namespace ccfv {

bool isSupportedTriggerTerm(TNode n);

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

  bool isFloatable(TNode var)
  {
    Node root = d_dsu->find(var);
    auto it = d_floatable.find(root);
    return (it != d_floatable.end()) ? it->second : false;
  }

  Node getGround(TNode rep, size_t index) const;
  void setGround(TNode rep, size_t index, TNode x);

 private:
  std::unique_ptr<FloatersUnionFind> d_dsu;
  std::vector<Node> d_freeVars;
  std::vector<Node> d_lits;
  std::map<Node, bool> d_floatable;
  std::map<Node, Node> d_ground[2];
};

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
