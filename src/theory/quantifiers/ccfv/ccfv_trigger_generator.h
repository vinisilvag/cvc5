/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Generation of the triggers used by CCFV for trigger-based instantiation,
 * following cvc5's InstStrategyUserPatterns and InstStrategyAutoGenTriggers.
 */

#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_TRIGGER_GENERATOR_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_TRIGGER_GENERATOR_H

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "expr/node.h"
#include "options/quantifiers_options.h"
#include "smt/env_obj.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

class QuantifiersRegistry;
class QuantifiersState;

/** A trigger, as used by CCFV. */
struct CcfvTriggerEntry
{
  /** The trigger terms, with bound variables. */
  std::vector<Node> d_pats;
  /** The selected patterns (instantiation constants), sorted: its key. */
  std::vector<Node> d_key;
  /** The preprocessed ground subterms of the trigger. */
  std::vector<Node> d_groundTerms;
  /** The last check in which this trigger was processed. */
  uint64_t d_round = 0;
};

/**
 * The triggers of a quantified formula. Mirrors the information kept by
 * cvc5's InstStrategyUserPatterns and InstStrategyAutoGenTriggers.
 */
struct CcfvQuantTriggers
{
  /** Whether the quantified formula has user-provided patterns. */
  bool d_hasUserPatterns = false;
  /** The user triggers. */
  std::vector<CcfvTriggerEntry> d_user;
  /** The terms that must not be used in triggers (no-patterns). */
  std::vector<Node> d_noPatterns;
  /** Whether the pattern terms were already generated. */
  bool d_patternsGenerated = false;
  /** Whether triggers were already made. */
  bool d_madeTriggers = false;
  /** The number of rounds the automatic triggers were processed, -1 if
   * none. Used to regenerate triggers. */
  int d_counter = -1;
  /** The number of variables covered by the pattern terms. */
  size_t d_numTriggerVars = 0;
  /** The pattern terms (instantiation constants): [0] those with all
   * variables (single triggers), [1] the others (multi-triggers). */
  std::vector<Node> d_pool[2];
  /** The automatic triggers: [0] single, [1] multi. */
  std::vector<CcfvTriggerEntry> d_auto[2];
};

/**
 * Generates the triggers of quantified formulas with the same selection as
 * cvc5's E-matching: user patterns, and automatic triggers (single triggers,
 * and a multi-trigger that is regenerated during the search).
 */
class CcfvTriggerGenerator : protected EnvObj
{
 public:
  CcfvTriggerGenerator(Env& env, QuantifiersState& qs, QuantifiersRegistry& qr);

  /** The triggers of q, registering its user patterns the first time. */
  CcfvQuantTriggers& getQuantTriggers(Node q);

  /** Same as InstStrategyAutoGenTriggers::generateTriggers. */
  void generateTriggers(Node q, CcfvQuantTriggers& qt);

  /** Same as InstStrategy::getInstUserPatMode. */
  options::UserPatMode getUserPatMode() const;

 private:
  /** Same as InstStrategyAutoGenTriggers::generatePatternTerms. */
  bool generatePatternTerms(Node q, CcfvQuantTriggers& qt);
  /** Adds an automatic trigger of pool r, if it is new and supported. */
  void addAutoTrigger(Node q,
                      CcfvQuantTriggers& qt,
                      size_t r,
                      const std::vector<Node>& icPats);
  /**
   * Makes a trigger from patterns in instantiation constant form: ground
   * subterms are preprocessed and the terms are converted to bound variables.
   * @return false if CCFV does not support some term.
   */
  bool makeTrigger(Node q,
                   const std::vector<Node>& icPats,
                   CcfvTriggerEntry& tr);

  QuantifiersState& d_qstate;
  QuantifiersRegistry& d_qreg;
  /** The triggers of each quantified formula. */
  std::unordered_map<Node, CcfvQuantTriggers> d_qTriggers;
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
