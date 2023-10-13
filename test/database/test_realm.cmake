#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# get testsuite directory list
set(TEST_DIR_PATH ${ROOT_DIR}/test)
if(SUITE STREQUAL "all")
    # Get all the test pool components
    _get_sub_dir_list(TEST_SUITE_LIST ${TEST_DIR_PATH})
else()
    set(TEST_SUITE_LIST ${SUITE})
endif()

set(TEST_INCLUDE ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND TEST_INCLUDE
    ${ROOT_DIR}/val/realm/inc/
    ${ROOT_DIR}/val/realm/src/
    ${ROOT_DIR}/val/common/inc/
    ${ROOT_DIR}/val/common/src/
    ${ROOT_DIR}/plat/common/inc/
    ${ROOT_DIR}/plat/common/src/
    ${ROOT_DIR}/plat/targets/${TARGET}/inc/
    ${ROOT_DIR}/plat/targets/${TARGET}/src/
    ${ROOT_DIR}/plat/driver/inc/
    ${ROOT_DIR}/plat/driver/src/
    ${ROOT_DIR}/test/database/
    ${ROOT_DIR}/test/common/
    ${ROOT_DIR}/val/common/xlat_tables_v2/include/
    ${RMM_ACS_QCBOR_INCLUDE_PATH}
)

foreach(SUITE ${TEST_SUITE_LIST})
    list(APPEND TEST_INCLUDE
    ${ROOT_DIR}/test/${SUITE}/common/
)
endforeach()

if(${TEST_COMBINE})

set(TEST_LIB ${EXE_NAME}_test_lib)

# Compile all .c/.S files from test directory
if(${SUITE} STREQUAL "all")
file(GLOB TEST_SRC
    "${ROOT_DIR}/test/*/*/*_realm.c"
    "${ROOT_DIR}/test/*/*/*_realm.S"
    "${ROOT_DIR}/test/*/common/*_realm.c"
    "${ROOT_DIR}/test/common/*_realm.c"
    "${RMM_ACS_TARGET_QCBOR}/src/qcbor_decode.c"
    "${RMM_ACS_TARGET_QCBOR}/src/UsefulBuf.c"
    "${RMM_ACS_TARGET_QCBOR}/src/ieee754.c"
)
else()
file(GLOB TEST_SRC
    "${ROOT_DIR}/test/${SUITE}/*/*_realm.c"
    "${ROOT_DIR}/test/${SUITE}/*/*_realm.S"
    "${ROOT_DIR}/test/${SUITE}/common/*_realm.c"
    "${ROOT_DIR}/test/common/*_realm.c"
    "${RMM_ACS_TARGET_QCBOR}/src/UsefulBuf.c"
    "${RMM_ACS_TARGET_QCBOR}/src/ieee754.c"
    "${RMM_ACS_TARGET_QCBOR}/src/qcbor_decode.c"
)
endif()

list(APPEND TEST_SRC
    ${ROOT_DIR}/test/database/test_database_realm.c
)

# Create TEST library
add_library(${TEST_LIB} STATIC ${TEST_SRC})

#Create compile list files
list(APPEND COMPILE_LIST ${TEST_SRC})
set(COMPILE_LIST ${COMPILE_LIST} PARENT_SCOPE)

target_include_directories(${TEST_LIB} PRIVATE ${TEST_INCLUDE})

create_executable(${EXE_NAME} ${BUILD}/output/ "")
unset(TEST_SRC)

else()

# test source directory
set(TEST_SOURCE_DIR ${ROOT_DIR}/test)
if(SUITE STREQUAL "all")
    # Get all the test pool components
    _get_sub_dir_list(SUITE_LIST ${TEST_SOURCE_DIR})
else()
    set(SUITE_LIST ${SUITE})
endif()


# Generate per test ELFs
foreach(SUITE ${SUITE_LIST})
    #message(STATUS "[ACS] : Compiling sources from ${SUITE} suite")
    # Get all the test folders from a given test component
    _get_sub_dir_list(TEST_LIST ${TEST_SOURCE_DIR}/${SUITE})
    foreach(TEST ${TEST_LIST})
        #message(STATUS "[ACS] : Compiling sources from ${TEST} Test")
        file(GLOB TEST_SRC "${TEST_SOURCE_DIR}/${SUITE}/${TEST}/${TEST}_realm.c")
        if(${SUITE} STREQUAL "attestation_measurement")
            list(APPEND TEST_SRC
                    "${RMM_ACS_TARGET_QCBOR}/src/qcbor_decode.c"
                    "${RMM_ACS_TARGET_QCBOR}/src/UsefulBuf.c"
                    "${RMM_ACS_TARGET_QCBOR}/src/ieee754.c"
                )
        endif()
        list(APPEND TEST_SRC ${TEST_SOURCE_DIR}/database/test_database_realm.c)
        file(GLOB TEST_COMMON_FILE ${TEST_SOURCE_DIR}/common/*_realm.c)
        foreach(COMMON_FILE1 ${TEST_COMMON_FILE})
            list(APPEND TEST_SRC ${COMMON_FILE1})
        endforeach()
        file(GLOB SUITE_COMMON_FILE ${TEST_SOURCE_DIR}/${SUITE}/common/*_realm.c)
        foreach(COMMON_FILE2 ${SUITE_COMMON_FILE})
            list(APPEND TEST_SRC ${COMMON_FILE2})
        endforeach()
        #Create compile list files
        list(APPEND COMPILE_LIST ${TEST_SRC})
        set(COMPILE_LIST ${COMPILE_LIST} PARENT_SCOPE)
        set(TEST_LIB ${EXE_NAME}_${TEST}_test_lib)
        add_definitions(-Dd_${TEST})
        add_library(${TEST_LIB} STATIC ${TEST_SRC})
        target_compile_definitions(${TEST_LIB} PRIVATE d_${TEST})
        target_include_directories(${TEST_LIB} PRIVATE ${TEST_INCLUDE})
        file(MAKE_DIRECTORY ${BUILD}/output/${SUITE}/${TEST})
        create_executable(${EXE_NAME} ${BUILD}/output/${SUITE}/${TEST} ${TEST})
        remove_definitions(-Dd_${TEST})
        unset(TEST_SRC)
    endforeach()
endforeach()
endif()
