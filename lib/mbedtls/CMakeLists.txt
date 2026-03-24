#-------------------------------------------------------------------------------
# Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

project(acs_mbedtls LANGUAGES C)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/external/mbedtls/build/)
set(MBEDTLS_SRC_DIR "${RMM_ACS_TARGET_MBEDTLS}")
set(MBEDTLS_BIN_DIR "${CMAKE_BINARY_DIR}/external/mbedtls/build")

# Starting Mbed TLS 3.x release, mbedtls/library/CMakeLists.txt has dependency on
# link_to_source() function that is in mbedtls/CMakeLists.txt, so define a dummy
# link_to_source here.
function(link_to_source base_name)
endfunction(link_to_source)

# Ensure that the CFlags modifications are local in scope and only apply to
# MbedTLS build

set(BACKUP_C_FLAGS "${CMAKE_C_FLAGS}")

# Enable using floating point registers for mbed TLS
string(REPLACE "-Wsign-conversion" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
string(REPLACE "-Wconversion" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})

add_subdirectory("${MBEDTLS_SRC_DIR}/library" "${MBEDTLS_BIN_DIR}/library" EXCLUDE_FROM_ALL)

set(CMAKE_C_FLAGS "${BACKUP_C_FLAGS}")

add_library(rmm-acs-mbedtls INTERFACE)
target_include_directories(rmm-acs-mbedtls INTERFACE
    "${ROOT_DIR}/lib/configs/mbedtls"
    "${MBEDTLS_SRC_DIR}/include")

target_compile_definitions(rmm-acs-mbedtls INTERFACE
    "-DMBEDTLS_CONFIG_FILE=<rmm_mbedtls_config.h>")

# Include targets mbedtls, mbedcrypto, mbedx509
set(MBEDTLS_LIBS mbedtls)
list(APPEND MBEDTLS_LIBS mbedx509)
list(APPEND MBEDTLS_LIBS mbedcrypto)

foreach(MBEDTLS_LIB IN ITEMS ${MBEDTLS_LIBS})
    target_include_directories(${MBEDTLS_LIB} PUBLIC
        "${ROOT_DIR}/lib/configs/mbedtls"
        "${MBEDTLS_SRC_DIR}/include")
    target_compile_definitions(${MBEDTLS_LIB} PUBLIC
        "-DMBEDTLS_CONFIG_FILE=<rmm_mbedtls_config.h>")
    target_link_libraries(rmm-acs-mbedtls INTERFACE ${MBEDTLS_LIB})
endforeach()
