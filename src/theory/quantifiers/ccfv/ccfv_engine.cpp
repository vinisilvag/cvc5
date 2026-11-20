/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Implementation of Congruence Closure with Free Variables (CCFV) quantifier
 * instantiation engine.
 */

#include "theory/quantifiers/ccfv/ccfv_engine.h"

#include <memory>

#include "base/output.h"
#include "expr/skolem_manager.h"
#include "theory/quantifiers/ccfv/ccfv_cnf.h"
#include "theory/quantifiers/ccfv/ccfv_indices.h"
#include "theory/quantifiers/ccfv/ccfv_solver.h"
#include "theory/quantifiers/ccfv/ccfv_trigger_generator.h"
#include "theory/quantifiers/ccfv/ccfv_utils.h"
#include "theory/quantifiers/first_order_model.h"
#include "theory/quantifiers/instantiate.h"
#include "theory/quantifiers/quantifiers_attributes.h"
#include "theory/quantifiers/quantifiers_registry.h"
#include "theory/quantifiers/quantifiers_state.h"
#include "theory/quantifiers/term_registry.h"
#include "theory/uf/equality_engine.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

CcfvEngine::Statistics::Statistics(StatisticsRegistry& sr)
    : d_cbqiTime(sr.registerTimer("ccfv::cbqiTime")),
      d_tbqiTime(sr.registerTimer("ccfv::tbqiTime")),
      d_cnfLimitExceeded(sr.registerInt("ccfv::cnfLimitExceeded"))
{
}

CcfvEngine::CcfvEngine(Env& env,
                       QuantifiersState& qs,
                       QuantifiersInferenceManager& qim,
                       QuantifiersRegistry& qr,
                       TermRegistry& tr)
    : QuantifiersModule(env, qs, qim, qr, tr),
      d_stats(statisticsRegistry()),
      d_solver(std::make_unique<CcfvSolver>(env)),
      d_triggerGen(env, qs, qr)
{
}

CcfvEngine::~CcfvEngine() = default;

const ccfv::CcfvIndices& CcfvEngine::getIndices(eq::EqualityEngine* ee)
{
  if (!d_indices) d_indices = std::make_unique<ccfv::CcfvIndices>(ee);
  return *d_indices;
}

bool CcfvEngine::needsCheck(Theory::Effort level)
{
  // Run when quantifier-free theories have reached a consistent assignment
  return d_qstate.getInstWhenNeedsCheck(level);
}

void CcfvEngine::check(Theory::Effort level, QEffort quant_e)
{
  d_indices.reset();

  Trace("ccfv") << "[engine] check at level " << level << ", effort " << quant_e
                << std::endl;
  if (quant_e == QEFFORT_CONFLICT)
  {
    // Same as QuantConflictFind, which only runs at full effort (it does not
    // follow --inst-when, and never runs at last call)
    if (level == Theory::EFFORT_FULL && !options().quantifiers.ccfvNoCbqi)
    {
      // the whole check is timed, as QuantConflictFind does
      TimerStat::CodeTimer timer(d_stats.d_cbqiTime);
      checkConflictInst(level);
    }
  }
  else if (quant_e == QEFFORT_STANDARD && !options().quantifiers.ccfvNoTbqi)
  {
    // the whole check is timed, as the instantiation engine does
    TimerStat::CodeTimer timer(d_stats.d_tbqiTime);
    checkTriggerInst(level);
  }
}

