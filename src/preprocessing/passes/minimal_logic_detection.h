/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Minimal logic detection preprocessing pass.
 *
 * Traverses assertions to determine the minimal logic required for
 * problems with under-specified logic (e.g., ALL or QF_ALL) and specializes
 * the solver's logic accordingly.
 */

#ifndef CVC5__PREPROCESSING__PASSES__MINIMAL_LOGIC_DETECTION_H_
#define CVC5__PREPROCESSING__PASSES__MINIMAL_LOGIC_DETECTION_H_

#include "expr/node.h"
#include "preprocessing/preprocessing_pass.h"
#include "theory/logic_info.h"

namespace cvc5::internal {
namespace preprocessing {
namespace passes {

/**
 * Preprocessing pass that analyzes the problem's assertions to detect the
 * minimal SMT-LIB logic needed to solve it. When the logic is generic or
 * under-specified (such as ALL or QF_ALL), this pass identifies active
 * theories, quantifiers, and arithmetic restrictions, and narrows the
 * solver's logic to optimize downstream preprocessing.
 */
class MinimalLogicDetection : public PreprocessingPass
{
 public:
  MinimalLogicDetection(PreprocessingPassContext* preprocContext);
  ~MinimalLogicDetection();

  /**
   * Get the minimal logic detected during the last run of this pass.
   *
   * @return A const reference to the LogicInfo containing the detected logic.
   */
  const LogicInfo& getDetectedLogic() const { return d_detectedLogic; }

 private:
  /** The minimal logic detected from inspecting the assertions. */
  LogicInfo d_detectedLogic;

 protected:
  PreprocessingPassResult applyInternal(
      AssertionPipeline* assertionsToPreprocess) override;
};

}  // namespace passes
}  // namespace preprocessing
}  // namespace cvc5::internal

#endif /* CVC5__PREPROCESSING__PASSES__MINIMAL_LOGIC_DETECTION_H_ */
