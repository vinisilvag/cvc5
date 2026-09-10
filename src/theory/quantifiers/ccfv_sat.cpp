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

#include "theory/quantifiers/ccfv_sat.h"

#include "options/quantifiers_options.h"
#include "theory/quantifiers/quantifiers_state.h"
#include "theory/quantifiers/term_registry.h"
#include "theory/uf/equality_engine_iterator.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

CcfvSatEngine::CcfvSatEngine(Env& env, QuantifiersState& qs, TermRegistry& tr)
    : EnvObj(env), d_qstate(qs), d_treg(tr)
{
}

bool CcfvSatEngine::findSubstitutions(
    Node q,
    const std::vector<Node>& L,
    std::vector<std::vector<Node>>& substitutions)
{
  std::cout << "[CcfvSatEngine] Starting the search for substitutions of\n  "
            << q << std::endl;

  std::cout << "\n[CcfvSatEngine] L free variables (" << q[0].getNumChildren()
            << "):" << std::endl;
  for (const Node& v : q[0])
  {
    std::cout << "  " << v << " : " << v.getType() << std::endl;
  }

  std::cout << "\n[CcfvSatEngine] L (" << L.size()
            << " literals):" << std::endl;
  for (const Node& lit : L)
  {
    std::cout << "  " << lit << std::endl;
  }

  std::cout << "\n[CcfvSatEngine] E: " << std::endl;
  eq::EqualityEngine* ee = d_qstate.getEqualityEngine();
  eq::EqClassesIterator eqcs_i(ee);
  while (!eqcs_i.isFinished())
  {
    Node rep = *eqcs_i;
    std::cout << "  Class [" << rep << "] : {";
    eq::EqClassIterator eqc_i(rep, ee);
    bool first = true;
    while (!eqc_i.isFinished())
    {
      if (!first) std::cout << ", ";
      std::cout << *eqc_i;
      first = false;
      ++eqc_i;
    }
    std::cout << "}" << std::endl;
    ++eqcs_i;
  }

  std::cout << "\n=====================================================\n"
            << std::endl;

  std::cout << "[CcfvSatEngine] Starting SAT encode" << std::endl;
  // Encode in SAT

  std::cout << "[CcfvSatEngine] Calling SAT solver" << std::endl;
  // Call solver
  //
  std::cout << "[CcfvSatEngine] Retrieving model" << std::endl;
  // Get model

  std::cout << "[CcfvSatEngine] Translating model to substitution" << std::endl;
  // Translate model to substitution

  return !substitutions.empty();
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
