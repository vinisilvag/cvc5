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
#include "theory/quantifiers/ccfv/ccfv_solver.h"
#include "theory/quantifiers/ccfv/ccfv_utils.h"
#include "theory/quantifiers/ematching/pattern_term_selector.h"
#include "theory/quantifiers/first_order_model.h"
#include "theory/quantifiers/instantiate.h"
#include "theory/quantifiers/term_registry.h"
#include "theory/uf/equality_engine.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

CcfvEngine::CcfvEngine(Env& env,
                       QuantifiersState& qs,
                       QuantifiersInferenceManager& qim,
                       QuantifiersRegistry& qr,
                       TermRegistry& tr)
    : QuantifiersModule(env, qs, qim, qr, tr),
      d_solver(std::make_unique<CcfvSolver>(env))
{
}

CcfvEngine::~CcfvEngine() = default;

bool CcfvEngine::needsCheck(Theory::Effort level)
{
  // Run when quantifier-free theories have reached a consistent assignment
  return d_qstate.getInstWhenNeedsCheck(level);
}

QuantifiersModule::QEffort CcfvEngine::needsModel(
    CVC5_UNUSED Theory::Effort level)
{
  return QEFFORT_MODEL;
}

void CcfvEngine::check(Theory::Effort level, QEffort quant_e)
{
  Trace("ccfv") << "CcfvEngine::check at level " << level << ", effort "
                << quant_e << std::endl;
  if (quant_e == QEFFORT_CONFLICT)
  {
    checkConflictInst(level);
  }
  else if (quant_e == QEFFORT_STANDARD)
  {
    checkTriggerInst(level);
  }
  else if (quant_e == QEFFORT_MODEL)
  {
    return;
    checkModelInst(level);
  }
}

