###############################################################################
# Top contributors (to current version):
#   Aina Niemetz
#
# This file is part of the cvc5 project.
#
# Copyright (c) 2009-2024 by the authors listed in the file AUTHORS
# in the top-level source directory and their institutional affiliations.
# All rights reserved.  See the file COPYING in the top-level source
# directory for licensing information.
# #############################################################################
#
# The build system configuration.
##

#-----------------------------------------------------------------------------#
# Add target 'capitests', builds and runs
# > api tests

add_custom_target(build-capitests)
add_dependencies(build-tests build-capitests)

# Disabled temporarily due to problems in the nightly builds
# # See the comment about 'make test' in test/regress/cli/CMakeLists.txt
# add_test(build_capitests_test "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --config "$<CONFIG>" --target build-capitests)
# set_tests_properties(build_capitests_test PROPERTIES FIXTURES_SETUP build_capitests_fixture)

add_custom_target(capitests
  COMMAND ctest --output-on-failure -L "api" -j${CTEST_NTHREADS} $$ARGS
  DEPENDS build-capitests)

set(CVC5_API_TEST_FLAGS -D__BUILDING_CVC5_API_TEST -Dcvc5_obj_EXPORTS)

macro(cvc5_add_capi_test name)
  set(test_bin_dir ${CMAKE_BINARY_DIR}/bin/test/api/c)
  set(testname capi_${name})
  add_executable(${testname} ${name}.c)
  target_link_libraries(${testname} PUBLIC main-test)
  target_compile_definitions(${testname} PRIVATE ${CVC5_API_TEST_FLAGS})
  set_target_properties(${testname}
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${test_bin_dir})
  add_test(api/c/${testname} ${ENV_PATH_CMD} ${test_bin_dir}/${testname})
  set_tests_properties(api/c/${testname} PROPERTIES LABELS "api capi")
  # set_tests_properties(api/cpp/${name} PROPERTIES FIXTURES_REQUIRED build_capitests_fixture)
  add_dependencies(build-capitests ${testname})
endmacro()

cvc5_add_capi_test(boilerplate)
#cvc5_add_capi_test(ouroborous)
#cvc5_add_capi_test(reset_assertions)
#cvc5_add_capi_test(sep_log_api)
cvc5_add_capi_test(smt2_compliance)
#cvc5_add_capi_test(two_solvers)
#cvc5_add_capi_test(issue5074)
#cvc5_add_capi_test(issue5893)
#cvc5_add_capi_test(issue4889)
#cvc5_add_capi_test(issue6111)
#cvc5_add_capi_test(issue7000)
#cvc5_add_capi_test(proj-issue306)
#cvc5_add_capi_test(proj-issue308)
#cvc5_add_capi_test(proj-issue334)
#cvc5_add_capi_test(proj-issue337)
#cvc5_add_capi_test(proj-issue344)
#cvc5_add_capi_test(proj-issue345)
#cvc5_add_capi_test(proj-issue373)
#cvc5_add_capi_test(proj-issue377)
#cvc5_add_capi_test(proj-issue378)
#cvc5_add_capi_test(proj-issue379)
#cvc5_add_capi_test(proj-issue381)
#cvc5_add_capi_test(proj-issue382)
#cvc5_add_capi_test(proj-issue383)
#cvc5_add_capi_test(proj-issue386)
#cvc5_add_capi_test(proj-issue388)
#cvc5_add_capi_test(proj-issue395)
#cvc5_add_capi_test(proj-issue399)
#cvc5_add_capi_test(proj-issue413)
#cvc5_add_capi_test(proj-issue414)
#cvc5_add_capi_test(proj-issue416)
#cvc5_add_capi_test(proj-issue418)
#cvc5_add_capi_test(proj-issue420)
#cvc5_add_capi_test(proj-issue421)
#cvc5_add_capi_test(proj-issue422)
#cvc5_add_capi_test(proj-issue423)
#cvc5_add_capi_test(proj-issue426)
#cvc5_add_capi_test(proj-issue429)
#cvc5_add_capi_test(proj-issue431)
#cvc5_add_capi_test(proj-issue434)
#cvc5_add_capi_test(proj-issue435)
#cvc5_add_capi_test(proj-issue436)
#cvc5_add_capi_test(proj-issue440)
#cvc5_add_capi_test(proj-issue445)
#cvc5_add_capi_test(proj-issue455)
#cvc5_add_capi_test(proj-issue484)
#cvc5_add_capi_test(proj-issue538)
#cvc5_add_capi_test(proj-issue567)
#cvc5_add_capi_test(proj-issue570)
#cvc5_add_capi_test(proj-issue573)
#cvc5_add_capi_test(proj-issue574)
#cvc5_add_capi_test(proj-issue575)
#cvc5_add_capi_test(proj-issue576)
#cvc5_add_capi_test(proj-issue580)
#cvc5_add_capi_test(proj-issue581)
#cvc5_add_capi_test(proj-issue587)
#cvc5_add_capi_test(proj-issue600)
#cvc5_add_capi_test(proj-issue611)
#cvc5_add_capi_test(proj-issue612)
#cvc5_add_capi_test(proj-issue618)
#cvc5_add_capi_test(proj-issue621)
#cvc5_add_capi_test(proj-issue644)
#cvc5_add_capi_test(proj-issue646)
#cvc5_add_capi_test(proj-issue652)
#cvc5_add_capi_test(proj-issue654)
#cvc5_add_capi_test(proj-issue655)
#cvc5_add_capi_test(proj-issue656)
#cvc5_add_capi_test(proj-issue657)
#cvc5_add_capi_test(proj-issue666)
