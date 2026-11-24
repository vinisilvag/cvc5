/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * SAT-encoded CCFV solver for E-ground (dis)unification.
 */

#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_SAT_SOLVER_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_SAT_SOLVER_H

#include <map>
#include <utility>
#include <vector>

#include "expr/node.h"
#include "smt/env_obj.h"

namespace cvc5::internal {
namespace theory {

namespace eq {
class EqualityEngine;
}

namespace quantifiers {

/**
 * CCFV solver based on a reduction of E-ground (dis)unification to
 * propositional SAT.
 */
class CcfvSatSolver : protected EnvObj
{
 public:
  CcfvSatSolver(Env& env);
  ~CcfvSatSolver() = default;

  /**
   * @param freeVars The free variables in L (the quantified variables).
   * @param lits The set of equational literals L to satisfy.
   * @param ee The equality engine representing the ground context E.
   * @param substitutions Output: solutions, index-by-index with `freeVars`.
   * @return True if at least one solution was found, false otherwise.
   */
  bool solve(const std::vector<Node>& freeVars,
             const std::vector<Node>& lits,
             eq::EqualityEngine* ee,
             std::vector<std::vector<Node>>& substitutions);

 private:
  bool isPreprocessed(const std::vector<Node>& lits, eq::EqualityEngine* ee);

  bool normalize(std::vector<Node>& lits, eq::EqualityEngine* ee);
  bool flatten(std::vector<Node>& lits);
  bool removeGroundLiterals(std::vector<Node>& lits);
  bool removeTrivialAssignments(std::vector<Node>& lits);

  void preprocess(std::vector<Node>& lits, eq::EqualityEngine* ee);

  std::map<std::pair<Node, Node>, Node> d_pNodes;
  std::map<Node, Node> d_qNodes;
  Node getPVar(TNode x, TNode rep)
  {
    auto it = d_pNodes.find({x, rep});
    if (it != d_pNodes.end())
    {
      return it->second;
    }
    NodeManager* nm = nodeManager();
    Node v = nm->mkDummySkolem("P_" + x.toString() + "_" + rep.toString(),
                               nm->booleanType());
    d_pNodes[{x, rep}] = v;
    return v;
  }

  Node getQVar(TNode x)
  {
    auto it = d_qNodes.find(x);
    if (it != d_qNodes.end())
    {
      return it->second;
    }
    NodeManager* nm = nodeManager();
    Node v = nm->mkDummySkolem("Q_" + x.toString(), nm->booleanType());
    d_qNodes[x] = v;
    return v;
  }

  void registerGroundTermsFromL(const std::vector<Node>& lits,
                                eq::EqualityEngine* ee);

  std::map<Node, std::vector<std::pair<Node, Node>>> computeSignatures(
      eq::EqualityEngine* ee);
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