void CcfvEngine::checkConflictInst(CVC5_UNUSED Theory::Effort level)
{
  Trace("ccfv") << "[cbqi] starting conflict-based instantiation check"
                << std::endl;

  if (d_qstate.isConflictingInst())
  {
    Trace("ccfv") << "[cbqi] model already conflicted, aborting" << std::endl;
    return;
  }

  FirstOrderModel* fm = d_treg.getModel();
  size_t nquant = fm->getNumAssertedQuantifiers();
  Instantiate* qinst = d_qim.getInstantiate();
  eq::EqualityEngine* ee = d_qstate.getEqualityEngine();

  Trace("ccfv") << "[cbqi] " << nquant << " asserted quantifiers" << std::endl;

  for (size_t i = 0; i < nquant; ++i)
  {
    TNode q = fm->getAssertedQuantifier(i, true);
    // Same filter as QuantConflictFind: only quantified formulas owned by
    // this module (or by no module) that are still active
    if (!d_qreg.hasOwnership(q, this) || !fm->isQuantifierActive(q))
    {
      continue;
    }

    Trace("ccfv-debug") << "[cbqi] checking quantifier: " << q << std::endl;

    if (d_quantCnf.find(q) == d_quantCnf.end())
    {
      std::vector<Node> vars;
      std::vector<std::vector<Node>> clauses;
      ccfv::CnfStatus cnfStatus = ccfv::computeQuantCnf(
          q, vars, clauses, options().quantifiers.ccfvCnfLiteralLimit);
      if (cnfStatus != ccfv::CnfStatus::SUCCESS)
      {
        if (cnfStatus == ccfv::CnfStatus::TOO_LARGE)
        {
          // the quantifier is only attempted once, as its CNF is cached
          ++d_stats.d_cnfLimitExceeded;
        }
        Trace("ccfv") << "[cbqi] skipping " << q << ": no CNF available"
                      << std::endl;
        d_quantCnf[q] = {};
        continue;
      }
      d_quantCnf[q] = std::move(clauses);
      d_quantVars[q] = std::move(vars);
    }
    if (d_quantCnf[q].empty())
    {
      continue;
    }

    const std::vector<std::vector<Node>>& clauses = d_quantCnf[q];
    const std::vector<Node>& freeVars = d_quantVars[q];

    Trace("ccfv-debug") << "[cbqi] variables: " << freeVars << std::endl;

    // Set when a conflicting instance was added or when adding failed, in
    // which case q is not checked further this round (as QuantConflictFind)
    bool done = false;
    for (const std::vector<Node>& clause : clauses)
    {
      Trace("ccfv-debug") << "[cbqi] clause: " << clause << std::endl;
      std::vector<Node> L;
      for (TNode lit : clause)
      {
        L.push_back(lit.negate());
      }

      std::vector<std::vector<Node>> substitutions;
      bool solved = d_solver->solve(options().quantifiers.ccfvMode,
                                    freeVars,
                                    L,
                                    ee,
                                    getIndices(ee),
                                    substitutions);
      if (solved)
      {
        Trace("ccfv-debug")
            << "[cbqi] found " << substitutions.size()
            << " conflicting instance(s) for " << q << std::endl;
        for (std::vector<Node>& sub : substitutions)
        {
          Trace("ccfv-debug") << "[cbqi] applying substitution: " << freeVars
                              << " -> " << sub << std::endl;

          // Workaround to avoid different free variables count after prenexing
          std::vector<Node> origSub(sub.begin(),
                                    sub.begin() + q[0].getNumChildren());
          if (!qinst->addInstantiation(
                  q, origSub, InferenceId::QUANTIFIERS_INST_CBQI_CONFLICT))
          {
            // The instance was already added (duplicate): stop with this
            // quantified formula to avoid exponential behavior
            Trace("ccfv") << "[cbqi] instance for " << q
                          << " already exists, skipping the quantifier"
                          << std::endl;
            done = true;
            break;
          }
          // Conflict found and added. Marking q as relevant ensures it is
          // checked first on the next round
          Trace("ccfv") << "[cbqi] conflicting instance added for " << q << ": "
                        << origSub << std::endl;
          fm->markRelevant(q);
          d_qstate.notifyConflictingInst();
          done = true;
          break;
        }
      }
      if (done)
      {
        break;
      }
    }
    if (d_qstate.isConflictingInst() || d_qstate.isInConflict())
    {
      return;
    }
    Trace("ccfv-debug") << "[cbqi] no conflicting instance found for " << q
                        << std::endl;
  }
}

bool CcfvEngine::shouldProcess(Node q)
{
  if (!d_qreg.hasOwnership(q, this))
  {
    return false;
  }
  // also ignore internal quantifiers
  return !d_qreg.getQuantAttributes().isQuantBounded(q);
}

