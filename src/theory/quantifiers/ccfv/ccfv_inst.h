#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_INST_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_INST_H

#include "theory/quantifiers/quant_module.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

class CcfvInst : public QuantifiersModule
{
 public:
  CcfvInst(Env& env,
           QuantifiersState& qs,
           QuantifiersInferenceManager& qim,
           QuantifiersRegistry& qr,
           TermRegistry& tr);
  ~CcfvInst() override = default;

  /** Called when a new quantifier is registered */
  void registerQuantifier(Node q) override;

  /** Decides when cvc5 should call this module */
  bool needsCheck(Theory::Effort level) override;

  /** Called at the beginning of each check round */
  void reset_round(Theory::Effort level) override;

  /** Core check function called at each quantifier effort level */
  void check(Theory::Effort level, QEffort quant_e) override;

  std::string identify() const override { return "MyCustomInst"; }

 private:
  /** Your custom Conflict-based instantiation implementation */
  void checkConflictInst(Theory::Effort level);

  /** Your custom Trigger-based instantiation implementation */
  void checkTriggerInst(Theory::Effort level);
};

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal

#endif
