#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

if(_TOOLCHAIN_CMAKE_LOADED)
  return()
endif()
set(_TOOLCHAIN_CMAKE_LOADED TRUE)

get_filename_component(_CMAKE_C_TOOLCHAIN_LOCATION "${CMAKE_C_COMPILER}" PATH)

set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}" CACHE FILEPATH "The GNUARM asm" FORCE)
set(CMAKE_AR "${CROSS_COMPILE}ar" CACHE FILEPATH "The GNUARM archiver" FORCE)

# Set the compiler's target architecture profile based on ARM_ARCH_MINOR option
if(${ARM_ARCH_MINOR} STREQUAL 0)
    set(TARGET_SWITCH "-march=armv${ARM_ARCH_MAJOR}-a")
else()
    set(TARGET_SWITCH "-march=armv${ARM_ARCH_MAJOR}.${ARM_ARCH_MINOR}-a")
endif()

if(${ENABLE_PIE})
    set(COMPILE_PIE_SWITCH "-fpie")
    add_definitions(-DENABLE_PIE)
else()
    set(COMPILE_PIE_SWITCH "")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(C_COMPILE_DEBUG_OPTIONS "-g -Wa,-gdwarf-4")
    set(ASM_COMPILE_DEBUG_OPTIONS "-g -Wa,--gdwarf-4")
else()
    set(COMPILE_DEBUG_OPTIONS "")
endif()


set(CMAKE_C_FLAGS          "${TARGET_SWITCH}  ${COMPILE_PIE_SWITCH} ${C_COMPILE_DEBUG_OPTIONS} -ffunction-sections -fdata-sections -mstrict-align -Os -ffreestanding -Wall -Werror -std=gnu99 -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wextra -Wconversion -Wsign-conversion -Wcast-align -Wstrict-overflow -DCMAKE_GNUARM_COMPILE -Wno-packed-bitfield-compat")
set(CMAKE_ASM_FLAGS        "${TARGET_SWITCH} ${ASM_COMPILE_DEBUG_OPTIONS} -c -x assembler-with-cpp -Wall -Werror -ffunction-sections -fdata-sections -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wextra -Wconversion -Wsign-conversion -Wcast-align -Wstrict-overflow -DCMAKE_GNUARM_COMPILE")
