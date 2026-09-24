#include "theory/quantifiers/ccfv/ccfv_engine.h"

#include "base/output.h"
#include "theory/quantifiers/ccfv/ccfv_solver.h"
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

    // Free variables in L
    std::vector<Node> vars(q[0].begin(), q[0].end());

    // L should be a set of conjunctive equality literals
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

    Trace("ccfv-debug") << "  Variables: " << vars << std::endl;
    Trace("ccfv-debug") << "  L (negated body): " << L << std::endl;

    std::vector<std::vector<Node>> substitutions;
    bool substitutionFound = d_solver->solve(
        options().quantifiers.ccfvMode, vars, L, ee, substitutions);

    if (substitutionFound && !substitutions.empty())
    {
      Trace("ccfv") << "CcfvEngine: Found " << substitutions.size()
                    << " conflicting instance(s) for " << q << std::endl;

      for (const std::vector<Node>& sub : substitutions)
      {
        Trace("ccfv-debug")
            << "  Applying instantiation terms: " << sub << std::endl;
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
  // Trigger-based quantifier instantiation via CCFV is not yet implemented
  WarningOnce()
      << "CCFV: Trigger-based quantifier instantiation is not yet implemented."
      << std::endl;
  return;

  // FirstOrderModel* fm = d_treg.getModel();
  // size_t nquant = fm->getNumAssertedQuantifiers();
  // Instantiate* qinst = d_qim.getInstantiate();
  //
  // for (size_t i = 0; i < nquant; ++i)
  // {
  //   Node q = fm->getAssertedQuantifier(i, true);
  //   if (!fm->isQuantifierActive(q)) continue;
  //
  //   // TODO: Custom trigger matching algorithm here
  //   // std::vector<Node> terms = ...;
  //   // qinst->addInstantiation(q, terms,
  //   // InferenceId::QUANTIFIERS_INST_E_MATCHING);
  // }
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
