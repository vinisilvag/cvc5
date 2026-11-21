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

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "expr/node.h"
#include "theory/quantifiers/ccfv/ccfv_trigger_generator.h"
#include "theory/quantifiers/quant_module.h"
#include "util/statistics_stats.h"

namespace cvc5::internal {
namespace theory {

namespace eq {
class EqualityEngine;
}

namespace quantifiers {

namespace ccfv {
class CcfvIndices;
}

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

  /** Decides when cvc5 should call this module */
  bool needsCheck(Theory::Effort level) override;

  /** Core check function called at each quantifier effort level */
  void check(Theory::Effort level, QEffort quant_e) override;

  std::string identify() const override { return "CcfvEngine"; }

 private:
  struct Statistics
  {
    Statistics(StatisticsRegistry& sr);
    /** Time of conflict-based instantiation checks (as QCF's d_cbqi_time). */
    TimerStat d_cbqiTime;
    /** Time of trigger-based instantiation checks (as the instantiation
     * engine's d_ematching_time). */
    TimerStat d_tbqiTime;
    /** Number of quantified formulas skipped because their CNF would exceed
     * the literal limit. */
    IntStat d_cnfLimitExceeded;
    /** Time building CcfvIndices (nested in the cbqi/tbqi times). */
    TimerStat d_indicesTime;
    /** Number of times CcfvIndices were built. */
    IntStat d_indicesBuilds;
    /** Time inside the CCFV solver (nested in the cbqi/tbqi times). The
     * lazily computed disequality lists are accounted here. */
    TimerStat d_searchTime;

    /** Counters of one technique (CBQI or TBQI). */
    struct Technique
    {
      Technique(StatisticsRegistry& sr, const std::string& prefix);
      /** Calls of check. */
      IntStat d_checks;
      /** Calls with at least one solver call (rounds that did search). */
      IntStat d_roundsWithSolve;
      /** Solver calls: clauses for CBQI, grounded triggers for TBQI. */
      IntStat d_solveCalls;
      /** Solver calls that returned at least one substitution. */
      IntStat d_solveSolved;
      /** Instantiations accepted by Instantiate::addInstantiation. */
      IntStat d_instsAdded;
      /** Instantiations rejected by Instantiate::addInstantiation. */
      IntStat d_instsRejected;
      /** Search nodes explored (decision mode). */
      IntStat d_nodes;
      /** Search nodes per solver call. */
      AverageStat d_nodesPerSolve;
      /** Search nodes per round with at least one solver call. */
      AverageStat d_nodesPerRound;
    };
    Technique d_cbqi;
    Technique d_tbqi;
  };
  Statistics d_stats;

  /** Search nodes and solver calls of the round in progress. */
  uint64_t d_roundNodes = 0;
  uint64_t d_roundSolves = 0;
  /** Calls the solver, accounting search time, nodes and solver calls. */
  bool solveTimed(Statistics::Technique& t,
                  const std::vector<Node>& freeVars,
                  const std::vector<Node>& lits,
                  eq::EqualityEngine* ee,
                  const ccfv::CcfvIndices& indices,
                  std::vector<std::vector<Node>>& substitutions);
  /** Starts/ends the accounting of a check of technique t. */
  void beginRound(Statistics::Technique& t);
  void endRound(Statistics::Technique& t);

  std::unordered_map<Node, std::vector<std::vector<Node>>> d_quantCnf;
  /** The quantified formulas processed in the current trigger check. */
  std::vector<Node> d_quants;
  /** The number of trigger checks so far. */
  uint64_t d_checkCount = 0;
  std::unique_ptr<ccfv::CcfvIndices> d_indices;
  std::unordered_map<Node, std::vector<Node>> d_quantVars;

  /** CCFV solver instance for solving E-ground (dis)unification */
  std::unique_ptr<CcfvSolver> d_solver;

  /** Generation of the triggers of each quantified formula. */
  CcfvTriggerGenerator d_triggerGen;

  /** Custom Conflict-based instantiation implementation */
  void checkConflictInst(Theory::Effort level);

  /** Custom Trigger-based instantiation implementation */
  void checkTriggerInst(Theory::Effort level);

  /** Same as InstantiationEngine::shouldProcess. */
  bool shouldProcess(Node q);
  /**
   * Same as InstantiationEngine::doInstantiationRound: tries internal effort
   * levels, stopping at the first one that added a lemma.
   */
  void doInstantiationRound(Theory::Effort effort);
  /**
   * Same as InstStrategyUserPatterns::process.
   * @return true if the strategy is unfinished at effort level e.
   */
  bool processUserPatterns(Node q, CcfvQuantTriggers& qt, int e);
  /**
   * Same as InstStrategyAutoGenTriggers::process.
   * @return true if the strategy is unfinished at effort level e.
   */
  bool processAutoGen(Node q, CcfvQuantTriggers& qt, int e);
  /**
   * Adds the instantiations of a trigger, as Trigger::addInstantiations.
   * @return The number of lemmas added.
   */
  uint64_t processTrigger(Node q, const CcfvTriggerEntry& tr);

  const ccfv::CcfvIndices& getIndices(eq::EqualityEngine* ee);
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
