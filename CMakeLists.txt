#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

project(rmm-acs LANGUAGES)

### Tool dependency check - start ###

# Set the minimum required version of CMake for the project
cmake_minimum_required(VERSION 3.19)

# cmake_policy
cmake_policy(SET CMP0057 NEW)

### Tool dependency check - end ###

find_package(Git REQUIRED)

get_filename_component(ROOT_DIR . ABSOLUTE)

# Set internal build directory variable
set(BUILD ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "Setting build directory to ${BUILD}" FORCE)
file(MAKE_DIRECTORY ${BUILD}/output/)

# Set global compile list variable
set(COMPILE_LIST "")

add_definitions(-DQCBOR_DISABLE_FLOAT_HW_USE)

#### Include cmake support module ###
include(${ROOT_DIR}/tools/cmake/toolchain/utils.cmake)
include(${ROOT_DIR}/tools/cmake/toolchain/default.cmake)
include(${ROOT_DIR}/tools/cmake/common/CMakeExternal.cmake)
####


### Valid value range for command line argument ###

list(APPEND VERBOSE_LIST 1 2 3 4 5)
list(APPEND ENABLE_PIE_LIST ON OFF)
list(APPEND TEST_COMBINE_LIST ON OFF)
list(APPEND CMAKE_BUILD_TYPE_LIST Release Debug)
list(APPEND ARM_ARCH_MAJOR_LIST 8 9)
list(APPEND SECURE_TEST_ENABLE_LIST 1)

###

### Command line argument check ###

if(CMAKE_BUILD_TYPE)
    if(NOT ${CMAKE_BUILD_TYPE} IN_LIST CMAKE_BUILD_TYPE_LIST)
        message(FATAL_ERROR "[ACS] : Error: Unspported value for -DCMAKE_BUILD_TYPE=, supported values are : ${CMAKE_BUILD_TYPE_LIST}")
    endif()
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE INTERNAL "CMAKE_BUILD_TYPE is set to ${CMAKE_BUILD_TYPE}" FORCE)
else()
    set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_DFLT} CACHE INTERNAL "Defaulting CMAKE_BUILD_TYPE to ${CMAKE_BUILD_TYPE_DFLT}" FORCE)
endif()

# Check for valid targets
_get_sub_dir_list(TARGET_LIST ${ROOT_DIR}/plat/targets/)
if(NOT DEFINED TARGET)
    set(TARGET ${TARGET_DFLT} CACHE INTERNAL "Defaulting target to ${TARGET}" FORCE)
else()
    set(TARGET ${TARGET} CACHE INTERNAL "TARGET is set to ${TARGET}" FORCE)
endif()

if(NOT ${TARGET} IN_LIST TARGET_LIST)
    message(FATAL_ERROR "[ACS] : Error: Unspported value for -DTARGET=, supported targets are : ${TARGET_LIST}")
else()
    message(STATUS "[ACS] : TARGET is set to ${TARGET}")
endif()

# Check for valid suites
_get_sub_dir_list(SUITE_LIST ${ROOT_DIR}/test/)
if(NOT DEFINED SUITE)
    set(SUITE ${SUITE_DFLT} CACHE INTERNAL "Defaulting suite to ${SUITE}" FORCE)
else()
    set(SUITE ${SUITE} CACHE INTERNAL "SUITE is set to ${SUITE}" FORCE)
endif()

list(APPEND SUITE_LIST all)
if(NOT ${SUITE} IN_LIST SUITE_LIST)
    message(FATAL_ERROR "[ACS] : Error: Unspported value for -DSUITE=, supported values are : ${SUITE_LIST}")
else()
    message(STATUS "[ACS] : SUITE is set to ${SUITE}")
endif()

# Check for VERBOSE
if(NOT DEFINED VERBOSE)
    set(VERBOSE ${VERBOSE_DFLT} CACHE INTERNAL "Default VERBOSE value" FORCE)
        message(STATUS "[ACS] : Defaulting VERBOSE to ${VERBOSE}")
else()
    if(NOT ${VERBOSE} IN_LIST VERBOSE_LIST)
        message(FATAL_ERROR "[ACS] : Error: Unspported value for -DVERBOSE=, supported values are : ${VERBOSE_LIST}")
    endif()
    message(STATUS "[ACS] : VERBOSE is set to ${VERBOSE}")
endif()

