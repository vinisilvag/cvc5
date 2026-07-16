#include "preprocessing/passes/minimal_logic_detection.h"

#include "expr/node_algorithm.h"
#include "options/base_options.h"
#include "preprocessing/assertion_pipeline.h"
#include "preprocessing/preprocessing_pass_context.h"

namespace cvc5::internal {
namespace preprocessing {
namespace passes {

MinimalLogicDetection::MinimalLogicDetection(
    PreprocessingPassContext* preprocContext)
    : PreprocessingPass(preprocContext, "minimal-logic-detection")
{
}

PreprocessingPassResult MinimalLogicDetection::applyInternal(
    AssertionPipeline* assertionsToPreprocess)
{
  std::cout << "applying internal for minimal logic preprocessing\n";

  std::cout << "after preprocessing, the theories identified are\n";
  std::cout << "<list theories>\n";

  return PreprocessingPassResult::NO_CONFLICT;
}

}  // namespace passes
}  // namespace preprocessing
}  // namespace cvc5::internal
