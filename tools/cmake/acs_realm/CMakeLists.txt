#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

project(acs_realm LANGUAGES C ASM)

set(EXE_NAME "${PROJECT_NAME}")

set(VAL_LIB ${EXE_NAME}_val_lib)
set(PAL_LIB ${EXE_NAME}_pal_lib)

add_definitions(-DACS_REALM_BUILD)
include(${ROOT_DIR}/plat/targets/${TARGET}/pal.cmake)
include(${ROOT_DIR}/val/realm/val_realm.cmake)
include(${ROOT_DIR}/test/database/test_realm.cmake)
