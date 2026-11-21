/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Indices over the equality engine E used by CCFV: representatives by type,
 * applications by operator and representative, and (lazy) disequalities.
 */

#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_INDICES_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_INDICES_H

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "expr/node.h"
#include "expr/type_node.h"
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

class CcfvIndices
{
 public:
  // isRelevant (optional): ground terms for which it returns false are not
  // indexed (as TermDb does); by default every term of the EE is indexed
  CcfvIndices(eq::EqualityEngine* ee,
              const std::function<bool(TNode)>& isRelevant = nullptr);
  const std::vector<Node>& getDiseqs(TNode t) const;
  const std::vector<Node>& getReps(TNode f) const;
  const std::vector<Node>& getRepsOfType(TypeNode tn) const;
  const std::vector<Node>& getApps(TNode f) const;
  const std::vector<Node>& getApps(TNode f, TNode t) const;
  Node fallbackForType(const TypeNode& tn) const;

 private:
  eq::EqualityEngine* d_ee;
  std::unordered_map<TypeNode, std::vector<Node>> repsByType;
  std::unordered_map<Node, std::vector<Node>> allApps;
  mutable std::unordered_map<Node, std::vector<Node>> diseqs;
  mutable std::unordered_set<Node> diseqsDone;
  std::unordered_map<std::pair<Node, Node>, std::vector<Node>, NodePairHash>
      apps;
  std::unordered_map<Node, std::vector<Node>> reps;
  mutable std::unordered_map<TypeNode, Node> fallbackTermCache;

  static const std::vector<Node> s_empty;
};

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
