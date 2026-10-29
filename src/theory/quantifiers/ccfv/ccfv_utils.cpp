/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Implementation of CCFV preprocessing utilities: Negation Normal Form (NNF),
 * universal quantifier prenexing, and classical distributive CNF expansion.
 */

#include "theory/quantifiers/ccfv/ccfv_utils.h"

#include "expr/node_algorithm.h"
#include "theory/uf/equality_engine.h"
#include "theory/uf/equality_engine_iterator.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {
namespace ccfv {

// TODO: Tseitin transformation? Check with Haniel later
Node computeNnf(NodeManager* nm, Node n, bool pol)
{
  switch (n.getKind())
  {
    // Double negation cancellation: alternating polarity bit flips sign
    // directly
    case Kind::NOT:
    {
      return computeNnf(nm, n[0], !pol);
    }
    // AND and OR: applies De Morgan's duality and flattens nested same-kind
    // nodes to avoid quadratic recursion during distributivity
    case Kind::AND:
    {
      Kind targetKind = pol ? Kind::AND : Kind::OR;
      std::vector<Node> children;

      for (const Node& nn : n)
      {
        Node nnf = computeNnf(nm, nn, pol);
        if (nnf.isNull())
        {
          return Node::null();
        }

        // Inline grandchildren if child matches the parent's target connective
        // (flattening)
        if (nnf.getKind() == targetKind)
        {
          children.insert(children.end(), nnf.begin(), nnf.end());
        }
        else
        {
          children.push_back(nnf);
        }
      }

      return targetKind == Kind::AND ? nm->mkAnd(children) : nm->mkOr(children);
    }
    case Kind::OR:
    {
      Kind targetKind = pol ? Kind::OR : Kind::AND;
      std::vector<Node> children;

      for (const Node& nn : n)
      {
        Node nnf = computeNnf(nm, nn, pol);
        if (nnf.isNull())
        {
          return Node::null();
        }

        if (nnf.getKind() == targetKind)
        {
          children.insert(children.end(), nnf.begin(), nnf.end());
        }
        else
        {
          children.push_back(nnf);
        }
      }

      return targetKind == Kind::AND ? nm->mkAnd(children) : nm->mkOr(children);
    }
    case Kind::XOR:
    {
      return computeNnf(nm, nm->mkNode(Kind::EQUAL, n[0], n[1]), !pol);
    }
    case Kind::IMPLIES:
    {
      Node a = computeNnf(nm, n[0], !pol);
      Node b = computeNnf(nm, n[1], pol);
      if (a.isNull() || b.isNull())
      {
        return Node::null();
      }

      return pol ? nm->mkOr(std::vector<Node>{a, b})
                 : nm->mkAnd(std::vector<Node>{a, b});
    }
    case Kind::EQUAL:
    {
      if (!n[0].getType().isBoolean())
      {
        return pol ? n : n.negate();
      }

      Node aPos = computeNnf(nm, n[0], true);
      Node aNeg = computeNnf(nm, n[0], false);
      Node bPos = computeNnf(nm, n[1], true);
      Node bNeg = computeNnf(nm, n[1], false);

      if (aPos.isNull() || aNeg.isNull() || bPos.isNull() || bNeg.isNull())
      {
        return Node::null();
      }

      if (pol)
      {
        // (¬A ∨ B) ∧ (¬B ∨ A)
        Node l = nm->mkOr(std::vector<Node>{aNeg, bPos});
        Node r = nm->mkOr(std::vector<Node>{bNeg, aPos});
        return nm->mkAnd(std::vector<Node>{l, r});
      }
      else
      {
        // (A ∧ ¬B) ∨ (¬A ∧ B)
        Node l = nm->mkAnd(std::vector<Node>{aPos, bNeg});
        Node r = nm->mkAnd(std::vector<Node>{aNeg, bPos});
        return nm->mkOr(std::vector<Node>{l, r});
      }
    }
    case Kind::ITE:
    {
      if (!n.getType().isBoolean())
      {
        return Node::null();
      }

      Node cPos = computeNnf(nm, n[0], true);
      Node cNeg = computeNnf(nm, n[0], false);
      if (cPos.isNull() || cNeg.isNull())
      {
        return Node::null();
      }

      if (pol)
      {
        Node tPos = computeNnf(nm, n[1], true);
        Node ePos = computeNnf(nm, n[2], true);
        if (tPos.isNull() || ePos.isNull())
        {
          return Node::null();
        }

        // (¬C ∨ T) ∧ (C ∨ E)
        Node l = nm->mkOr(std::vector<Node>{cNeg, tPos});
        Node r = nm->mkOr(std::vector<Node>{cPos, ePos});
        return nm->mkAnd(std::vector<Node>{l, r});
      }
      else
      {
        Node tNeg = computeNnf(nm, n[1], false);
        Node eNeg = computeNnf(nm, n[2], false);
        if (tNeg.isNull() || eNeg.isNull())
        {
          return Node::null();
        }

        // (C ∧ ¬T) ∨ (¬C ∧ ¬E)
        Node l = nm->mkAnd(std::vector<Node>{cPos, tNeg});
        Node r = nm->mkAnd(std::vector<Node>{cNeg, eNeg});
        return nm->mkOr(std::vector<Node>{l, r});
      }
    }
    case Kind::FORALL:
    case Kind::EXISTS:
    {
      Kind targetKind = pol                             ? n.getKind()
                        : (n.getKind() == Kind::FORALL) ? Kind::EXISTS
                                                        : Kind::FORALL;

      Node body = computeNnf(nm, n[1], pol);
      if (body.isNull())
      {
        return Node::null();
      }

      if (n.getNumChildren() == 3)
      {
        return nm->mkNode(targetKind, n[0], body, n[2]);
      }
      return nm->mkNode(targetKind, n[0], body);
    }
    default:
    {
      if (isLiteral(n))
      {
        return pol ? n : n.negate();
      }
      return Node::null();
    }
  }
}

Node skolemize(NodeManager* nm, Node n, std::vector<Node>& freeVars)
{
  switch (n.getKind())
  {
    case Kind::FORALL:
    {
      std::vector<Node> newFreevars = freeVars;
      for (const Node& v : n[0])
      {
        newFreevars.push_back(v);
      }
      Node newBody = skolemize(nm, n[1], newFreevars);
      return nm->mkNode(Kind::FORALL, n[0], newBody);
    }
    case Kind::EXISTS:
    {
      std::vector<TypeNode> argTypes;
      for (const Node& v : freeVars)
      {
        argTypes.push_back(v.getType());
      }

      std::vector<Node> vars;
      std::vector<Node> skolemTerms;
      for (const Node& v : n[0])
      {
        vars.push_back(v);
        if (freeVars.empty())
        {
          skolemTerms.push_back(NodeManager::mkDummySkolem("sk", v.getType()));
        }
        else
        {
          TypeNode fnType = nm->mkFunctionType(argTypes, v.getType());
          Node op = NodeManager::mkDummySkolem("skop", fnType);
          std::vector<Node> appArgs = {op};
          appArgs.insert(appArgs.end(), freeVars.begin(), freeVars.end());
          skolemTerms.push_back(nm->mkNode(Kind::APPLY_UF, appArgs));
        }
      }

      Node skBody = n[1].substitute(
          vars.begin(), vars.end(), skolemTerms.begin(), skolemTerms.end());

      return skolemize(nm, skBody, freeVars);
    }
    case Kind::AND:
    case Kind::OR:
    {
      std::vector<Node> children;
      for (const Node& child : n)
      {
        children.push_back(skolemize(nm, child, freeVars));
      }
      return nm->mkNode(n.getKind(), children);
    }
    default:
    {
      return n;
    }
  }
}

Node computePrenex(NodeManager* nm, Node n, std::vector<Node>& vars)
{
  switch (n.getKind())
  {
    case Kind::FORALL:
    {
      for (const Node& v : n[0])
      {
        vars.push_back(v);
      }
      return computePrenex(nm, n[1], vars);
    }
    case Kind::AND:
    case Kind::OR:
    {
      std::vector<Node> children;
      for (const Node& child : n)
      {
        Node newChild = computePrenex(nm, child, vars);
        if (newChild.isNull())
        {
          return Node::null();
        }
        children.push_back(newChild);
      }
      return n.getKind() == Kind::AND ? nm->mkAnd(children)
                                      : nm->mkOr(children);
    }
    default:
    {
      return n;
    }
  }
}

std::vector<std::vector<Node>> toCnf(Node n)
{
  switch (n.getKind())
  {
    case Kind::AND:
    {
      std::vector<std::vector<Node>> result;
      for (const Node& child : n)
      {
        std::vector<std::vector<Node>> childCnf = toCnf(child);
        result.insert(result.end(), childCnf.begin(), childCnf.end());
      }
      return result;
    }
    case Kind::OR:
    {
      if (n.getNumChildren() == 0)
      {
        return {};
      }

      std::vector<std::vector<Node>> current = toCnf(n[0]);
      for (size_t i = 1; i < n.getNumChildren(); ++i)
      {
        std::vector<std::vector<Node>> next = toCnf(n[i]);
        std::vector<std::vector<Node>> merged;

        // Merge every accumulated clause with every clause of the next disjunct
        for (const auto& c1 : current)
        {
          for (const auto& c2 : next)
          {
            std::vector<Node> combined = c1;
            combined.insert(combined.end(), c2.begin(), c2.end());
            merged.push_back(std::move(combined));
          }
        }
        current = std::move(merged);
      }
      return current;
    }
    default:
    {
      return {{n}};
    }
  }
}

bool computeQuantCnf(Node q,
                     std::vector<Node>& vars,
                     std::vector<std::vector<Node>>& clauses)
{
  NodeManager* nm = q.getNodeManager();

  Node nnf = computeNnf(nm, q, true);
  if (nnf.isNull())
  {
    return false;
  }

  // Pre-Skolemization to avoid unnecessary dependencies
  std::vector<Node> freeVars;
  Node skolemizedBody = skolemize(nm, nnf, freeVars);
  if (skolemizedBody.isNull())
  {
    return false;
  }

  // Prenexing to lift for all's to the left
  Node prenexBody = computePrenex(nm, skolemizedBody, vars);
  if (prenexBody.isNull())
  {
    return false;
  }

  // Distributivity
  clauses = toCnf(prenexBody);
  return true;
}

bool isSupportedTriggerTerm(Node n)
{
  if (n.getKind() != Kind::APPLY_UF)
  {
    return false;
  }

  std::vector<TNode> visit = {n};
  std::unordered_set<TNode> visited;
  while (!visit.empty())
  {
    TNode cur = visit.back();
    visit.pop_back();
    if (!visited.insert(cur).second)
    {
      continue;
    }
    if (cur.isVar() || !expr::hasBoundVar(cur))
    {
      continue;
    }
    // Trigger killer
    if (cur.getKind() != Kind::APPLY_UF)
    {
      return false;
    }
    visit.insert(visit.end(), cur.begin(), cur.end());
  }

  return true;
}

CcfvIndices::CcfvIndices(eq::EqualityEngine* ee)
{
  d_ee = ee;

  std::vector<Node> allReps;
  for (eq::EqClassesIterator eqcs_i(ee); !eqcs_i.isFinished(); ++eqcs_i)
  {
    allReps.push_back(*eqcs_i);
  }

  // Compute `diseqs`
  for (size_t i = 0; i < allReps.size(); ++i)
  {
    for (size_t j = i + 1; j < allReps.size(); ++j)
    {
      if ((allReps[i].getType() == allReps[j].getType())
          && ee->areDisequal(allReps[i], allReps[j], false))
      {
        diseqs[allReps[i]].push_back(allReps[j]);
        diseqs[allReps[j]].push_back(allReps[i]);
      }
    }
  }

  // Compute `apps` and `reps`
  for (const Node& rep : allReps)
  {
    eq::EqClassIterator eqc_i(rep, ee);
    while (!eqc_i.isFinished())
    {
      Node term = *eqc_i;
      if (term.getKind() == Kind::APPLY_UF)
      {
        Node f = term.getOperator();

        // Compute `apps`
        apps[{f, rep}].push_back(term);

        // Compute `reps`
        if (apps[{f, rep}].size() == 1)
        {
          reps[f].push_back(rep);
        }
      }
      ++eqc_i;
    }
  }
}

const std::vector<Node> CcfvIndices::s_empty = {};

const std::vector<Node>& CcfvIndices::getDiseqs(Node t) const
{
  Node rep;
  if (d_ee->hasTerm(t))
    rep = d_ee->getRepresentative(t);
  else
    rep = t;
  auto it = diseqs.find(rep);
  return (it != diseqs.end()) ? it->second : s_empty;
}

const std::vector<Node>& CcfvIndices::getReps(Node f) const
{
  auto it = reps.find(f);
  return (it != reps.end()) ? it->second : s_empty;
}

const std::vector<Node>& CcfvIndices::getApps(Node f, Node t) const
{
  Node rep;
  if (d_ee->hasTerm(t))
    rep = d_ee->getRepresentative(t);
  else
    rep = t;
  auto it = apps.find({f, rep});
  return (it != apps.end()) ? it->second : s_empty;
}

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
