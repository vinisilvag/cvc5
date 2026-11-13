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
Node computeNnf(NodeManager* nm, TNode n, bool pol)
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

      for (TNode nn : n)
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

      for (TNode nn : n)
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
        return pol ? Node(n) : n.negate();
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
        return pol ? Node(n) : n.negate();
      }
      return Node::null();
    }
  }
}

Node skolemize(NodeManager* nm, TNode n, std::vector<Node>& freeVars)
{
  switch (n.getKind())
  {
    case Kind::FORALL:
    {
      std::vector<Node> newFreevars = freeVars;
      for (TNode v : n[0])
      {
        newFreevars.push_back(v);
      }
      Node newBody = skolemize(nm, n[1], newFreevars);
      return nm->mkNode(Kind::FORALL, n[0], newBody);
    }
    case Kind::EXISTS:
    {
      std::vector<TypeNode> argTypes;
      for (TNode v : freeVars)
      {
        argTypes.push_back(v.getType());
      }

      std::vector<Node> vars;
      std::vector<Node> skolemTerms;
      for (TNode v : n[0])
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
      for (TNode child : n)
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

Node computePrenex(NodeManager* nm, TNode n, std::vector<Node>& vars)
{
  switch (n.getKind())
  {
    case Kind::FORALL:
    {
      for (TNode v : n[0])
      {
        vars.push_back(v);
      }
      return computePrenex(nm, n[1], vars);
    }
    case Kind::AND:
    case Kind::OR:
    {
      std::vector<Node> children;
      for (TNode child : n)
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

std::vector<std::vector<Node>> toCnf(TNode n)
{
  switch (n.getKind())
  {
    case Kind::AND:
    {
      std::vector<std::vector<Node>> result;
      for (TNode child : n)
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

bool computeQuantCnf(TNode q,
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

bool isSupportedTriggerTerm(TNode n)
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
  for (TNode rep : allReps)
  {
    eq::EqClassIterator eqc_i(rep, ee);
    while (!eqc_i.isFinished())
    {
      TNode term = *eqc_i;
      if (term.getKind() == Kind::APPLY_UF)
      {
        TNode f = term.getOperator();

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

const std::vector<Node>& CcfvIndices::getDiseqs(TNode t) const
{
  Node rep;
  if (d_ee->hasTerm(t))
    rep = d_ee->getRepresentative(t);
  else
    rep = t;
  auto it = diseqs.find(rep);
  return (it != diseqs.end()) ? it->second : s_empty;
}

const std::vector<Node>& CcfvIndices::getReps(TNode f) const
{
  auto it = reps.find(f);
  return (it != reps.end()) ? it->second : s_empty;
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

FloatersUnionFind::FloatersUnionFind(const std::vector<Node>& xs)
{
  for (TNode x : xs)
  {
    d_representative[x] = x;
  }
}

Node FloatersUnionFind::find(TNode x)
{
  auto it = d_representative.find(x);
  if (it == d_representative.end())
  {
    d_representative[x] = x;
    return x;
  }

  std::vector<TNode> S = {};
  Node curr = x;
  while (d_representative[curr] != curr)
  {
    S.push_back(curr);
    curr = d_representative[curr];
  }
  for (TNode y : S)
  {
    d_representative[y] = curr;
  }
  return curr;
}

void FloatersUnionFind::merge(TNode x,
                              TNode y,
                              std::vector<std::pair<Node, Node>>& toMerge)
{
  Node repX = find(x);
  Node repY = find(y);
  if (repX == repY)
  {
    return;
  }

  for (size_t i = 0; i < 2; ++i)
  {
    auto itX = d_child[i].find(repX);
    auto itY = d_child[i].find(repY);
    if (itY != d_child[i].end())
    {
      if (itX != d_child[i].end())
      {
        toMerge.push_back({itX->second, itY->second});
      }
    }
    else if (itX != d_child[i].end())
    {
      d_child[i][repY] = itX->second;
    }
  }

  d_representative[repX] = repY;
}

bool FloatersUnionFind::areEquivalent(TNode x, TNode y)
{
  return find(x) == find(y);
}

Node FloatersUnionFind::getChild(TNode rep, size_t index) const
{
  Assert(index < 2) << "Index must be 0 (left child) or 1 (right child)";
  auto it = d_child[index].find(rep);
  if (it == d_child[index].end())
  {
    return Node::null();
  }
  return it->second;
}

void FloatersUnionFind::debugPrint(std::ostream& out)
{
  std::map<Node, std::vector<Node>> eqClasses;
  for (const auto& [node, _] : d_representative)
  {
    Node root = find(node);
    eqClasses[root].push_back(node);
  }

  out << "================= Floater Equivalences (<~>) ================="
      << std::endl;
  if (eqClasses.empty())
  {
    out << "  (No class registered)" << std::endl;
  }

  for (const auto& [root, members] : eqClasses)
  {
    out << "  Class [" << root << "]: { ";
    for (size_t i = 0; i < members.size(); ++i)
    {
      out << members[i] << (i + 1 < members.size() ? ", " : " ");
    }
    out << "}" << std::endl;

    Node c0 = getChild(root, 0);
    Node c1 = getChild(root, 1);
    if (!c0.isNull() || !c1.isNull())
    {
      out << "    Children: " << (c0.isNull() ? "?" : c0.toString()) << " @ "
          << (c1.isNull() ? "?" : c1.toString()) << std::endl;
    }
  }
  out << "=============================================================="
      << std::endl;
}

void FloatersUnionFind::setChild(TNode rep, size_t index, TNode x)
{
  Assert(index < 2) << "Index must be 0 or 1";
  d_child[index][rep] = x;
}

HoccfvUtils::HoccfvUtils(const std::vector<Node>& freeVars,
                         const std::vector<Node>& lits)
    : d_dsu(std::make_unique<FloatersUnionFind>(freeVars)),
      d_freeVars(freeVars),
      d_lits(lits)
{
}

void HoccfvUtils::computeFloaterEquivalences()
{
  std::vector<std::pair<Node, Node>> toMerge = {};

  for (TNode lit : d_lits)
  {
    if (lit.getKind() == Kind::NOT)
    {
      continue;
    }
    TNode l = lit[0], r = lit[1];
    if (l.getKind() == Kind::HO_APPLY && r.getKind() == Kind::HO_APPLY)
    {
      continue;
    }

    TNode u0 = (l.getKind() == Kind::BOUND_VARIABLE) ? l : r;
    TNode u = (l.getKind() == Kind::BOUND_VARIABLE) ? r : l;

    if (u0.getKind() != Kind::BOUND_VARIABLE || u.getKind() != Kind::HO_APPLY)
    {
      continue;
    }

    Node rep0 = d_dsu->find(u0);
    for (size_t i = 0; i < 2; ++i)
    {
      auto existingChild = d_dsu->getChild(rep0, i);
      if (!existingChild.isNull())
      {
        if (existingChild != u[i])
        {
          toMerge.push_back({u[i], existingChild});
        }
      }
      else
      {
        d_dsu->setChild(rep0, i, u[i]);
      }
    }
  }

  while (!toMerge.empty())
  {
    std::pair<Node, Node> curr = toMerge.back();
    toMerge.pop_back();
    d_dsu->merge(curr.first, curr.second, toMerge);
  }

  debugPrintFloaterEquivalences();
}

void HoccfvUtils::applyGroundOrTypeConstraints()
{
  for (TNode var : d_freeVars)
  {
    d_floatable[var] = true;
  }

  for (TNode lit : d_lits)
  {
    if (lit.getKind() == Kind::NOT)
    {
      continue;
    }

    Node l = lit[0], r = lit[1];
    if (l.getKind() == Kind::HO_APPLY && r.getKind() == Kind::HO_APPLY)
    {
      continue;
    }

    Node u0 = (l.getKind() == Kind::BOUND_VARIABLE) ? l : r;
    Node u = (l.getKind() == Kind::BOUND_VARIABLE) ? r : l;

    if (u0.getKind() != Kind::BOUND_VARIABLE || u.getKind() != Kind::HO_APPLY)
    {
      continue;
    }

    Node x = d_dsu->find(u0);
    for (size_t i = 0; i < 2; ++i)
    {
      bool isGround = !expr::hasFreeVar(u[i]);
      TNode childNode = d_dsu->getChild(x, i);

      if (!childNode.isNull())
      {
        Node y = d_dsu->find(childNode);
        if (y.getType() != u[i].getType() || isGround)
        {
          d_floatable[y] = false;
        }
      }

      if (isGround)
      {
        Node repUi = d_dsu->find(u[i]);
        auto it = d_ground[i].find(x);
        if (it != d_ground[i].end())
        {
          d_floatable[x] = d_floatable[x] && (repUi == it->second);
        }
        else
        {
          d_ground[i][x] = u[i];
        }
      }
    }
  }

  debugPrintGroundOrTypeConstraints();
}

void HoccfvUtils::debugPrintGroundOrTypeConstraints(std::ostream& out)
{
  out << "=========== Ground and Type Constraints Report ==========="
      << std::endl;

  std::map<Node, std::vector<Node>> classes;
  for (TNode var : d_freeVars)
  {
    classes[d_dsu->find(var)].push_back(var);
  }

  if (classes.empty())
  {
    out << "  (No variable registered)" << std::endl;
  }

  for (const auto& [root, members] : classes)
  {
    bool floatable = isFloatable(root);

    out << "Classe [" << root << "] "
        << (floatable ? "[FLOATABLE]" : "[BLOCKED: NOT FLOATABLE]")
        << std::endl;
    out << "  - Type: " << root.getType() << std::endl;
    out << "  - Members: { ";
    for (size_t i = 0; i < members.size(); ++i)
    {
      out << members[i] << (i + 1 < members.size() ? ", " : " ");
    }
    out << "}" << std::endl;

    Node c0 = d_dsu->getChild(root, 0);
    Node c1 = d_dsu->getChild(root, 1);
    if (!c0.isNull() || !c1.isNull())
    {
      out << "  - Structured children: (" << (c0.isNull() ? "?" : c0.toString())
          << " @ " << (c1.isNull() ? "?" : c1.toString()) << ")" << std::endl;
    }

    auto itG0 = d_ground[0].find(root);
    auto itG1 = d_ground[1].find(root);
    if (itG0 != d_ground[0].end() || itG1 != d_ground[1].end())
    {
      out << "  - Ground fixed children: ("
          << (itG0 != d_ground[0].end() ? itG0->second.toString() : "-")
          << " @ "
          << (itG1 != d_ground[1].end() ? itG1->second.toString() : "-") << ")"
          << std::endl;
    }
    out << "----------------------------------------------------------"
        << std::endl;
  }
  out << "=========================================================="
      << std::endl;
}

Node HoccfvUtils::getGround(TNode rep, size_t index) const
{
  Assert(index < 2) << "Index must be 0 (left child) or 1 (right child)";
  auto it = d_ground[index].find(rep);
  if (it == d_ground[index].end())
  {
    return Node::null();
  }
  return it->second;
}

void HoccfvUtils::setGround(TNode rep, size_t index, TNode x)
{
  Assert(index < 2) << "Index must be 0 or 1";
  d_ground[index][rep] = x;
}

void HoccfvUtils::computeFloaterDependencyGraph()
{
  std::map<Node, size_t> index;
  std::map<Node, size_t> low;
  std::map<Node, bool> onStack;

  for (TNode var : d_freeVars)
  {
    Node root = d_dsu->find(var);
    for (size_t i = 0; i < 2; ++i)
    {
      Node c = d_dsu->getChild(root, i);
      if (!c.isNull())
      {
        d_dsu->setChild(root, i, d_dsu->find(c));
      }
    }

    index[root] = 0;
    low[root] = 0;
    onStack[root] = false;
  }

  size_t counter = 1;
  std::vector<Node> S = {};

  std::function<void(TNode)> strongConnect = [&](TNode x) {
    index[x] = counter;
    low[x] = counter;
    onStack[x] = true;
    S.push_back(x);
    counter++;

    for (size_t i = 0; i < 2; ++i)
    {
      Node y = d_dsu->getChild(x, i);
      if (!y.isNull())
      {
        if (index.find(y) != index.end())
        {
          if (index[y] == 0)
          {
            strongConnect(y);
            low[x] = std::min(low[x], low[y]);
          }
          else if (onStack[y])
          {
            low[x] = std::min(low[x], index[y]);
          }
        }
      }
    }

    if (low[x] == index[x])
    {
      bool hasSelfLoop =
          (d_dsu->getChild(x, 0) == x) || (d_dsu->getChild(x, 1) == x);
      bool isSccFloatable = isFloatable(x) && !hasSelfLoop;

      Node y = S.back();
      S.pop_back();

      while (y != x)
      {
        isSccFloatable = false;
        onStack[y] = false;
        d_floatable[y] = false;

        y = S.back();
        S.pop_back();
      }

      onStack[x] = false;
      d_floatable[x] = isSccFloatable;
    }
  };

  for (TNode var : d_freeVars)
  {
    Node root = d_dsu->find(var);
    if (index[root] == 0)
    {
      strongConnect(root);
    }
  }
}

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
