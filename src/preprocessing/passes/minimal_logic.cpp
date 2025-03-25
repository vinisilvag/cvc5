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

namespace cvc5::internal {
namespace preprocessing {
namespace passes {

MinimalLogic::MinimalLogic(PreprocessingPassContext* preprocContext)
    : PreprocessingPass(preprocContext, "minimal-logic")
{
}

bool isStringOperator(Kind node)
{
  switch (node)
  {
    case Kind::SEQUENCE_TYPE:
    case Kind::CONST_SEQUENCE:
    case Kind::SEQ_UNIT:
    case Kind::SEQ_NTH:
    case Kind::STRING_TO_REGEXP:
    case Kind::REGEXP_CONCAT:
    case Kind::REGEXP_UNION:
    case Kind::REGEXP_INTER:
    case Kind::REGEXP_DIFF:
    case Kind::REGEXP_STAR:
    case Kind::REGEXP_PLUS:
    case Kind::REGEXP_OPT:
    case Kind::REGEXP_RANGE:
    case Kind::REGEXP_COMPLEMENT:
    case Kind::REGEXP_NONE:
    case Kind::REGEXP_ALL:
    case Kind::REGEXP_ALLCHAR:
    case Kind::REGEXP_REPEAT_OP:
    case Kind::REGEXP_REPEAT:
    case Kind::REGEXP_LOOP_OP:
    case Kind::REGEXP_LOOP:
    case Kind::REGEXP_RV:
    case Kind::SEQ_EMPTY_OF_TYPE:
    {
      return true;
    }
    default: break;
  }

  return false;
}

bool MinimalLogic::hasVariable(TNode node)
{
  for (TNode current : NodeDfsIterable(node, VisitOrder::POSTORDER))
    if (current.isVar()) return true;
  return false;
}

void MinimalLogic::traverseAssertion(TNode assertion)
{
  // BV -> bitvector literals and function symbols of BV theory
  // FP -> bitvector literals
  bool bitVectorLiterals = false, bitVectorFunctions = false;

  // TODO: see cache stuff later
  for (TNode current : NodeDfsIterable(assertion, VisitOrder::POSTORDER))
  {
    // Handle quantifiers
    switch (current.getKind())
    {
      case Kind::FORALL:
      case Kind::EXISTS:
      case Kind::LAMBDA:
      {
        quantifiers = true;
        break;
      }
      default: break;
    }

    // Handle BV and FP
    if (current.getType().isBitVector()) bitVectorLiterals = true;
    switch (current.getKind())
    {
      // List all BV operators
      default: break;
    }

    // Handle arrays
    if (current.getType().isArray()) arrays = true;
    switch (current.getKind())
    {
      case Kind::ARRAY_TYPE:
      case Kind::SELECT:
      case Kind::STORE:
      case Kind::EQ_RANGE:
      case Kind::STORE_ALL:
      {
        arrays = true;
        break;
      }
      default: break;
    }

    // Handle strings
    // TODO: check with Haniel later
    if (current.isVar())
    {
      if (current.getType().isString() || current.getType().isSequence()
          || current.getType().isRegExp())
        strings = true;
    }
    if (isStringOperator(current.getKind())) strings = true;

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

  if (bitVectorLiterals && bitVectorFunctions) bitVectors = true;
  if (bitVectorLiterals && !bitVectorFunctions) floatingPointers = true;

  std::cout << "end assertion\n\n";
}

PreprocessingPassResult MinimalLogic::applyInternal(
    AssertionPipeline* assertionsToPreprocess)
{
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

  // A or AX: ?

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