# Check for ARM_ARCH_MAJOR
if(NOT DEFINED ARM_ARCH_MAJOR)
    set(ARM_ARCH_MAJOR "${ARM_ARCH_MAJOR_DFLT}" CACHE INTERNAL "Default ARM_ARCH_MAJOR value" FORCE)
        message(STATUS "[ACS] : Defaulting ARM_ARCH_MAJOR to ${ARM_ARCH_MAJOR}")
else()
    if(NOT ${ARM_ARCH_MAJOR} IN_LIST ARM_ARCH_MAJOR_LIST)
        message(FATAL_ERROR "[ACS] : Error: Unspported value for -DARM_ARCH_MAJOR=, supported values are : ${ARM_ARCH_MAJOR_LIST}")
    endif()
    message(STATUS "[ACS] : ARM_ARCH_MAJOR is set to ${ARM_ARCH_MAJOR}")
endif()

# Check for ARM_ARCH_MINOR
if(NOT DEFINED ARM_ARCH_MINOR)
    set(ARM_ARCH_MINOR "${ARM_ARCH_MINOR_DFLT}" CACHE INTERNAL "Default ARM_ARCH_MINOR value" FORCE)
        message(STATUS "[ACS] : Defaulting ARM_ARCH_MINOR to ${ARM_ARCH_MINOR}")
else()
    message(STATUS "[ACS] : ARM_ARCH_MINOR is set to ${ARM_ARCH_MINOR}")
endif()

# Check for ENABLE_PIE
if(NOT DEFINED ENABLE_PIE)
    set(ENABLE_PIE ${ENABLE_PIE_DFLT} CACHE INTERNAL "Default ENABLE_PIE value" FORCE)
        message(STATUS "[ACS] : Defaulting ENABLE_PIE to ${ENABLE_PIE}")
else()
    if(NOT ${ENABLE_PIE} IN_LIST ENABLE_PIE_LIST)
        message(FATAL_ERROR "[ACS] : Error: Unspported value for -DENABLE_PIE=, supported values are : ${ENABLE_PIE_LIST}")
    endif()
    message(STATUS "[ACS] : ENABLE_PIE is set to ${ENABLE_PIE}")
endif()

# Check for TEST_COMBINE
if(NOT DEFINED TEST_COMBINE)
    set(TEST_COMBINE ${TEST_COMBINE_DFLT} CACHE INTERNAL "Default TEST_COMBINE value" FORCE)
        message(STATUS "[ACS] : Defaulting TEST_COMBINE to ${TEST_COMBINE}")
else()
    if(NOT ${TEST_COMBINE} IN_LIST TEST_COMBINE_LIST)
        message(FATAL_ERROR "[ACS] : Error: Unspported value for -DTEST_COMBINE=, supported values are : ${TEST_COMBINE_LIST}")
    endif()
    message(STATUS "[ACS] : TEST_COMBINE is set to ${TEST_COMBINE}")
endif()

if(NOT DEFINED SREC_CAT)
    message(FATAL_ERROR "SREC_CAT is undefined. Set with srec_cat utility path")
else()
    message(STATUS "[ACS] : SREC_CAT is set to ${SREC_CAT}")
endif ()

if(DEFINED SUITE_TEST_RANGE)
    if(DEFINED SUITE)
        set(SUITE ${SUITE_DFLT} CACHE INTERNAL "Defaulting suite to ${SUITE}" FORCE)
        message(WARNING "[ACS] : SUITE is set to all when SUITE_TEST_RANGE is defined ")
    endif()
    add_definitions(-DSUITE_TEST_RANGE)
    list(LENGTH SUITE_TEST_RANGE SUITE_TEST_RANGE_LENGTH)
    if(${SUITE_TEST_RANGE_LENGTH} GREATER "2")
        message(FATAL_ERROR "[ACS] : -DSUITE_TEST_RANGE=<...> value error! accepts two "
                                             " numbers in quotes separated with ';'")
    endif()
    if(${SUITE_TEST_RANGE_LENGTH} EQUAL "2")
        list(GET SUITE_TEST_RANGE 0 SUITE_TEST_RANGE_MIN)
        list(GET SUITE_TEST_RANGE 1 SUITE_TEST_RANGE_MAX)
        message(STATUS "[ACS] : Testing (${SUITE_TEST_RANGE_MIN}, ${SUITE_TEST_RANGE_MAX}) of ${SUITE} suite")
    endif()
    if(${SUITE_TEST_RANGE_LENGTH} EQUAL "1")
        set(SUITE_TEST_RANGE_MIN ${SUITE_TEST_RANGE})
        set(SUITE_TEST_RANGE_MAX ${SUITE_TEST_RANGE})
        message(STATUS "[ACS] : Testing ${SUITE_TEST_RANGE_MIN} of ${SUITE} suite")
    endif()
    message(STATUS "[ACS] : Test range is (${SUITE_TEST_RANGE_MIN}, ${SUITE_TEST_RANGE_MAX}) of ${SUITE} suite")
    add_definitions(-DSUITE_TEST_RANGE_MIN="${SUITE_TEST_RANGE_MIN}")
    add_definitions(-DSUITE_TEST_RANGE_MAX="${SUITE_TEST_RANGE_MAX}")
