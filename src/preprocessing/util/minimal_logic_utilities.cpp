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

#include "preprocessing/util/minimal_logic_utilities.h"

namespace cvc5::internal {
namespace preprocessing {
namespace util {

bool MinimalLogicUtilities::isBvOperator(Kind nodeKind)
{
  switch (nodeKind)
  {
    case Kind::BITVECTOR_TYPE:
    case Kind::CONST_BITVECTOR:
    case Kind::BITVECTOR_FROM_BOOLS:
    case Kind::BITVECTOR_SIZE:
    case Kind::CONST_BITVECTOR_SYMBOLIC:
    case Kind::BITVECTOR_CONCAT:
    case Kind::BITVECTOR_AND:
    case Kind::BITVECTOR_COMP:
    case Kind::BITVECTOR_OR:
    case Kind::BITVECTOR_XOR:
    case Kind::BITVECTOR_NOT:
    case Kind::BITVECTOR_NAND:
    case Kind::BITVECTOR_NOR:
    case Kind::BITVECTOR_XNOR:
    case Kind::BITVECTOR_MULT:
    case Kind::BITVECTOR_NEG:
    case Kind::BITVECTOR_ADD:
    case Kind::BITVECTOR_SUB:
    case Kind::BITVECTOR_UDIV:
    case Kind::BITVECTOR_UREM:
    case Kind::BITVECTOR_SDIV:
    case Kind::BITVECTOR_SMOD:
    case Kind::BITVECTOR_SREM:
    case Kind::BITVECTOR_ASHR:
    case Kind::BITVECTOR_LSHR:
    case Kind::BITVECTOR_SHL:
    case Kind::BITVECTOR_ULE:
    case Kind::BITVECTOR_ULT:
    case Kind::BITVECTOR_UGE:
    case Kind::BITVECTOR_UGT:
    case Kind::BITVECTOR_SLE:
    case Kind::BITVECTOR_SLT:
    case Kind::BITVECTOR_SGE:
    case Kind::BITVECTOR_SGT:
    case Kind::BITVECTOR_ULTBV:
    case Kind::BITVECTOR_SLTBV:
    case Kind::BITVECTOR_REDAND:
    case Kind::BITVECTOR_REDOR:
    case Kind::BITVECTOR_NEGO:
    case Kind::BITVECTOR_UADDO:
    case Kind::BITVECTOR_SADDO:
    case Kind::BITVECTOR_UMULO:
    case Kind::BITVECTOR_SMULO:
    case Kind::BITVECTOR_USUBO:
    case Kind::BITVECTOR_SSUBO:
    case Kind::BITVECTOR_SDIVO:
    case Kind::BITVECTOR_ITE:
    case Kind::BITVECTOR_ACKERMANNIZE_UDIV:
    case Kind::BITVECTOR_ACKERMANNIZE_UREM:
    case Kind::BITVECTOR_EAGER_ATOM:
    case Kind::BITVECTOR_BIT_OP:
    case Kind::BITVECTOR_BIT:
    case Kind::BITVECTOR_EXTRACT_OP:
    case Kind::BITVECTOR_EXTRACT:
    case Kind::BITVECTOR_REPEAT_OP:
    case Kind::BITVECTOR_REPEAT:
    case Kind::BITVECTOR_ROTATE_LEFT_OP:
    case Kind::BITVECTOR_ROTATE_LEFT:
    case Kind::BITVECTOR_ROTATE_RIGHT_OP:
    case Kind::BITVECTOR_ROTATE_RIGHT:
    case Kind::BITVECTOR_SIGN_EXTEND_OP:
    case Kind::BITVECTOR_SIGN_EXTEND:
    case Kind::BITVECTOR_ZERO_EXTEND_OP:
    case Kind::BITVECTOR_ZERO_EXTEND:
    {
      return true;
    }
    default: break;
  }
  return false;
}

bool MinimalLogicUtilities::isFloatingPointerOperator(Kind nodeKind)
{
  switch (nodeKind)
  {
    case Kind::CONST_FLOATINGPOINT:
    case Kind::CONST_ROUNDINGMODE:
    case Kind::FLOATINGPOINT_TYPE:
    case Kind::FLOATINGPOINT_FP:
    case Kind::FLOATINGPOINT_EQ:
    case Kind::FLOATINGPOINT_ABS:
    case Kind::FLOATINGPOINT_NEG:
    case Kind::FLOATINGPOINT_ADD:
    case Kind::FLOATINGPOINT_SUB:
    case Kind::FLOATINGPOINT_MULT:
    case Kind::FLOATINGPOINT_DIV:
    case Kind::FLOATINGPOINT_FMA:
    case Kind::FLOATINGPOINT_SQRT:
    case Kind::FLOATINGPOINT_REM:
    case Kind::FLOATINGPOINT_RTI:
    case Kind::FLOATINGPOINT_MIN:
    case Kind::FLOATINGPOINT_MAX:
    case Kind::FLOATINGPOINT_MIN_TOTAL:
    case Kind::FLOATINGPOINT_MAX_TOTAL:
    case Kind::FLOATINGPOINT_LEQ:
    case Kind::FLOATINGPOINT_LT:
    case Kind::FLOATINGPOINT_GEQ:
    case Kind::FLOATINGPOINT_GT:
    case Kind::FLOATINGPOINT_IS_NORMAL:
    case Kind::FLOATINGPOINT_IS_SUBNORMAL:
    case Kind::FLOATINGPOINT_IS_ZERO:
    case Kind::FLOATINGPOINT_IS_INF:
    case Kind::FLOATINGPOINT_IS_NAN:
    case Kind::FLOATINGPOINT_IS_NEG:
    case Kind::FLOATINGPOINT_IS_POS:
    case Kind::FLOATINGPOINT_TO_FP_FROM_IEEE_BV_OP:
    case Kind::FLOATINGPOINT_TO_FP_FROM_IEEE_BV:
    case Kind::FLOATINGPOINT_TO_FP_FROM_FP_OP:
    case Kind::FLOATINGPOINT_TO_FP_FROM_FP:
    case Kind::FLOATINGPOINT_TO_FP_FROM_REAL_OP:
    case Kind::FLOATINGPOINT_TO_FP_FROM_REAL:
    case Kind::FLOATINGPOINT_TO_FP_FROM_SBV_OP:
    case Kind::FLOATINGPOINT_TO_FP_FROM_SBV:
    case Kind::FLOATINGPOINT_TO_FP_FROM_UBV_OP:
    case Kind::FLOATINGPOINT_TO_FP_FROM_UBV:
    case Kind::FLOATINGPOINT_TO_UBV_OP:
    case Kind::FLOATINGPOINT_TO_UBV:
    case Kind::FLOATINGPOINT_TO_UBV_TOTAL_OP:
    case Kind::FLOATINGPOINT_TO_UBV_TOTAL:
    case Kind::FLOATINGPOINT_TO_SBV_OP:
    case Kind::FLOATINGPOINT_TO_SBV:
    case Kind::FLOATINGPOINT_TO_SBV_TOTAL_OP:
    case Kind::FLOATINGPOINT_TO_SBV_TOTAL:
    case Kind::FLOATINGPOINT_TO_REAL:
    case Kind::FLOATINGPOINT_TO_REAL_TOTAL:
    case Kind::FLOATINGPOINT_COMPONENT_NAN:
    case Kind::FLOATINGPOINT_COMPONENT_INF:
    case Kind::FLOATINGPOINT_COMPONENT_ZERO:
    case Kind::FLOATINGPOINT_COMPONENT_SIGN:
    case Kind::FLOATINGPOINT_COMPONENT_EXPONENT:
    case Kind::FLOATINGPOINT_COMPONENT_SIGNIFICAND:
    case Kind::ROUNDINGMODE_BITBLAST:
    {
      return true;
    }
    default: break;
  }
  return false;
}

bool MinimalLogicUtilities::isStringOperator(Kind nodeKind)
{
  switch (nodeKind)
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

bool MinimalLogicUtilities::isArrayOperator(Kind nodeKind)
{
  switch (nodeKind)
  {
    case Kind::ARRAY_TYPE:
    case Kind::SELECT:
    case Kind::STORE:
    case Kind::EQ_RANGE:
    case Kind::STORE_ALL:
    {
      return true;
    }
    default: break;
  }
  return false;
}

bool MinimalLogicUtilities::isQuantifier(Kind nodeKind)
{
  switch (nodeKind)
  {
    case Kind::FORALL:
    case Kind::EXISTS:
    case Kind::INST_CONSTANT:
    case Kind::ORACLE:
    case Kind::BOUND_VAR_LIST:
    case Kind::INST_PATTERN:
    case Kind::INST_NO_PATTERN:
    case Kind::INST_ATTRIBUTE:
    case Kind::INST_POOL:
    case Kind::INST_ADD_TO_POOL:
    case Kind::SKOLEM_ADD_TO_POOL:
    case Kind::ORACLE_FORMULA_GEN:
    case Kind::INST_PATTERN_LIST:
    {
      return true;
    }
    default: break;
  }
  return false;
}

bool MinimalLogicUtilities::isDifferenceLogicOperator(Kind nodeKind)
{
  switch (nodeKind)
  {
    case Kind::LT:
    case Kind::LEQ:
    case Kind::GT:
    case Kind::GEQ:
    case Kind::EQUAL:
    case Kind::DISTINCT:
    {
      return true;
    }
    default: break;
  }
  return false;
}

}  // namespace util
}  // namespace preprocessing
}  // namespace cvc5::internal
