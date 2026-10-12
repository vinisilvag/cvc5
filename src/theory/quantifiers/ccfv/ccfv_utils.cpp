#include "theory/quantifiers/ccfv/ccfv_utils.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {
namespace ccfv {

Node computeNnf(NodeManager* nm, Node n, bool pol)
{
  switch (n.getKind())
  {
    case Kind::NOT:
    {
      return computeNnf(nm, n[0], !pol);
    }
    // AND and OR: applies DeMorgan and flattening to avoid nesting
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

  Node nnf = computeNnf(nm, q, /*pol=*/true);
  if (nnf.isNull())
  {
    return false;
  }

  Node prenexBody = computePrenex(nm, nnf, vars);
  if (prenexBody.isNull())
  {
    return false;
  }

  clauses = toCnf(prenexBody);
  return true;
}

CcfvIndices::CcfvIndices(eq::EqualityEngine* ee) {}

std::vector<Node> CcfvIndices::getDiseqs(Node t) { return {}; }

std::vector<Node> CcfvIndices::getReps(Node f) { return {}; }

std::vector<Node> CcfvIndices::getApps(Node f, Node t) { return {}; }

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
