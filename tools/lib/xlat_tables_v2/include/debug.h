/*
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <config.h>
#include <smc.h>

#define RSI_VERSION                           (0xC4000190)
#define RSI_REQ_VERSION                       ((1 << 16) | 0)

typedef enum {
    INFO    = 1,
    DBG     = 2,
    TEST    = 3,
    WARN    = 4,
    ERROR   = 5,
    ALWAYS  = 9
} print_verbosity_t;

#define print_func PLAT_PRINT_FUNC
extern void print_func(print_verbosity_t verbosity, const char *fmt, ...);

#ifdef PLAT_REALM_PRINT_FUNC
#define realm_print_func PLAT_REALM_PRINT_FUNC
extern void realm_print_func(print_verbosity_t verbosity, const char *fmt, ...);
#endif

#define LOG(print_verbosity, fmt, ...)                                   \
   do {                                                                  \
    if (print_verbosity >= VERBOSITY)                                    \
    {                                                                    \
        if (in_realm())                                                  \
        {                                                                \
            realm_print_func(print_verbosity, fmt, ##__VA_ARGS__);       \
            if (print_verbosity == ERROR)                                \
                realm_print_func(ERROR, "Check failed at %s , line:%d",  \
                                            __FILE__, __LINE__);         \
        }                                                                \
        else                                                             \
        {                                                                \
            print_func(print_verbosity, fmt, ##__VA_ARGS__);             \
            if (print_verbosity == ERROR)                                \
                print_func(ERROR, "Check failed at %s , line:%d",        \
                                            __FILE__, __LINE__);         \
        }                                                                \
    }                                                                    \
   } while (0);

static inline bool in_realm(void)
{
    struct smc_result smc_res;

	monitor_call_with_res(RSI_VERSION, RSI_REQ_VERSION,
			      0UL, 0UL, 0UL, 0UL, 0UL, &smc_res);

    if (smc_res.x[0] != SMC_NOT_SUPPORTED)
    {
        return true;
    }

    return false;
}

