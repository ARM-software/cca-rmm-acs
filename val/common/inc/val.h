/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_H_
#define _VAL_H_

#include "pal_interfaces.h"
#include "val_def.h"
#include "val_framework.h"
extern uint64_t security_state;

#define MIN(a, b)        ((a) < (b) ? (a) : (b))
#define MAX(a, b)        ((a) > (b) ? (a) : (b))

/* Various test status codes, Max value = 0xff
 * 0,1 - For all components
 * 1 - 100 Host Test use
 * 101 - 200 Realm Test use
 * 201 - 240 Secure Test Use
 * 241 - 255 VAL use
 */

#define  VAL_SUCCESS            0
#define  VAL_ERROR              1
#define  VAL_ERROR_POINT(n)     n
#define  VAL_TEST_INIT_FAILED   241
#define  VAL_STATUS_INVALID     242
#define  VAL_SKIP_CHECK         243
#define  VAL_SIM_ERROR          244
#define  VAL_STATUS_ERROR_MAX   255

#define VAL_INVALID_TEST_NUM 0xFFFFFFFF

#define  VAL_TEST_PREP_SEQ_FAILED            0xFFFFFFFFFFFFFFFF

#define VAL_EXTRACT_BITS(data, start, end) ((data >> start) & ((1ul << (end - start + 1)) - 1))
#define VAL_BIT_MASK(len) ((1 << len) - 1)
/* Set the value in given position */
#define VAL_SET_BITS(data, pos, len, val) (((uint32_t)(~(uint32_t)0 & \
                                        ~(uint32_t)(VAL_BIT_MASK(len) << pos)) & data) \
                                         | (val << pos))

#define VAL_BIT_MASK_ULL(_msb, _lsb) \
    ((~ULL(0) >> (63UL - (_msb))) & (~ULL(0) << (_lsb)))

/* SHARED REGION Index size */
#define BLOCK_SIZE         8
#define OFFSET(index)    (index * BLOCK_SIZE)

/* Print char limit for val_printf caller */
#define PRINT_LIMIT       80

/* NVM Indext size */
#define VAL_NVM_BLOCK_SIZE         4
#define VAL_NVM_OFFSET(nvm_idx)    (nvm_idx * VAL_NVM_BLOCK_SIZE)

/* Shared region layout
 * 0x0  - 0x7    TEST_NUM
 * 0x8  - 0xF    TEST_STATUS
 * 0x10 - 0x63   REALM_PRINTF_MSG - 90 Chars
 * 0x68 - 0x6F   REALM_PRINTF_DATA1
 * 0x70 - 0x77   REALM_PRINTF_DATA2
 * 0x78 - 0x9F   TEST_NAME_STRING - 40 Chars
 * 0xA0 - 0xFFF  VAL_RESERVED
 * 0x1000 - SHARED_END - Test usecase
 * */

typedef enum {
    VAL_CURR_TEST_NUM     = 0,
    VAL_CURR_TEST_STATUS  = 1,
    VAL_PRINTF_MSG        = 2,
    VAL_PRINTF_DATA1      = 13,
    VAL_PRINTF_DATA2      = 14,
    VAL_CURR_TEST_NAME    = 15,
    VAL_TEST_USE1         = 512,
    VAL_TEST_USE2         = 520,
    VAL_TEST_USE3         = 528,
    VAL_TEST_USE4         = 536,
    VAL_TEST_USE5         = 544,
    VAL_PRINT_OFFSET      = 552,
} val_shared_region_map_index_te;

#define TEST_NUM_OFFSET OFFSET(VAL_CURR_TEST_NUM)
#define END_NUM_OFFSET OFFSET(VAL_END_TEST_NUM)
#define TEST_STATUS_OFFSET OFFSET(VAL_CURR_TEST_STATUS)
#define REALM_PRINTF_MSG_OFFSET OFFSET(VAL_PRINTF_MSG)
#define REALM_PRINTF_DATA1_OFFSET OFFSET(VAL_PRINTF_DATA1)
#define REALM_PRINTF_DATA2_OFFSET OFFSET(VAL_PRINTF_DATA2)
#define TEST_NAME_OFFSET OFFSET(VAL_CURR_TEST_NAME)
#define TEST_USE_OFFSET1 OFFSET(VAL_TEST_USE1)
#define TEST_USE_OFFSET2 OFFSET(VAL_TEST_USE2)
#define TEST_USE_OFFSET3 OFFSET(VAL_TEST_USE3)
#define TEST_USE_OFFSET4 OFFSET(VAL_TEST_USE4)
#define TEST_USE_OFFSET5 OFFSET(VAL_TEST_USE5)
#define PRINT_OFFSET OFFSET(VAL_PRINT_OFFSET)

