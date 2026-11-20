/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Implementation of CCFV preprocessing: Negation Normal Form (NNF),
 * universal quantifier prenexing, and classical distributive CNF expansion.
 */

#include "theory/quantifiers/ccfv/ccfv_cnf.h"

#include <cstdint>
#include <unordered_map>
#include <utility>

#include "base/output.h"
#include "expr/node_algorithm.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {
namespace ccfv {

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

/** Number of clauses and literals of the CNF of a formula. */
struct CnfSize
{
  uint64_t d_clauses;
  uint64_t d_lits;
};

/**
 * Computes the size of the CNF that toCnf would produce for n, without
 * building it. It mirrors toCnf: AND concatenates the clauses of its children,
 * OR distributes (cartesian product), and anything else is a single literal.
 * The results are memoized, as n is a DAG.
 */
CnfSize cnfSize(TNode n, std::unordered_map<Node, CnfSize>& memo)
{
  auto it = memo.find(n);
  if (it != memo.end()) return it->second;

  CnfSize res{1, 1};
  switch (n.getKind())
  {
    case Kind::AND:
    {
      res = {0, 0};
      for (TNode child : n)
      {
        CnfSize c = cnfSize(child, memo);
        res.d_clauses += c.d_clauses;
        res.d_lits += c.d_lits;
      }
      break;
    }
    case Kind::OR:
    {
      if (n.getNumChildren() == 0)
      {
        res = {0, 0};
        break;
      }
      res = cnfSize(n[0], memo);
      for (size_t i = 1; i < n.getNumChildren(); ++i)
      {
        CnfSize c = cnfSize(n[i], memo);
        // Every accumulated clause is combined with every clause of the
        // next disjunct: old literals repeat c.d_clauses times and new
        // literals repeat res.d_clauses times
        res.d_lits = res.d_lits * c.d_clauses + res.d_clauses * c.d_lits;
        res.d_clauses *= c.d_clauses;
      }
      break;
    }
    default: break;
  }
  memo.emplace(n, res);
  return res;
}

bool computeQuantCnf(TNode q,
                     std::vector<Node>& vars,
                     std::vector<std::vector<Node>>& clauses,
                     uint64_t cnfLiteralLimit)
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

  // Guard against the exponential blow-up of the distributivity
  if (cnfLiteralLimit != 0)
  {
    std::unordered_map<Node, CnfSize> memo;
    if (cnfSize(prenexBody, memo).d_lits > cnfLiteralLimit)
    {
      Trace("ccfv-cnf") << "CNF of " << q << " exceeds " << cnfLiteralLimit
                        << " literals, skipping" << std::endl;
      return false;
    }
  }

  // Distributivity
  clauses = toCnf(prenexBody);

  return true;
}

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
