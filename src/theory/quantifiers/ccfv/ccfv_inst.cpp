#include "theory/quantifiers/ccfv/ccfv_inst.h"

#include "theory/quantifiers/first_order_model.h"
#include "theory/quantifiers/instantiate.h"
#include "theory/quantifiers/term_registry.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

CcfvInst::CcfvInst(Env& env,
                   QuantifiersState& qs,
                   QuantifiersInferenceManager& qim,
                   QuantifiersRegistry& qr,
                   TermRegistry& tr)
    : QuantifiersModule(env, qs, qim, qr, tr)
{
}

void CcfvInst::registerQuantifier(Node q)
{
  // Preprocess q, extract terms, build patterns, etc.
}

bool CcfvInst::needsCheck(Theory::Effort level)
{
  // Run when quantifier-free theories have reached a consistent assignment
  return d_qstate.getInstWhenNeedsCheck(level);
}

void CcfvInst::reset_round(Theory::Effort level)
{
  // Reset per-round data structures
}

void CcfvInst::check(Theory::Effort level, QEffort quant_e)
{
  if (quant_e == QEFFORT_CONFLICT)
  {
    // Step 1: Run your custom conflict-based instantiation!
    checkConflictInst(level);
    // Note: If you find an instantiation and add it via
    // Instantiate::addInstantiation(), d_qim.hasSentLemma() becomes true, and
    // QuantifiersEngine will immediately abort the round and send the lemma to
    // the SAT solver (skipping QEFFORT_STANDARD).
  }
  else if (quant_e == QEFFORT_STANDARD)
  {
    // Step 2: Run your custom trigger-based instantiation!
    // (Only reached if checkConflictInst found no conflicting lemmas)
    checkTriggerInst(level);
  }
}

void CcfvInst::checkConflictInst(Theory::Effort level)
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

void CcfvInst::checkTriggerInst(Theory::Effort level)
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

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