void CcfvEngine::checkConflictInst(CVC5_UNUSED Theory::Effort level)
{
  Trace("ccfv") << "CcfvEngine: Starting conflict-based instantiation check"
                << std::endl;

  FirstOrderModel* fm = d_treg.getModel();
  size_t nquant = fm->getNumAssertedQuantifiers();
  Instantiate* qinst = d_qim.getInstantiate();
  eq::EqualityEngine* ee = d_qstate.getEqualityEngine();

  Trace("ccfv") << "CcfvEngine: " << nquant << " asserted quantifiers in model"
                << std::endl;

  for (size_t i = 0; i < nquant; ++i)
  {
    Node q = fm->getAssertedQuantifier(i, true);
    if (!fm->isQuantifierActive(q))
    {
      continue;
    }

    Trace("ccfv-debug") << "CcfvEngine: Checking quantifier: " << q
                        << std::endl;

    if (d_quantCnf.find(q) == d_quantCnf.end())
    {
      std::vector<Node> vars;
      std::vector<std::vector<Node>> clauses;
      if (!ccfv::computeQuantCnf(q, vars, clauses))
      {
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

    Trace("ccfv-debug") << "  Variables: " << freeVars << std::endl;

    for (const std::vector<Node>& clause : clauses)
    {
      Trace("ccfv-debug") << "  Instantiating for clause: " << clause
                          << std::endl;
      std::vector<Node> L;
      for (const Node& lit : clause)
      {
        L.push_back(lit.negate());
      }

      std::vector<std::vector<Node>> substitutions;
      if (d_solver->solve(
              options().quantifiers.ccfvMode, freeVars, L, ee, substitutions))
      {
        Trace("ccfv") << "CcfvEngine: Found " << substitutions.size()
                      << " conflicting instance(s) for " << q << std::endl;
        for (std::vector<Node>& sub : substitutions)
        {
          Trace("ccfv-debug") << "  Applying instantiation terms: " << freeVars
                              << " mapsto " << sub << std::endl;
          if (qinst->addInstantiation(
                  q, sub, InferenceId::QUANTIFIERS_INST_CBQI_CONFLICT))
          {
            d_qstate.notifyConflictingInst();
            fm->markRelevant(q);
            // Conflict found and added
            // Breaking from the loop after the addition
            return;
          }
        }
      }
    }
    Trace("ccfv-debug") << "  No conflicting instance found for " << q
                        << std::endl;
  }
}

void CcfvEngine::checkTriggerInst(CVC5_UNUSED Theory::Effort level)
{
  Trace("ccfv") << "CcfvEngine: Starting trigger-based instantiation check"
                << std::endl;

  FirstOrderModel* fm = d_treg.getModel();
  size_t nquant = fm->getNumAssertedQuantifiers();
  Instantiate* qinst = d_qim.getInstantiate();
  eq::EqualityEngine* ee = d_qstate.getEqualityEngine();

  Trace("ccfv") << "CcfvEngine: " << nquant << " asserted quantifiers in model"
                << std::endl;

  for (size_t i = 0; i < nquant; ++i)
  {
    Node q = fm->getAssertedQuantifier(i, true);
    if (!fm->isQuantifierActive(q))
    {
      continue;
    }

    Trace("ccfv-debug") << "CcfvEngine: Checking quantifier: " << q
                        << std::endl;

    std::vector<Node> freeVars(q[0].begin(), q[0].end());
    std::vector<std::vector<Node>> triggers = {};
    if (q.getNumChildren() == 3)
    {
      Trace("ccfv-debug") << "CcfvEngine: User-defined triggers: " << q[2]
                          << std::endl;
      for (const Node& userPattern : q[2])
      {
        if (userPattern.getKind() != Kind::INST_PATTERN)
        {
          continue;
        }

        std::vector<Node> patList = {};
        for (const Node& pattern : userPattern)
        {
          if (ccfv::isSupportedTriggerTerm(pattern))
          {
            patList.push_back(pattern);
          }
        }
        if (patList.size() == userPattern.getNumChildren())
        {
          triggers.push_back(patList);
        }
      }
    }

    if (triggers.empty())
    {
      Trace("ccfv-debug") << "CcfvEngine: Automatic trigger selection"
                          << std::endl;
      std::vector<Node> patterns;
      std::map<Node, inst::TriggerTermInfo> tinfo;
      inst::PatternTermSelector pts(
          q, options().quantifiers.triggerSelMode, {}, true, false, false);
      pts.collect(d_qreg.getInstConstantBody(q), patterns, tinfo);
      std::vector<Node> multiTrigger;
      for (size_t k = 0; k < patterns.size(); ++k)
      {
        size_t freeVarsNum = tinfo[patterns[k]].d_fv.size();
        Node pattern =
            d_qreg.substituteInstConstantsToBoundVariables(patterns[k], q);

        if (!ccfv::isSupportedTriggerTerm(pattern))
        {
          continue;
        }

        if (freeVarsNum == freeVars.size())
        {
          triggers.push_back({pattern});
        }
        else
        {
          multiTrigger.push_back(pattern);
        }
      }

      if (triggers.empty() && !multiTrigger.empty())
      {
        triggers.push_back(multiTrigger);
      }
    }

    if (triggers.empty())
    {
      Trace("ccfv-debug") << "  No triggers found, skipping." << std::endl;
      continue;
    }

    Trace("ccfv-debug") << "  Set of triggers:" << std::endl;
    for (const std::vector<Node>& patterns : triggers)
    {
      Trace("ccfv-debug") << "  " << patterns << std::endl;
    }

    // Helper to build L based on trigger selection
    std::unordered_map<Node, std::vector<Node>> apps = {};
    for (eq::EqClassesIterator eqcs_i(ee); !eqcs_i.isFinished(); ++eqcs_i)
    {
      Node rep = *eqcs_i;
      eq::EqClassIterator eqc_i(rep, ee);
      while (!eqc_i.isFinished())
      {
        Node term = *eqc_i;
        if (term.getKind() == Kind::APPLY_UF)
        {
          Node f = term.getOperator();
          apps[f].push_back(term);
        }
        ++eqc_i;
      }
    }

    Trace("ccfv-debug") << "  Variables: " << freeVars << std::endl;
    int addedLemmas = 0;

    // Generating L candidates
    // M algorithm from Knuth
    for (const std::vector<Node>& trigger : triggers)
    {
      size_t triggerSize = trigger.size();
      bool canMatch = true;

      for (const Node& pat : trigger)
      {
        if (apps[pat.getOperator()].empty())
        {
          canMatch = false;
          break;
        }
      }

      if (!canMatch) continue;

      std::vector<int> idx(triggerSize, 0);
      while (true)
      {
        std::vector<Node> L = {};

        for (size_t k = 0; k < triggerSize; ++k)
        {
          Node f = trigger[k].getOperator();
          Node t = apps[f][idx[k]];
          L.push_back(trigger[k].eqNode(t));
        }

        Trace("ccfv-debug")
            << "  Selecting grounded trigger: " << L << std::endl;
        std::vector<std::vector<Node>> substitutions;
        if (d_solver->solve(
                options().quantifiers.ccfvMode, freeVars, L, ee, substitutions))
        {
          Trace("ccfv") << "CcfvEngine: Found " << substitutions.size()
                        << " substitutions for " << q << std::endl;
          for (std::vector<Node>& sub : substitutions)
          {
            Trace("ccfv-debug")
                << "  Applying instantiation terms: " << freeVars << " mapsto "
                << sub << std::endl;
            if (qinst->addInstantiation(
                    q, sub, InferenceId::QUANTIFIERS_INST_E_MATCHING))
            {
              addedLemmas++;
              if (d_qstate.isInConflict()) return;
            }
          }
        }
        else
        {
          Trace("ccfv-debug")
              << "  No relevant substitution found with this triggers"
              << std::endl;
        }

        int pos = triggerSize - 1;
        while (pos >= 0)
        {
          Node fPos = trigger[pos].getOperator();
          idx[pos]++;
          if (idx[pos] < (int)apps[fPos].size())
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
    }

    if (addedLemmas > 0)
    {
      Trace("ccfv-debug") << addedLemmas
                          << " added lemmas with trigger-based instantiation"
                          << std::endl;
    }
    else
    {
      Trace("ccfv-debug") << "  No relevant substitution found with "
                             "trigger-based instantiation for quantifier "
                          << q << std::endl;
    }
  }
}

void CcfvEngine::checkModelInst(CVC5_UNUSED Theory::Effort level)
{
  // Model-based quantifier instantiation via CCFV is not yet implemented
  WarningOnce()
      << "CCFV: Model-based quantifier instantiation is not yet implemented."
      << std::endl;
  return;
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
