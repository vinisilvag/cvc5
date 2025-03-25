/******************************************************************************
 * Top contributors (to current version):
 *  Vinicius Gomes
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2025 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Performs the preprocessing of the input problem, identifying the minimal
 * logic of that problem when the defined logic is ALL or QF_ALL.
 */

#include "cvc5_private.h"

#ifndef CVC5__PREPROCESSING__PASSES__MINIMAL_LOGIC_H
#define CVC5__PREPROCESSING__PASSES__MINIMAL_LOGIC_H

#include "preprocessing/preprocessing_pass.h"

namespace cvc5::internal {
namespace preprocessing {
namespace passes {

class MinimalLogic : public PreprocessingPass
{
 public:
  MinimalLogic(PreprocessingPassContext* preprocContext);

 protected:
  PreprocessingPassResult applyInternal(
      AssertionPipeline* assertionsToPreprocess) override;

 private:
  bool quantifiers = false, bitVectors = false, floatingPointers = false,
       arrays = false, strings = false, linear = true, integers = false,
       reals = false;
  bool hasVariable(TNode assertion);
  void traverseAssertion(TNode assertion);
};

}  // namespace passes
}  // namespace preprocessing
}  // namespace cvc5::internal

#endif /* CVC5__PREPROCESSING__PASSES__MINIMAL_LOGIC_H */
