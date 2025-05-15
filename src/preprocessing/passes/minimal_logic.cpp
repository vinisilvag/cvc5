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
#include "expr/node_traversal.h"
#include "options/base_options.h"
#include "preprocessing/assertion_pipeline.h"
#include "preprocessing/preprocessing_pass_context.h"
#include "preprocessing/util/minimal_logic_utilities.h"

namespace cvc5::internal {
namespace preprocessing {
namespace passes {

MinimalLogic::MinimalLogic(PreprocessingPassContext* preprocContext)
    : PreprocessingPass(preprocContext, "minimal-logic")
{
}

void MinimalLogic::traverseAssertion(TNode assertion)
{
  bool hasFloatingPointerFunctions = false, hasBitVectorLiterals = false,
       hasBitVectorFunctions = false;

  std::unordered_map<TNode, bool> contains_var;

  for (TNode current : NodeDfsIterable(
           assertion, VisitOrder::POSTORDER, [&contains_var](TNode nn) {
             // Contains variable map used as cache
             return contains_var.count(nn);
           }))
  {
    // Handle quantifiers
    if (util::MinimalLogicUtilities::isQuantifier(current.getKind()))
      hasQuantifiers = true;

    // Handle BV and FP
    if (current.getType().isBitVector()) hasBitVectorLiterals = true;
    if (util::MinimalLogicUtilities::isBvOperator(current.getKind()))
      hasBitVectorFunctions = true;
    if (util::MinimalLogicUtilities::isFloatingPointerOperator(
            current.getKind()))
      hasFloatingPointerFunctions = true;

    // Handle arrays
    if (current.getType().isArray()
        || util::MinimalLogicUtilities::isArrayOperator(current.getKind()))
      hasArrays = true;

    // Handle strings
    if (current.isVar())
    {
      if (current.getType().isString() || current.getType().isSequence()
          || current.getType().isRegExp())
        hasStrings = true;
    }
    if (util::MinimalLogicUtilities::isStringOperator(current.getKind()))
      hasStrings = true;

    // Handle arithmetic
    // Handle integer and reals
    if (current.isVar())
    {
      if (current.getType().isInteger()) hasIntegers = true;
      if (current.getType().isReal()) hasReals = true;
    }

    if (current.isVar())
    {
      contains_var[current] = true;
    }
    else
    {
      bool has_var = false;
      for (const TNode& child : current)
      {
        has_var |= contains_var[child];
      }
      contains_var[current] = has_var;
    }

    // Handle linearity
    // Do this only if the problem is still linear
    if (isLinear == true)
    {
      switch (current.getKind())
      {
        case Kind::DIVISION:
        case Kind::DIVISION_TOTAL:
        case Kind::INTS_DIVISION:
        case Kind::INTS_DIVISION_TOTAL:
        case Kind::MULT:
        {
          int variableCount = 0;
          for (TNode child : current)
            if (contains_var[child]) variableCount++;
          if (variableCount >= 2) isLinear = false;
          break;
        }
        case Kind::ABS:
        {
          if (contains_var[current]) isLinear = false;
          break;
        }
        default: break;
      }
    }

    // Handle IDL and RDL
    // if (!util::MinimalLogicUtilities::isDifferenceLogicOperator(
    //         current.getKind()))
    // {
    //   idl = false;
    //   rdl = false;
    // }
    // else
    // {
    //   // Check the form
    // }

    // Handle UF (partially)
    if (current.getKind() == Kind::APPLY_UF
        || current.getKind() == Kind::UNINTERPRETED_SORT_VALUE)
    {
      isUF = true;
    }

    std::cout << current << "\n";
  }

  if (hasBitVectorLiterals || hasBitVectorFunctions) hasBitVectors = true;
  if (hasBitVectorLiterals && hasFloatingPointerFunctions)
    hasFloatingPointers = true;

  std::cout << "end assertion\n\n";
}

PreprocessingPassResult MinimalLogic::applyInternal(
    AssertionPipeline* assertionsToPreprocess)
{
  // TODO: check this later
  // AlwaysAssert(!options().base.incrementalSolving);
  std::cout << "applying internal for minimal logic preprocessing\n";
  std::cout << "assertions\n";
  for (const Node& assertion : assertionsToPreprocess->ref())
  {
    std::cout << assertion << "\n";
  }

  std::cout << "\nprocessing:\n";

  for (const Node& assertion : assertionsToPreprocess->ref())
  {
    traverseAssertion(assertion);
  }

  // UF: ?

  std::cout << "quantifiers = " << hasQuantifiers << "\n";
  std::cout << "strings = " << hasStrings << "\n";
  std::cout << "bitvectors = " << hasBitVectors << "\n";
  std::cout << "floating pointers = " << hasFloatingPointers << "\n";
  std::cout << "linear = " << isLinear << "\n";
  std::cout << "integers = " << hasIntegers << "\n";
  std::cout << "reals = " << hasReals << "\n";

  return PreprocessingPassResult::NO_CONFLICT;
}

}  // namespace passes
}  // namespace preprocessing
}  // namespace cvc5::internal
