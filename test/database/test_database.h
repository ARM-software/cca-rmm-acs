/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _TEST_DATABASE_H_
#define _TEST_DATABASE_H_

#include "val.h"
#include "val_framework.h"
#include "val_libc.h"
#include "val_mmu.h"

/* Test prototype */
typedef void (*test_fptr_t)(void);

/* Structure to hold all test info */
typedef struct {
    char                suite_name[PRINT_LIMIT];
    char                test_name[PRINT_LIMIT];
    test_fptr_t         host_fn; /* Host Test function */
    test_fptr_t         realm_fn; /* Realm Test function */
    test_fptr_t         secure_fn; /* Secure Test function */
} test_db_t;

#define DECLARE_TEST_FN(testname) \
    extern  void testname##_host(void);\
    extern  void testname##_realm(void);\
    extern  void testname##_secure(void);

#define HOST_TEST_ONLY(suitename, testname) \
    {"Suite="#suitename" : ", #testname, testname##_host, NULL, NULL}

#define REALM_TEST_ONLY(suitename, testname) \
    {" "#suitename, #testname, NULL, testname##_realm, NULL}

#define SECURE_TEST_ONLY(suitename, testname) \
    {" "#suitename, #testname, NULL, NULL, testname##_secure}

#define DUMMY_TEST(suitename, testname) \
    {" ", " ", NULL, NULL, NULL}

#define TEST_FUNC_DECLARATION
#include "test_list.h"

#endif /* _TEST_DATABASE_H_ */
