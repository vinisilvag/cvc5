/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Implementation of the CCFV indices over the equality engine.
 */

#include "theory/quantifiers/ccfv/ccfv_indices.h"

#include <set>

#include "theory/uf/equality_engine.h"
#include "theory/uf/equality_engine_iterator.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {
namespace ccfv {

CcfvIndices::CcfvIndices(eq::EqualityEngine* ee,
                         const std::function<bool(TNode)>& isRelevant)
    : d_ee(ee)
{
  for (eq::EqClassesIterator eqcs_i(ee); !eqcs_i.isFinished(); ++eqcs_i)
  {
    Node rep = *eqcs_i;
    bool relevantClass = false;
    // One term per signature (operator and representatives of the
    // arguments). Congruent terms always share a class, so the set is local
    std::set<std::pair<Node, std::vector<Node>>> signatures;
    for (eq::EqClassIterator eqc_i(rep, ee); !eqc_i.isFinished(); ++eqc_i)
    {
      TNode term = *eqc_i;
      if (isRelevant && !isRelevant(term))
      {
        continue;
      }
      relevantClass = true;
      if (term.getKind() != Kind::APPLY_UF)
      {
        continue;
      }
      Node f = term.getOperator();
      std::vector<Node> sig;
      for (TNode child : term)
      {
        sig.push_back(ee->hasTerm(child) ? Node(ee->getRepresentative(child))
                                         : Node(child));
      }
      if (!signatures.insert({f, sig}).second)
      {
        // congruent to a term already indexed
        continue;
      }
      allApps[f].push_back(term);
      auto& app = apps[{f, rep}];
      app.push_back(term);
      if (app.size() == 1)
      {
        reps[f].push_back(rep);
      }
    }
    if (relevantClass)
    {
      repsByType[rep.getType()].push_back(rep);
    }
  }
}

const std::vector<Node> CcfvIndices::s_empty = {};

const std::vector<Node>& CcfvIndices::getDiseqs(TNode t) const
{
  Node rep;
  if (d_ee->hasTerm(t))
    rep = d_ee->getRepresentative(t);
  else
    rep = t;

  if (diseqsDone.insert(rep).second)
  {
    auto bt = repsByType.find(rep.getType());
    if (bt != repsByType.end())
    {
      std::vector<Node>& out = diseqs[rep];
      for (const Node& other : bt->second)
      {
        if (other != rep && d_ee->areDisequal(rep, other, false))
          out.push_back(other);
      }
    }
  }
  auto it = diseqs.find(rep);
  return (it != diseqs.end()) ? it->second : s_empty;
}

const std::vector<Node>& CcfvIndices::getReps(TNode f) const
{
  auto it = reps.find(f);
  return (it != reps.end()) ? it->second : s_empty;
}

const std::vector<Node>& CcfvIndices::getRepsOfType(TypeNode tn) const
{
  auto it = repsByType.find(tn);
  return (it != repsByType.end()) ? it->second : s_empty;
}

const std::vector<Node>& CcfvIndices::getApps(TNode f) const
{
  auto it = allApps.find(f);
  return (it != allApps.end()) ? it->second : s_empty;
}

const std::vector<Node>& CcfvIndices::getApps(TNode f, TNode t) const
{
  Node rep;
  if (d_ee->hasTerm(t))
    rep = d_ee->getRepresentative(t);
  else
    rep = t;
  auto it = apps.find({f, rep});
  return (it != apps.end()) ? it->second : s_empty;
}

Node CcfvIndices::fallbackForType(const TypeNode& tn) const
{
  auto it = fallbackTermCache.find(tn);
  if (it != fallbackTermCache.end()) return it->second;

  Node groundTerm;
  auto bt = repsByType.find(tn);
  if (bt != repsByType.end() && !bt->second.empty())
  {
    groundTerm = bt->second.front();
  }
  else
  {
    for (eq::EqClassesIterator e(d_ee); !e.isFinished(); ++e)
    {
      if ((*e).getType() == tn)
      {
        groundTerm = *e;
        break;
      }
    }
  }
  if (groundTerm.isNull()) groundTerm = NodeManager::mkGroundTerm(tn);

  return fallbackTermCache[tn] = groundTerm;
}

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