bool CcfvEngine::processUserPatterns(Node q, CcfvQuantTriggers& qt, int e)
{
  if (e == 0)
  {
    return true;
  }
  options::UserPatMode upm = d_triggerGen.getUserPatMode();
  int peffort = upm == options::UserPatMode::RESORT ? 2 : 1;
  if (e < peffort)
  {
    return true;
  }
  if (e != peffort)
  {
    return false;
  }
  for (const CcfvTriggerEntry& tr : qt.d_user)
  {
    processTrigger(q, tr);
    if (d_qstate.isInConflict())
    {
      break;
    }
  }
  return false;
}

bool CcfvEngine::processAutoGen(Node q, CcfvQuantTriggers& qt, int e)
{
  options::UserPatMode upm = d_triggerGen.getUserPatMode();
  // we do not auto-generate triggers if the mode is trust or strict
  if (qt.d_hasUserPatterns
      && (upm == options::UserPatMode::TRUST
          || upm == options::UserPatMode::STRICT))
  {
    return false;
  }
  int peffort = (qt.d_hasUserPatterns && upm != options::UserPatMode::IGNORE
                 && upm != options::UserPatMode::RESORT)
                    ? 2
                    : 1;
  if (e < peffort)
  {
    return true;
  }
  // whether to select new triggers during the search
  bool regenerate = options().quantifiers.incrementTriggers;
  int regenerateFrequency = regenerate ? 3 : 1;
  bool gen = false;
  if (e == peffort)
  {
    if (qt.d_counter < 0)
    {
      qt.d_counter = 0;
      gen = true;
    }
    else
    {
      qt.d_counter++;
      gen = regenerate && qt.d_counter % regenerateFrequency == 0;
    }
  }
  else
  {
    gen = true;
  }
  if (gen)
  {
    d_triggerGen.generateTriggers(q, qt);
  }

  bool hasInst = false;
  for (size_t r = 0; r < 2; r++)
  {
    for (CcfvTriggerEntry& tr : qt.d_auto[r])
    {
      if (tr.d_round == d_checkCount)
      {
        // trigger is already processed this round
        continue;
      }
      tr.d_round = d_checkCount;
      hasInst = processTrigger(q, tr) > 0 || hasInst;
      if (d_qstate.isInConflict())
      {
        break;
      }
    }
    if (d_qstate.isInConflict()
        || (hasInst && options().quantifiers.multiTriggerPriority))
    {
      break;
    }
  }
  return false;
}

void CcfvEngine::doInstantiationRound(Theory::Effort effort)
{
  size_t lastWaiting = d_qim.numPendingLemmas();
  // iterate over an internal effort level e
  int e = 0;
  int eLimit = effort == Theory::EFFORT_LAST_CALL ? 10 : 2;
  bool useUser =
      options().quantifiers.userPatternsQuant != options::UserPatMode::IGNORE;
  bool finished = false;
  // while unfinished, try effort level=0,1,2....
  while (!finished && e <= eLimit)
  {
    Trace("ccfv-debug") << "[tbqi] internal effort level " << e << std::endl;
    finished = true;
    for (const Node& q : d_quants)
    {
      CcfvQuantTriggers& qt = d_triggerGen.getQuantTriggers(q);
      if (useUser && processUserPatterns(q, qt, e))
      {
        finished = false;
      }
      if (d_qstate.isInConflict())
      {
        return;
      }
      if (processAutoGen(q, qt, e))
      {
        finished = false;
      }
      if (d_qstate.isInConflict())
      {
        return;
      }
    }
    // do not consider another level if already added lemma at this level
    if (d_qim.numPendingLemmas() > lastWaiting)
    {
      finished = true;
    }
    e++;
  }
}

