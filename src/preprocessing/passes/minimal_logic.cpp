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

// TODO: improve this verification (just one traversal)
bool MinimalLogic::hasVariable(TNode node)
{
  for (TNode current : NodeDfsIterable(node, VisitOrder::POSTORDER))
    if (current.isVar()) return true;
  return false;
}

void MinimalLogic::traverseAssertion(TNode assertion)
{
  bool floatingPointerFunctions = false, bitVectorLiterals = false,
       bitVectorFunctions = false;

  // TODO: see cache stuff later
  for (TNode current : NodeDfsIterable(assertion, VisitOrder::POSTORDER))
  {
    // Handle quantifiers
    if (util::MinimalLogicUtilities::isQuantifier(current.getKind()))
      quantifiers = true;

    // Handle BV and FP
    if (current.getType().isBitVector()) bitVectorLiterals = true;
    if (util::MinimalLogicUtilities::isBvOperator(current.getKind()))
      bitVectorFunctions = true;
    if (util::MinimalLogicUtilities::isFloatingPointerOperator(
            current.getKind()))
      floatingPointerFunctions = true;

    // Handle arrays
    if (current.getType().isArray()
        || util::MinimalLogicUtilities::isArrayOperator(current.getKind()))
      arrays = true;

    // Handle strings
    if (current.isVar())
    {
      if (current.getType().isString() || current.getType().isSequence()
          || current.getType().isRegExp())
        strings = true;
    }
    if (util::MinimalLogicUtilities::isStringOperator(current.getKind()))
      strings = true;

    // Handle arithmetic
    // Handle integer and reals
    // TODO: check with Haniel later
    if (current.isVar())
    {
      if (current.getType().isInteger()) integers = true;
      if (current.getType().isReal()) reals = true;
    }

    // Handle linearity
    switch (current.getKind())
    {
      case Kind::MULT:
      {
        int variableCount = 0;
        for (TNode child : current)
          if (hasVariable(child)) variableCount++;
        if (variableCount >= 2) linear = false;
        break;
      }
      case Kind::ABS:
      {
        bool hasChildrenVariable = hasVariable(current);
        if (hasChildrenVariable) linear = false;
        break;
      }
      case Kind::DIVISION:
      case Kind::DIVISION_TOTAL:
      case Kind::INTS_DIVISION:
      case Kind::INTS_DIVISION_TOTAL:
      {
        int variableCount = 0;
        for (TNode child : current)
          if (hasVariable(child)) variableCount++;
        if (variableCount >= 2) linear = false;
        break;
      }
      default: break;
    }

    std::cout << current << "\n";
  }

  if (bitVectorLiterals || bitVectorFunctions) bitVectors = true;
  if (bitVectorLiterals && floatingPointerFunctions) floatingPointers = true;

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

  // IDL e RDL ?

  // UF: ?

  std::cout << "quantifiers = " << quantifiers << "\n";
  std::cout << "strings = " << strings << "\n";
  std::cout << "bitvectors = " << bitVectors << "\n";
  std::cout << "floating pointers = " << floatingPointers << "\n";
  std::cout << "linear = " << linear << "\n";
  std::cout << "integers = " << integers << "\n";
  std::cout << "reals = " << reals << "\n";

  return PreprocessingPassResult::NO_CONFLICT;
}

}  // namespace passes
}  // namespace preprocessing
}  // namespace cvc5::internal
