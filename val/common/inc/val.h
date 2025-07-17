/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
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

#define SEC_STATE_NS      1
#define SEC_STATE_REALM   2
#define SEC_STATE_SECURE  3

#define MIN(a, b)        ((a) < (b) ? (a) : (b))
#define MAX(a, b)        ((a) > (b) ? (a) : (b))

#define  VAL_ERROR                 1
#define  VAL_TEST_PREP_SEQ_FAILED  0xFFFFFFFFFFFFFFFF
#define  VAL_RESERVED              -1ULL

#define VAL_EXTRACT_BITS(data, start, end) ((data >> start) & ((1ul << (end - start + 1)) - 1))

#define VAL_BIT_MASK_ULL(_msb, _lsb) \
    ((~ULL(0) >> (63UL - (_msb))) & (~ULL(0) << (_lsb)))

/* SHARED REGION Index size */
#define BLOCK_SIZE         8
#define OFFSET(index)    (index * BLOCK_SIZE)

/* Print char limit for val_printf caller */
#define PRINT_LIMIT       80

/* Shared region layout
 * 0x0  - 0x7    TEST_STATUS
 * 0x8  - 0xF    TEST_NUM
 * 0x10 - 0x63   REALM_PRINTF_MSG - 90 Chars
 * 0x68 - 0x6F   REALM_PRINTF_DATA1
 * 0x70 - 0x77   REALM_PRINTF_DATA2
 * 0x78 - 0x9F   TEST_NAME_STRING - 40 Chars
 * 0xA0 - 0xFFF  VAL_RESERVED
 * 0x1000 - SHARED_END - Test usecase
 * */

typedef enum {
    VAL_CURR_TEST_STATUS  = 0,
    VAL_CURR_TEST_NUM     = 1,
    VAL_PRINTF_MSG        = 2,
    VAL_PRINTF_VERBOSITY  = 12,
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
#define REALM_PRINTF_MSG_OFFSET OFFSET(VAL_PRINTF_MSG)
#define REALM_PRINTF_VERBOSITY_OFFSET OFFSET(VAL_PRINTF_VERBOSITY)
#define REALM_PRINTF_DATA1_OFFSET OFFSET(VAL_PRINTF_DATA1)
#define REALM_PRINTF_DATA2_OFFSET OFFSET(VAL_PRINTF_DATA2)
#define TEST_NAME_OFFSET OFFSET(VAL_CURR_TEST_NAME)
#define TEST_USE_OFFSET1 OFFSET(VAL_TEST_USE1)
#define TEST_USE_OFFSET2 OFFSET(VAL_TEST_USE2)
#define TEST_USE_OFFSET3 OFFSET(VAL_TEST_USE3)
#define TEST_USE_OFFSET4 OFFSET(VAL_TEST_USE4)
#define TEST_USE_OFFSET5 OFFSET(VAL_TEST_USE5)
#define PRINT_OFFSET OFFSET(VAL_PRINT_OFFSET)


/* Macro to print the host and secure message and control the verbosity */
#define LOG(print_verbosity, fmt, ...)                                   \
   do {                                                                  \
    if (print_verbosity >= VERBOSITY)                                    \
    {                                                                    \
        val_print_secuity_state();                                       \
        if (security_state == SEC_STATE_REALM)                           \
        {                                                                \
            val_realm_printf(print_verbosity, fmt, ##__VA_ARGS__);       \
            if (print_verbosity == ERROR)                                \
                val_realm_printf(ERROR, "Check failed at %s , line:%d",  \
                                            __FILE__, __LINE__);         \
        }                                                                \
        else                                                             \
        {                                                                \
            val_printf(print_verbosity, fmt, ##__VA_ARGS__);             \
            if (print_verbosity == ERROR)                                \
                val_printf(ERROR, "Check failed at %s , line:%d",        \
                                            __FILE__, __LINE__);         \
        }                                                                \
    }                                                                    \
   } while (0);

/* Terminate simulation for unexpected events */
#define VAL_PANIC(x)                               \
   do {                                             \
        LOG(ERROR, x);                        \
        pal_terminate_simulation();                 \
   } while (0);

/* Assert macros */
#define TEST_ASSERT_EQUAL(arg1, arg2)                                       \
    do {                                                                    \
        if ((arg1) != arg2)                                                 \
        {                                                                   \
            LOG(ERROR, "Actual: %x, Expected: %x\n", arg1, arg2);         \
            return 1;                                                       \
        }                                                                   \
    } while (0);

#endif /* _VAL_H_ */
