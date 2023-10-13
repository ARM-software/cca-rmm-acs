#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(PAL_SRC
    ${ROOT_DIR}/plat/targets/${TARGET}/src/pal_driver.c
    ${ROOT_DIR}/plat/targets/${TARGET}/src/pal_interface.c
    ${ROOT_DIR}/plat/targets/${TARGET}/src/pal_secure_service.c
    ${ROOT_DIR}/plat/targets/${TARGET}/src/pal_cpu_info.c
    ${ROOT_DIR}/plat/targets/${TARGET}/src/pal_mmio.c
    ${ROOT_DIR}/plat/targets/${TARGET}/src/pal_irq.c
    ${ROOT_DIR}/plat/driver/src/pal_pl011_uart.c
    ${ROOT_DIR}/plat/driver/src/pal_sp805_watchdog.c
    ${ROOT_DIR}/plat/driver/src/pal_nvm.c
    ${ROOT_DIR}/plat/common/src/pal_smc.c
    ${ROOT_DIR}/plat/common/src/pal_libc.c
    ${ROOT_DIR}/plat/common/src/pal_syscall.S
    ${ROOT_DIR}/plat/common/src/pal_spinlock.S
    ${ROOT_DIR}/plat/driver/src/gic/pal_arm_gic_v3.c
    ${ROOT_DIR}/plat/driver/src/gic/pal_gic_common.c
    ${ROOT_DIR}/plat/driver/src/gic/pal_gic_v3.c
    ${ROOT_DIR}/plat/driver/src/gic/platform.S
    ${ROOT_DIR}/plat/driver/src/gic/asm_macros_common.S
)

#Create compile list files
list(APPEND COMPILE_LIST ${PAL_SRC})
set(COMPILE_LIST ${COMPILE_LIST} PARENT_SCOPE)

# Create PAL library
add_library(${PAL_LIB} STATIC ${PAL_SRC})

target_include_directories(${PAL_LIB} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${ROOT_DIR}/plat/common/inc/
    ${ROOT_DIR}/plat/targets/${TARGET}/inc/
    ${ROOT_DIR}/plat/driver/inc/
)

unset(PAL_SRC)
