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

#include "preprocessing/passes/minimal_logic.h"

#include "expr/node_algorithm.h"
#include "options/base_options.h"
#include "preprocessing/assertion_pipeline.h"
#include "preprocessing/preprocessing_pass_context.h"

namespace cvc5::internal {
namespace preprocessing {
namespace passes {

MinimalLogic::MinimalLogic(PreprocessingPassContext* preprocContext)
    : PreprocessingPass(preprocContext, "minimal-logic")
{
  std::cout << "minimal logic constructor!\n";
}

PreprocessingPassResult MinimalLogic::applyInternal(
    AssertionPipeline* assertionsToPreprocess)
{
  AlwaysAssert(!options().base.incrementalSolving);

  std::cout << "applying internal for minimal logic preprocessing\n";

  std::cout << "doing other stuff now\n";
  for (const Node& assertion : assertionsToPreprocess->ref())
  {
    std::cout << assertion << "\n";
  }

  return PreprocessingPassResult::NO_CONFLICT;
}

}  // namespace passes
}  // namespace preprocessing
}  // namespace cvc5::internal
