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

#include "base/output.h"
#include "theory/quantifiers/ccfv/ccfv_solver.h"
#include "theory/quantifiers/ematching/pattern_term_selector.h"
#include "theory/quantifiers/first_order_model.h"
#include "theory/quantifiers/instantiate.h"
#include "theory/quantifiers/term_registry.h"
#include "theory/uf/equality_engine.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

bool isClause(Node n)
{
  if (QuantifiersRewriter::isLiteral(n) || n.isConst())
  {
    return true;
  }
  if (n.getKind() == Kind::OR)
  {
    for (const Node& child : n)
    {
      if (!QuantifiersRewriter::isLiteral(child) && !child.isConst())
      {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool isCNF(Node n)
{
  if (isClause(n))
  {
    return true;
  }
  if (n.getKind() == Kind::AND)
  {
    for (const Node& child : n)
    {
      if (!isClause(child))
      {
        return false;
      }
    }
    return true;
  }
  return false;
}

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
    checkModelInst(level);
  }
}

void CcfvEngine::checkConflictInst(Theory::Effort level)
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

    // Temporary: check if every q is indeed in CNF
    Assert(isCNF(q));

    Trace("ccfv-debug") << "CcfvEngine: Checking quantifier: " << q
                        << std::endl;

    std::vector<Node> freeVars(q[0].begin(), q[0].end());
    std::vector<Node> L;
    if (q[1].getKind() == Kind::OR)
    {
      for (const Node& lit : q[1])
      {
        L.push_back(lit.negate());
      }
    }
    else
    {
      L.push_back(q[1].negate());
    }

    Trace("ccfv-debug") << "  Variables: " << freeVars << std::endl;
    Trace("ccfv-debug") << "  L (negated body): " << L << std::endl;

    std::vector<std::vector<Node>> substitutions;
    bool substitutionFound = d_solver->solve(
        options().quantifiers.ccfvMode, freeVars, L, ee, substitutions);

    if (substitutionFound && !substitutions.empty())
    {
      Trace("ccfv") << "CcfvEngine: Found " << substitutions.size()
                    << " conflicting instance(s) for " << q << std::endl;

      for (const std::vector<Node>& sub : substitutions)
      {
        Trace("ccfv-debug")
            << "  Applying instantiation terms: " << sub << std::endl;

        // TODO: apply instantiation
      }
    }
    else
    {
      Trace("ccfv-debug") << "  No conflicting instance found for quantifier."
                          << std::endl;
    }
  }
}

void CcfvEngine::checkTriggerInst(Theory::Effort level)
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
        std::vector<Node> patList = {};
        for (const Node& pattern : userPattern)
        {
          patList.push_back(pattern);
        }
        triggers.push_back(patList);
      }
    }
    else
    {
      Trace("ccfv-debug") << "CcfvEngine: Automatic trigger selection"
                          << std::endl;
      std::vector<Node> patterns;
      std::map<Node, inst::TriggerTermInfo> tinfo;
      inst::PatternTermSelector pts(
          d_env.getOptions(), q, options().quantifiers.triggerSelMode);
      pts.collect(d_qreg.getInstConstantBody(q), patterns, tinfo);
      for (size_t k = 0; k < triggers.size(); ++k)
      {
        patterns[k] =
            d_qreg.substituteInstConstantsToBoundVariables(patterns[k], q);
      }
      triggers.push_back(patterns);
    }

    if (triggers.empty())
    {
      Trace("ccfv-debug") << "  No triggers found, skipping." << std::endl;
      continue;
    }

    Trace("ccfv-debug") << "  Variables: " << freeVars << std::endl;
    for (const std::vector<Node>& patterns : triggers)
    {
      Trace("ccfv-debug") << "  Selected triggers: " << patterns << std::endl;

      std::vector<std::vector<Node>> substitutions;
      bool substitutionFound = d_solver->solve(options().quantifiers.ccfvMode,
                                               freeVars,
                                               patterns,
                                               ee,
                                               substitutions);

      if (substitutionFound && !substitutions.empty())
      {
        Trace("ccfv") << "CcfvEngine: Found " << substitutions.size()
                      << " substitutions for " << q << std::endl;

        for (const std::vector<Node>& sub : substitutions)
        {
          Trace("ccfv-debug")
              << "  Applying instantiation terms: " << sub << std::endl;

          // TODO: apply instantiation
        }
      }
      else
      {
        Trace("ccfv-debug")
            << "  No relevant substitution found with this triggers."
            << std::endl;
      }
    }

    Trace("ccfv-debug") << "  No relevant instance (triggered-based "
                           "instance) found for quantifier."
                        << std::endl;
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
