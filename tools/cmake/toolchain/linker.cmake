#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

if(_LINKER_CMAKE_LOADED)
  return()
endif()
set(_LINKER_CMAKE_LOADED TRUE)

set(GNUARM_LINKER "${CROSS_COMPILE}ld" CACHE FILEPATH "The GNUARM linker" FORCE)
set(GNUARM_OBJCOPY "${CROSS_COMPILE}objcopy" CACHE FILEPATH "The GNUARM objcopy" FORCE)
set(GNUARM_OBJDUMP "${CROSS_COMPILE}objdump" CACHE FILEPATH "The GNUARM objdump" FORCE)

if(${ENABLE_PIE})
    set(LINKER_PIE_SWITCH "-pie --no-dynamic-linker")
else()
    set(LINKER_PIE_SWITCH "")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(LINKER_DEBUG_OPTIONS "-g")
else()
    set(LINKER_DEBUG_OPTIONS "")
endif()

set(GNUARM_LINKER_FLAGS "--fatal-warnings  ${LINKER_PIE_SWITCH} ${LINKER_DEBUG_OPTIONS} -O1 --gc-sections --build-id=none")
set(GNUARM_OBJDUMP_FLAGS    "-dSx")
set(GNUARM_OBJCOPY_FLAGS    "-Obinary")

function (create_executable EXE_NAME OUTPUT_DIR TEST)
    set(SCATTER_INPUT_FILE "${ROOT_DIR}/tools/cmake/${EXE_NAME}/image.ld.S")
    set(SCATTER_OUTPUT_FILE "${OUTPUT_DIR}/${EXE_NAME}_image.ld")

    # Preprocess the scatter file for image layout symbols
    add_custom_command(OUTPUT CPP-LD--${EXE_NAME}${TEST}
                    COMMAND ${CROSS_COMPILE}gcc -E -P -I${ROOT_DIR}/val/common/inc/ -I${ROOT_DIR}/plat/targets/${TARGET}/inc/ ${SCATTER_INPUT_FILE} -o ${SCATTER_OUTPUT_FILE} -DCMAKE_BUILD={CMAKE_BUILD}
                    DEPENDS ${VAL_LIB} ${PAL_LIB} ${TEST_LIB})
    add_custom_target(CPP-LD-${EXE_NAME}${TEST} ALL DEPENDS CPP-LD--${EXE_NAME}${TEST})

    # Link the objects
    add_custom_command(OUTPUT ${EXE_NAME}${TEST}.elf
                    COMMAND ${GNUARM_LINKER} ${CMAKE_LINKER_FLAGS} -T ${SCATTER_OUTPUT_FILE} -o ${OUTPUT_DIR}/${EXE_NAME}.elf ${VAL_LIB}.a ${PAL_LIB}.a ${TEST_LIB}.a ${VAL_LIB}.a ${PAL_LIB}.a ${PAL_OBJ_LIST}
                    DEPENDS CPP-LD-${EXE_NAME}${TEST})
    add_custom_target(${EXE_NAME}${TEST}_elf ALL DEPENDS ${EXE_NAME}${TEST}.elf)

    # Create the dump info
    add_custom_command(OUTPUT ${EXE_NAME}${TEST}.dump
                    COMMAND ${GNUARM_OBJDUMP} ${GNUARM_OBJDUMP_FLAGS} ${OUTPUT_DIR}/${EXE_NAME}.elf > ${OUTPUT_DIR}/${EXE_NAME}.dump
                    DEPENDS ${EXE_NAME}${TEST}_elf)
    add_custom_target(${EXE_NAME}${TEST}_dump ALL DEPENDS ${EXE_NAME}${TEST}.dump)

    # Create the binary
    add_custom_command(OUTPUT ${EXE_NAME}${TEST}.bin
                    COMMAND ${GNUARM_OBJCOPY} ${GNUARM_OBJCOPY_FLAGS} ${OUTPUT_DIR}/${EXE_NAME}.elf ${OUTPUT_DIR}/${EXE_NAME}.bin
                    DEPENDS ${EXE_NAME}${TEST}_dump)
    add_custom_target(${EXE_NAME}${TEST}_bin ALL DEPENDS ${EXE_NAME}${TEST}.bin)

if(${EXE_NAME} STREQUAL "acs_realm")
    # Combine acs_host.bin and acs_realm.bin into acs_non_secure.bin
    add_custom_command(OUTPUT acs_non_secure${TEST}.bin
            COMMAND ${SREC_CAT} ${OUTPUT_DIR}/acs_host.bin -Binary ${OUTPUT_DIR}/acs_realm.bin -Binary -offset 0x100000 -o ${OUTPUT_DIR}/acs_non_secure.bin -Binary
            DEPENDS acs_host${TEST}_bin acs_realm${TEST}_bin)
    add_custom_target(ns_bin_file${TEST} ALL DEPENDS acs_non_secure${TEST}.bin)
endif()

endfunction()
