#include "theory/quantifiers/ccfv/ccfv_utils.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {
namespace ccfv {

// TODO: implement it
std::vector<Node> toCnf(NodeManager* nm, Node body)
{
  // Eliminate implications and bi-implications (and other operators)
  // NNF
  // Distributive
}

CcfvIndices::CcfvIndices(eq::EqualityEngine* ee) {}

std::vector<Node> CcfvIndices::getDiseqs(Node t) { return {}; }

std::vector<Node> CcfvIndices::getReps(Node f) { return {}; }

std::vector<Node> CcfvIndices::getApps(Node f, Node t) { return {}; }

}  // namespace ccfv
}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