endif()

# Check for SECURE_TEST_ENABLE
if(NOT DEFINED SECURE_TEST_ENABLE)
    message(STATUS "[ACS] : SECURE_TEST_ENABLE is not define so secure tests will be skipped.")
else()
    if(NOT ${SECURE_TEST_ENABLE} IN_LIST SECURE_TEST_ENABLE_LIST)
        message(FATAL_ERROR "[ACS] : Error: Unspported value for -DSECURE_TEST_ENABLE=, supported values are : ${SECURE_TEST_ENABLE_LIST}")
    endif()
    add_definitions(-DSECURE_TEST_ENABLE)
    message(STATUS "[ACS] : SECURE_TEST_ENABLE is define and secure test will run.")
endif()

if((${SUITE} STREQUAL "attestation_measurement") OR (${SUITE} STREQUAL "all"))
    set(RMM_ACS_TARGET_QCBOR		${CMAKE_CURRENT_BINARY_DIR}/rmm_acs_qcbor	CACHE PATH "Location of Q_CBOR sources.")
    set(RMM_ACS_QCBOR_INCLUDE_PATH      ${RMM_ACS_TARGET_QCBOR}/inc)
endif()

if(((${SUITE} STREQUAL "attestation_measurement") OR (${SUITE} STREQUAL "all")) AND (NOT EXISTS ${RMM_ACS_TARGET_QCBOR}))
execute_process(COMMAND ${GIT_EXECUTABLE} clone ${QCBOR_GIT_REPO_LINK} ${RMM_ACS_TARGET_QCBOR}
	RESULT_VARIABLE qcbor_clone_result
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
if(qcbor_clone_result)
	message(FATAL_ERROR "git clone failed for ${QCBOR_GIT_REPO_LINK}")
endif()

if(NOT qcbor_clone_result)
execute_process(COMMAND ${GIT_EXECUTABLE} checkout -q "${QCBOR_GIT_REPO_TAG}"
	RESULT_VARIABLE qcbor_checkout_result
	WORKING_DIRECTORY ${RMM_ACS_TARGET_QCBOR})
if(qcbor_checkout_result)
	message(FATAL_ERROR "git checkout failed for Repo : ${QCBOR_GIT_REPO_LINK}, Tag : ${QCBOR_GIT_REPO_TAG}")
endif()
endif()
endif()

# Setup toolchain parameters for compilation and link
include(${ROOT_DIR}/tools/cmake/toolchain/common.cmake)

###

### Add compilation macros ###

# Global macro to identify the ACS test suite cmake build
add_definitions(-DCMAKE_BUILD)
add_definitions(-DVERBOSITY=${VERBOSE})
add_definitions(-Dd_${SUITE})
add_definitions(-DPLAT_XLAT_TABLES_DYNAMIC)

if(${TEST_COMBINE})
add_definitions(-DTEST_COMBINE)
endif()
####

### Cmake clean target ###
list(APPEND CLEAN_LIST
        ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_HEADER}
        ${CMAKE_CURRENT_BINARY_DIR}/output
)

# Include the files for make clean
foreach(clean_item ${CLEAN_LIST})
        set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${clean_item})
endforeach()
###

### Compile host, realm and secure val/pal/test sources and create output binaries
add_subdirectory(${ROOT_DIR}/tools/cmake/acs_host)
add_subdirectory(${ROOT_DIR}/tools/cmake/acs_realm)
add_subdirectory(${ROOT_DIR}/tools/cmake/acs_secure)

### Throw waring for the files which is not compiled ###

list(REMOVE_DUPLICATES COMPILE_LIST)
execute_process(COMMAND python ${ROOT_DIR}/tools/scripts/compile_check.py "${COMPILE_LIST}" "${ROOT_DIR}" OUTPUT_VARIABLE NOT_COMPILED_FILES)
if(NOT ${NOT_COMPILED_FILES} MATCHES NULL)
	message(WARNING "Following files are not compiled ${NOT_COMPILED_FILES}")
endif()
