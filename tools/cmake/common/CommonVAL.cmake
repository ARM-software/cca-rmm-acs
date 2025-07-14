#-------------------------------------------------------------------------------
# Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Set path for Common VAL directory to be used
set(COMMON_VAL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/val_common)

# Set header paths in a list for Common VAL to include while compilation
set(COMMON_VAL_HEADERS  ${ROOT_DIR}/plat/common/inc/
                        ${ROOT_DIR}/plat/targets/${TARGET}/inc/)
