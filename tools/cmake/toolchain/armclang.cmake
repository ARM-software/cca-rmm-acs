#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Workaound to get rid of compilation error
set(CMAKE_SYSTEM_PROCESSOR cortex-a53)

if(_TOOLCHAIN_CMAKE_LOADED)
  return()
endif()
set(_TOOLCHAIN_CMAKE_LOADED TRUE)

if(NOT DEFINED CMAKE_C_COMPILER)
    message(FATAL_ERROR "Please set CMAKE_C_COMPILER to hold the full path of \
your compiler executable")
endif(NOT DEFINED CMAKE_C_COMPILER)

get_filename_component(_CMAKE_C_TOOLCHAIN_LOCATION "${CMAKE_C_COMPILER}" PATH)
find_program(CMAKE_ARMCCLANG_AR     armar   HINTS "${_CMAKE_C_TOOLCHAIN_LOCATION}" )

set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}" CACHE FILEPATH "The ARMCLANG asm" FORCE)
set(CMAKE_AR "${CMAKE_ARMCCLANG_AR}" CACHE FILEPATH "The GNUARM archiver" FORCE)

# Set the compiler's target architecture profile based on ARM_ARCH_MINOR option
if(${ARM_ARCH_MINOR} STREQUAL 0)
    set(TARGET_SWITCH "-march=armv${ARM_ARCH_MAJOR}-a -target aarch64-arm-none-eabi")
else()
    set(TARGET_SWITCH "-march=armv${ARM_ARCH_MAJOR}.${ARM_ARCH_MINOR}-a -target aarch64-arm-none-eabi")
endif()

if(${ENABLE_PIE})
    set(COMPILE_PIE_SWITCH "-fpie")
    add_definitions(-DENABLE_PIE)
else()
    set(COMPILE_PIE_SWITCH "")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(COMPILE_DEBUG_OPTIONS "-g -Wa,--gdwarf-2")
else()
    set(COMPILE_DEBUG_OPTIONS "")
endif()


set(CMAKE_C_FLAGS          "${TARGET_SWITCH}  ${COMPILE_PIE_SWITCH} ${COMPILE_DEBUG_OPTIONS} -ffunction-sections -fdata-sections -mstrict-align -Os -ffreestanding -Wall -Werror -std=gnu99 -mgeneral-regs-only -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wextra -Wconversion -Wsign-conversion -Wcast-align -Wstrict-overflow")
set(CMAKE_ASM_FLAGS        "${TARGET_SWITCH} ${COMPILE_DEBUG_OPTIONS} -c -x assembler-with-cpp -Wall -Werror -ffunction-sections -fdata-sections -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wextra -Wconversion -Wsign-conversion -Wcast-align -Wstrict-overflow")
set(CMAKE_C_CREATE_STATIC_LIBRARY  "<CMAKE_AR> --create -cr <TARGET> <LINK_FLAGS> <OBJECTS>")
