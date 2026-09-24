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

void CcfvEngine::registerQuantifier(Node q)
{
  // Preprocess q, extract terms, build patterns, etc.
}

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

void CcfvEngine::reset_round(Theory::Effort level)
{
  // Reset per-round data structures
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
    std::vector<Node> patTerms;
    if (q.getNumChildren() == 3)
    {
      // Trace("ccfv-debug") << "CcfvEngine: User-defined triggers: " << q[2]
      //                     << std::endl;
      // TODO: implement later
      // User-defined triggers is not yet implemented
      WarningOnce() << "CCFV: User-defined triggers is not yet implemented."
                    << std::endl;
      return;
    }
    else
    {
      Trace("ccfv-debug") << "CcfvEngine: Automatic trigger selection"
                          << std::endl;

      std::map<Node, inst::TriggerTermInfo> tinfo;
      inst::PatternTermSelector pts(
          d_env.getOptions(), q, options().quantifiers.triggerSelMode);
      pts.collect(d_qreg.getInstConstantBody(q), patTerms, tinfo);
      for (size_t k = 0; k < patTerms.size(); ++k)
      {
        patTerms[k] =
            d_qreg.substituteInstConstantsToBoundVariables(patTerms[k], q);
      }
    }

    if (patTerms.empty())
    {
      Trace("ccfv-debug") << "  No triggers found, skipping." << std::endl;
      continue;
    }

    Trace("ccfv-debug") << "  Selected triggers: " << patTerms << std::endl;
    Trace("ccfv-debug") << "  Variables: " << freeVars << std::endl;

    std::vector<std::vector<Node>> candidates(patTerms.size());
    std::vector<size_t> indices(patTerms.size(), 0);

    TermDb* db = d_treg.getTermDatabase();
    for (size_t j = 0; j < patTerms.size(); ++j)
    {
      const Node& pat = patTerms[j];
      if (!pat.hasOperator())
      {
        continue;
      }
      Node op = pat.getOperator();
      size_t nterms = db->getNumGroundTerms(op);

      // Otimization to consider only class representatives
      // It reduces the ammount of candidates and, because of that, the CCFV
      // number of calls
      std::unordered_set<Node> seenReps;

      for (size_t k = 0; k < nterms; ++k)
      {
        Node t = db->getGroundTerm(op, k);
        Node rep = ee->getRepresentative(t);
        if (seenReps.insert(rep).second)
        {
          candidates[j].push_back(t);
        }
      }
    }

    bool hasEmptyCandidate = false;
    for (size_t j = 0; j < patTerms.size(); ++j)
    {
      if (candidates[j].empty())
      {
        hasEmptyCandidate = true;
        break;
      }
    }
    if (hasEmptyCandidate)
    {
      Trace("ccfv-debug")
          << "  Some pattern has no ground terms in E, skipping." << std::endl;
      continue;
    }

    uint64_t totalComb = 1;
    for (size_t j = 0; j < patTerms.size(); ++j)
    {
      totalComb *= candidates[j].size();
    }
    Trace("ccfv") << "CcfvEngine: Trigger Cartesian product has " << totalComb
                  << " combination(s)" << std::endl;

    NodeManager* nm = nodeManager();
    while (true)
    {
      if (TraceIsOn("ccfv-debug"))
      {
        Trace("ccfv-debug") << "  Testing combination indices: [";
        for (size_t idx : indices)
        {
          Trace("ccfv-debug") << " " << idx;
        }
        Trace("ccfv-debug") << " ]" << std::endl;
      }

      std::vector<Node> L;
      for (size_t k = 0; k < patTerms.size(); ++k)
      {
        L.push_back(
            nm->mkNode(Kind::EQUAL, patTerms[k], candidates[k][indices[k]]));
      }

      Trace("ccfv-debug") << "  L: " << L << std::endl;

      std::vector<std::vector<Node>> substitutions;
      bool substitutionFound = d_solver->solve(
          options().quantifiers.ccfvMode, freeVars, L, ee, substitutions);

      if (substitutionFound && !substitutions.empty())
      {
        Trace("ccfv") << "CcfvEngine: Found " << substitutions.size()
                      << " relevant instance(s) for " << q << std::endl;

        for (const std::vector<Node>& sub : substitutions)
        {
          Trace("ccfv-debug")
              << "  Applying instantiation terms: " << sub << std::endl;

          // TODO: apply instantiation
        }
      }
      else
      {
        Trace("ccfv-debug") << "  No relevant instance found for this L. "
                               "Trying next combination."
                            << std::endl;
      }

      int p = static_cast<int>(patTerms.size()) - 1;
      while (p >= 0)
      {
        indices[p]++;
        if (indices[p] < candidates[p].size())
        {
          break;
        }
        else
        {
          indices[p] = 0;
          p--;
        }
      }
      if (p < 0)
      {
        Trace("ccfv-debug") << "  Cartesian product exhausted." << std::endl;
        break;
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
