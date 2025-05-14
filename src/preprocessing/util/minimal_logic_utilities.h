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
 * Stores utility functions that will be used in the process of identifying the
 * minimal logic of an input problem whose specified logic is either ALL or
 * QF_ALL.
 */

#include "cvc5_private.h"

#ifndef CVC5__MINIMAL_LOGIC_UTILITIES_H
#define CVC5__MINIMAL_LOGIC_UTILITIES_H

#include "expr/node_algorithm.h"

namespace cvc5::internal {
namespace preprocessing {

namespace util {

class MinimalLogicUtilities
{
 public:
  static bool isBvOperator(Kind nodeKind);
  static bool isFloatingPointerOperator(Kind nodeKind);
  static bool isStringOperator(Kind nodeKind);
  static bool isArrayOperator(Kind nodeKind);
  static bool isQuantifier(Kind nodeKind);
  static bool isDifferenceLogicOperator(Kind nodeKind);
};

}  // namespace util
}  // namespace preprocessing
}  // namespace cvc5::internal

#endif /* CVC5__MINIMAL_LOGIC_UTILITIES_H */
