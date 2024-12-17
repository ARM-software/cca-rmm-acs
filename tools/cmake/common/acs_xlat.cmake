#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(XLAT-LIB acs-xlat-lib)

set(XLAT_SRC_DIR ${ROOT_DIR}/tools/lib/xlat_tables_v2)
set(XLAT_BUILD_DIR ${ROOT_DIR}/build/xlat_tables_v2)

add_subdirectory(${XLAT_SRC_DIR} ${XLAT_BUILD_DIR})

target_compile_definitions(${XLAT-LIB} PUBLIC XLAT_CONFIG_FILE="${ROOT_DIR}/tools/configs/xlat/xlat_config.h")
