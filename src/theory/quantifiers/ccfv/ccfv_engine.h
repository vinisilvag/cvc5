/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Congruence Closure with Free Variables (CCFV) quantifier instantiation
 * engine.
 */

#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_ENGINE_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_ENGINE_H

#include <memory>

#include "theory/quantifiers/quant_module.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

class CcfvSolver;

/**
 * Congruence Closure with Free Variables (CCFV) instantiation engine.
 *
 * This class is a QuantifiersModule that drives quantifier instantiation
 * using CCFV-based E-ground (dis)unification. It manages:
 * - Conflict-based instantiation (at QEFFORT_CONFLICT): attempts to find
 *   substitutions that falsify the body of asserted quantified formulas under
 *   the current equality engine E.
 * - Trigger-based instantiation (at QEFFORT_STANDARD): solves matching
 *   equations between selected pattern terms and candidate ground terms in E.
 * - Model-based instantiation (at QEFFORT_MODEL): checks whether candidate
 *   models satisfy the asserted quantifiers.
 */
class CcfvEngine : public QuantifiersModule
{
 public:
  CcfvEngine(Env& env,
             QuantifiersState& qs,
             QuantifiersInferenceManager& qim,
             QuantifiersRegistry& qr,
             TermRegistry& tr);
  ~CcfvEngine() override;

  /** Called when a new quantifier is registered */
  void registerQuantifier(Node q) override;

  /** Decides when cvc5 should call this module */
  bool needsCheck(Theory::Effort level) override;

  /** Decides at what effort level this module needs a model to be built */
  QEffort needsModel(Theory::Effort level) override;

  /** Core check function called at each quantifier effort level */
  void check(Theory::Effort level, QEffort quant_e) override;

  std::string identify() const override { return "CcfvEngine"; }

 private:
  /** Custom Conflict-based instantiation implementation */
  void checkConflictInst(Theory::Effort level);

  /** Custom Trigger-based instantiation implementation */
  void checkTriggerInst(Theory::Effort level);

  /** Custom Model-based instantiation implementation */
  void checkModelInst(Theory::Effort level);

  /** CCFV solver instance for solving E-ground (dis)unification */
  std::unique_ptr<CcfvSolver> d_solver;
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
