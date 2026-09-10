/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * TODO: add description later.
 */

#include "cvc5_private.h"

#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_SAT_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_SAT_H

#include <vector>

#include "expr/node.h"
#include "smt/env_obj.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

class QuantifiersState;
class TermRegistry;

// TODO: add class description later
class CcfvSatEngine : protected EnvObj
{
 public:
  CcfvSatEngine(Env& env, QuantifiersState& qs, TermRegistry& tr);
  ~CcfvSatEngine() = default;

  bool findSubstitutions(Node q,
                         const std::vector<Node>& L,
                         std::vector<std::vector<Node>>& substitutions);

 private:
  QuantifiersState& d_qstate;
  TermRegistry& d_treg;
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif /* CVC5__THEORY__QUANTIFIERS__CCFV_SAT_H */