void CcfvEngine::checkTriggerInst(Theory::Effort level)
{
  Trace("ccfv") << "[tbqi] starting trigger-based instantiation check"
                << std::endl;

  if (d_qstate.isConflictingInst())
  {
    Trace("ccfv") << "[tbqi] model already conflicted, aborting" << std::endl;
    return;
  }

  ++d_checkCount;

  // collect all active quantified formulas belonging to this module
  FirstOrderModel* fm = d_treg.getModel();
  size_t nquant = fm->getNumAssertedQuantifiers();
  d_quants.clear();
  for (size_t i = 0; i < nquant; ++i)
  {
    Node q = fm->getAssertedQuantifier(i, true);
    if (shouldProcess(q) && fm->isQuantifierActive(q))
    {
      d_quants.push_back(q);
    }
  }
  Trace("ccfv") << "[tbqi] " << d_quants.size() << "/" << nquant
                << " quantifiers to process" << std::endl;

  if (!d_quants.empty())
  {
    doInstantiationRound(level);
  }
  d_quants.clear();
}

uint64_t CcfvEngine::processTrigger(Node q, const CcfvTriggerEntry& tr)
{
  uint64_t addedLemmas = 0;
  eq::EqualityEngine* ee = d_qstate.getEqualityEngine();
  Instantiate* qinst = d_qim.getInstantiate();

  // for each ground term t that does not exist in the equality engine, we add
  // a purification lemma of the form (k = t)
  for (const Node& gt : tr.d_groundTerms)
  {
    if (!ee->hasTerm(gt) && !gt.getType().isBoolean())
    {
      Node k = SkolemManager::mkPurifySkolem(gt);
      Trace("ccfv") << "[tbqi] ground term purification lemma: " << k.eqNode(gt)
                    << std::endl;
      d_qim.addPendingLemma(k.eqNode(gt), InferenceId::QUANTIFIERS_GT_PURIFY);
      addedLemmas++;
    }
  }

  const std::vector<Node>& trigger = tr.d_pats;
  std::vector<Node> freeVars(q[0].begin(), q[0].end());
  const ccfv::CcfvIndices& indices = getIndices(ee);
  Trace("ccfv-debug") << "[tbqi] processing trigger " << trigger << " of " << q
                      << std::endl;

  size_t triggerSize = trigger.size();
  for (TNode pat : trigger)
  {
    if (indices.getReps(pat.getOperator()).empty())
    {
      return addedLemmas;
    }
  }

  // Generating L candidates
  // Knuth Odometer algorithm (The Art of Computer Programming, volume 4A)
  std::vector<int> idx(triggerSize, 0);
  while (true)
  {
    std::vector<Node> L = {};

    for (size_t k = 0; k < triggerSize; ++k)
    {
      TNode f = trigger[k].getOperator();
      TNode t = indices.getReps(f)[idx[k]];
      L.push_back(trigger[k].eqNode(t));
    }

    Trace("ccfv-debug") << "[tbqi] grounded trigger: " << L << std::endl;
    std::vector<std::vector<Node>> substitutions;
    bool solved = d_solver->solve(options().quantifiers.ccfvMode,
                                  freeVars,
                                  L,
                                  ee,
                                  indices,
                                  substitutions);
    if (solved)
    {
      Trace("ccfv-debug") << "[tbqi] found " << substitutions.size()
                          << " substitution(s) for " << q << std::endl;
      for (std::vector<Node>& sub : substitutions)
      {
        Trace("ccfv-debug") << "[tbqi] applying substitution: " << freeVars
                            << " -> " << sub << std::endl;
        if (qinst->addInstantiation(
                q, sub, InferenceId::QUANTIFIERS_INST_E_MATCHING))
        {
          Trace("ccfv") << "[tbqi] instantiation added for " << q << ": " << sub
                        << std::endl;
          addedLemmas++;
          if (d_qstate.isInConflict())
          {
            return addedLemmas;
          }
        }
      }
    }

    int pos = triggerSize - 1;
    while (pos >= 0)
    {
      Node fPos = trigger[pos].getOperator();
      idx[pos]++;
      if (idx[pos] < (int)indices.getReps(fPos).size())
      {
        break;
      }
      idx[pos] = 0;
      pos--;
    }

    if (pos < 0)
    {
      break;
    }
  }
  return addedLemmas;
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
