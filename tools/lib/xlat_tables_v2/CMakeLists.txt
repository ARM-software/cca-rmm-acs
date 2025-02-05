#-------------------------------------------------------------------------------
# Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

project(xlat-lib LANGUAGES C ASM)

file(GLOB XLAT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.S"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.S"
)

add_library(${XLAT-LIB} STATIC ${XLAT_SRC})

target_include_directories(${XLAT-LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
