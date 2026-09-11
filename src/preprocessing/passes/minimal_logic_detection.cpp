/******************************************************************************
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2026 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Minimal logic detection preprocessing pass implementation.
 *
 * Implements the minimal logic detection preprocessing pass.
 */

#include "preprocessing/passes/minimal_logic_detection.h"

#include <unordered_set>
#include <vector>

#include "preprocessing/assertion_pipeline.h"
#include "preprocessing/preprocessing_pass_context.h"
#include "smt/env.h"

using namespace std;
using namespace cvc5::internal::kind;
using namespace cvc5::internal::theory;

namespace cvc5::internal {
namespace preprocessing {
namespace passes {

// Handle difference logic (IDL/RDL)?
// Auxiliary struct to store logic features detected during the assertion
// traversal
struct InputLogic
{
  bool hasQuantifiers = false;
  bool hasUF = false;
  bool hasBV = false;
  bool hasArrays = false;
  bool hasDatatypes = false;
  bool hasStrings = false;
  bool hasIntegers = false;
  bool hasReals = false;
  bool isNonlinear = false;
};

MinimalLogicDetection::MinimalLogicDetection(
    PreprocessingPassContext* preprocContext)
    : PreprocessingPass(preprocContext, "minimal-logic-detection") {};

PreprocessingPassResult MinimalLogicDetection::applyInternal(
    AssertionPipeline* assertionsToPreprocess)
{
  Trace("preprocessing") << "PRE logic: " << logicInfo().getLogicString()
                         << std::endl;

  InputLogic logic;
  // Track visited nodes to prevent redundant traversal of shared sub-trees
  std::unordered_set<TNode> visited;

  // Stack of unprocessed nodes in the AST
  std::vector<TNode> visit;
  for (size_t i = 0, size = assertionsToPreprocess->size(); i < size; ++i)
    visit.push_back((*assertionsToPreprocess)[i]);

  while (!visit.empty())
  {
    TNode cur = visit.back();
    visit.pop_back();
    // Skip if already visited
    if (!visited.insert(cur).second) continue;

    Kind k = cur.getKind();
    TypeNode tn = cur.getType();

    if (k == Kind::FORALL || k == Kind::EXISTS) logic.hasQuantifiers = true;
    if (k == Kind::APPLY_UF || tn.isUninterpretedSort()) logic.hasUF = true;

    if (k == Kind::MULT)
    {
      // Count non-constant factors; a product with > 1 variable is non-linear
      size_t nonConstCount = 0;
      for (const TNode& child : cur)
        if (!child.isConst()) nonConstCount++;
      if (nonConstCount > 1) logic.isNonlinear = true;
    }

    // Division or modulus by a non-constant denominator makes the problem
    // non-linear
    if (k == Kind::DIVISION || k == Kind::INTS_DIVISION
        || k == Kind::INTS_MODULUS)
      if (!cur[1].isConst()) logic.isNonlinear = true;

    if (tn.isBitVector())
      logic.hasBV = true;
    else if (tn.isArray())
      logic.hasArrays = true;
    else if (tn.isDatatype())
      logic.hasDatatypes = true;
    else if (tn.isStringLike())
      logic.hasStrings = true;
    else if (tn.isInteger())
      logic.hasIntegers = true;
    else if (tn.isReal())
      logic.hasReals = true;

    // Push all children of the current node in the AST onto the stack
    visit.insert(visit.end(), cur.begin(), cur.end());
  }
  // Construct a fresh, unlocked LogicInfo and enable theories based on what
  // was found in the traversal
  LogicInfo detected = LogicInfo("").getUnlockedCopy();

  if (logic.hasQuantifiers) detected.enableQuantifiers();
  if (logic.hasUF) detected.enableTheory(theory::THEORY_UF);
  if (logic.hasBV) detected.enableTheory(theory::THEORY_BV);
  if (logic.hasArrays) detected.enableTheory(theory::THEORY_ARRAYS);
  if (logic.hasDatatypes) detected.enableTheory(theory::THEORY_DATATYPES);
  if (logic.hasStrings) detected.enableTheory(theory::THEORY_STRINGS);
  if (logic.hasIntegers || logic.hasReals)
  {
    detected.enableTheory(theory::THEORY_ARITH);
    if (logic.hasIntegers) detected.enableIntegers();
    if (logic.hasReals) detected.enableReals();
    if (logic.isNonlinear)
      detected.arithNonLinear();
    else
      detected.arithOnlyLinear();
  }

  // Lock the detected logic and update the environment
  detected.lock();
  d_detectedLogic = detected;
  d_preprocContext->getEnv().setLogicInfo(d_detectedLogic);

  Trace("preprocessing") << "POST logic: " << logicInfo().getLogicString()
                         << std::endl;

  return PreprocessingPassResult::NO_CONFLICT;
}

MinimalLogicDetection::~MinimalLogicDetection() {}

}  // namespace passes
}  // namespace preprocessing
}  // namespace cvc5::internal