/* Struture to capture test state */
typedef struct {
    uint16_t reserved;
    uint8_t  state;
    uint8_t  status_code;
} val_test_status_buffer_ts;

/* Verbosity enums, Lower the value, higher the verbosity */
typedef enum {
    INFO    = 1,
    DBG     = 2,
    TEST    = 3,
    WARN    = 4,
    ERROR   = 5,
    ALWAYS  = 9
} val_print_verbosity_te;

typedef struct {
    uint32_t total_pass;
    uint32_t total_fail;
    uint32_t total_skip;
    uint32_t total_error;
} val_regre_report_ts;

typedef struct {
    uint32_t suite_num;
    uint32_t test_num;
    uint32_t end_test_num;
    uint32_t test_progress;
} val_test_info_ts;

typedef enum {
    NVM_PLATFORM_RESERVE_INDEX         = 0x0,
    NVM_CUR_SUITE_NUM_INDEX            = 0x1,
    NVM_CUR_TEST_NUM_INDEX             = 0x2,
    NVM_END_TEST_NUM_INDEX             = 0x3,
    NVM_TEST_PROGRESS_INDEX            = 0x4,
    NVM_TOTAL_PASS_INDEX               = 0x5,
    NVM_TOTAL_FAIL_INDEX               = 0x6,
    NVM_TOTAL_SKIP_INDEX               = 0x7,
    NVM_TOTAL_ERROR_INDEX              = 0x8,
} val_nvm_map_index_te;

/* Test state macros */
#define TEST_START                 0x01
#define TEST_PASS                  0x02
#define TEST_FAIL                  0x03
#define TEST_SKIP                  0x04
#define TEST_ERROR                 0x05
#define TEST_END                   0x06
#define TEST_REBOOTING             0x07

#define TEST_STATE_SHIFT           8
#define TEST_STATUS_CODE_SHIFT     0

#define TEST_STATE_MASK            0xFF
#define TEST_STATUS_CODE_MASK      0xFF

#define RESULT_START(status)     (((TEST_START) << TEST_STATE_SHIFT) |\
                                    ((status) << TEST_STATUS_CODE_SHIFT))
#define RESULT_PASS(status)     (((TEST_PASS) << TEST_STATE_SHIFT) |\
                                    ((status) << TEST_STATUS_CODE_SHIFT))
#define RESULT_FAIL(status)     (((TEST_FAIL) << TEST_STATE_SHIFT) |\
                                    ((status) << TEST_STATUS_CODE_SHIFT))
#define RESULT_SKIP(status)     (((TEST_SKIP) << TEST_STATE_SHIFT) |\
                                    ((status) << TEST_STATUS_CODE_SHIFT))
#define RESULT_ERROR(status)     (((TEST_ERROR) << TEST_STATE_SHIFT) |\
                                    ((status) << TEST_STATUS_CODE_SHIFT))

#define IS_TEST_FAIL(status)    (((status >> TEST_STATE_SHIFT) &\
                                    TEST_STATE_MASK) == TEST_FAIL)
#define IS_STATUS_FAIL(status)  ((status & TEST_STATUS_CODE_MASK) ? 1 : 0)

/* Macro to print the host and secure message and control the verbosity */
#define LOG(print_verbosity, x, y, z)               \
   do {                                             \
    if (print_verbosity >= VERBOSITY)               \
        val_common_printf(x, y, z);                        \
    if (print_verbosity == ERROR)                   \
    {                                               \
        val_common_printf("\t(Check failed at:", 0, 0);    \
        val_common_printf(__FILE__, 0, 0);                 \
        val_common_printf(" ,line:%d)\n", __LINE__, 0);    \
    }                                               \
   } while (0);

/* Terminate simulation for unexpected events */
#define VAL_PANIC(x)                               \
   do {                                             \
        LOG(ERROR, x, 0, 0);                        \
        pal_terminate_simulation();                 \
   } while (0);

/* Assert macros */
#define TEST_ASSERT_EQUAL(arg1, arg2)                                       \
    do {                                                                    \
        if ((arg1) != arg2)                                                 \
        {                                                                   \
            LOG(ERROR, "\tActual: %x, Expected: %x\n", arg1, arg2);         \
            return 1;                                                       \
        }                                                                   \
    } while (0);

#endif /* _VAL_H_ */
