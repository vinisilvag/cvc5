#include "theory/quantifiers/ccfv/ccfv_engine.h"

#include "base/output.h"
#include "theory/quantifiers/ccfv/ccfv_solver.h"
#include "theory/quantifiers/first_order_model.h"
#include "theory/quantifiers/instantiate.h"
#include "theory/quantifiers/term_registry.h"

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

QuantifiersModule::QEffort CcfvEngine::needsModel(CVC5_UNUSED Theory::Effort level)
{
  return QEFFORT_MODEL;
}

void CcfvEngine::reset_round(Theory::Effort level)
{
  // Reset per-round data structures
}

void CcfvEngine::check(Theory::Effort level, QEffort quant_e)
{
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
  FirstOrderModel* fm = d_treg.getModel();
  size_t nquant = fm->getNumAssertedQuantifiers();
  Instantiate* qinst = d_qim.getInstantiate();

  for (size_t i = 0; i < nquant; ++i)
  {
    Node q = fm->getAssertedQuantifier(i, true);
    if (!fm->isQuantifierActive(q)) continue;

    // TODO: Your custom conflict find algorithm here
    // If you find a conflict instantiation `terms` for `q`:
    // std::vector<Node> terms = ...;
    // bool added = qinst->addInstantiation(q, terms,
    // InferenceId::QUANTIFIERS_INST_CBQI_CONFLICT); if (added) return; //
    // Terminate early on conflict
  }
}

void CcfvEngine::checkTriggerInst(Theory::Effort level)
{
  FirstOrderModel* fm = d_treg.getModel();
  size_t nquant = fm->getNumAssertedQuantifiers();
  Instantiate* qinst = d_qim.getInstantiate();

  for (size_t i = 0; i < nquant; ++i)
  {
    Node q = fm->getAssertedQuantifier(i, true);
    if (!fm->isQuantifierActive(q)) continue;

    // TODO: Your custom trigger matching algorithm here
    // std::vector<Node> terms = ...;
    // qinst->addInstantiation(q, terms,
    // InferenceId::QUANTIFIERS_INST_E_MATCHING);
  }
}

void CcfvEngine::checkModelInst(CVC5_UNUSED Theory::Effort level)
{
  // Early return: Model-based quantifier instantiation via CCFV is not yet implemented
  WarningOnce()
      << "CCFV: Model-based quantifier instantiation is not yet implemented."
      << std::endl;
  return;
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
