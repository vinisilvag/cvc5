/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Add description later
 *
 */

#include "cvc5_private.h"

#ifndef CVC5__PREPROCESSING__PASSES__MINIMAL_LOGIC_DETECTION_H
#define CVC5__PREPROCESSING__PASSES__MINIMAL_LOGIC_DETECTION_H

#include "preprocessing/preprocessing_pass.h"

namespace cvc5::internal {
namespace preprocessing {
namespace passes {

class MinimalLogicDetection : public PreprocessingPass
{
 public:
  MinimalLogicDetection(PreprocessingPassContext* preprocContext);

 protected:
  PreprocessingPassResult applyInternal(
      AssertionPipeline* assertionsToPreprocess) override;
};

}  // namespace passes
}  // namespace preprocessing
}  // namespace cvc5::internal

#endif /* CVC5__PREPROCESSING__PASSES__MINIMAL_LOGIC_DETECTION_H */
