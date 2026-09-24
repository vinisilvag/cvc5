#ifndef CVC5__THEORY__QUANTIFIERS__CCFV_ENGINE_H
#define CVC5__THEORY__QUANTIFIERS__CCFV_ENGINE_H

#include <memory>

#include "theory/quantifiers/quant_module.h"

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

class CcfvSolver;

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

  /** Called at the beginning of each check round */
  void reset_round(Theory::Effort level) override;

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
