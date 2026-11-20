/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Implementation of CCFV utilities: trigger term checks, groundness test and
 * higher-order floater utilities.
 */

#include "theory/quantifiers/ccfv/ccfv_utils.h"

#include <unordered_set>

#include "expr/attribute.h"
#include "expr/node_algorithm.h"
#include "theory/uf/equality_engine.h"
#include "theory/uf/equality_engine_iterator.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {
namespace ccfv {

namespace {
struct CcfvGroundTag
{
};
struct CcfvGroundComputedTag
{
};
typedef expr::Attribute<CcfvGroundTag, bool> CcfvGroundAttr;
typedef expr::Attribute<CcfvGroundComputedTag, bool> CcfvGroundComputedAttr;
}  // namespace

bool isGround(TNode n)
{
  if (!n.getAttribute(CcfvGroundComputedAttr()))
  {
    n.setAttribute(CcfvGroundAttr(), !expr::hasFreeVar(n));
    n.setAttribute(CcfvGroundComputedAttr(), true);
  }
  return n.getAttribute(CcfvGroundAttr());
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
