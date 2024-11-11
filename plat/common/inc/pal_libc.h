/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal.h"

#ifndef _PAL_LIBC_H_
#define _PAL_LIBC_H_

#define assert(e)   ((e) ? (void)0 : pal_assert(#e, __LINE__, __FILE__))

#define CASSERT(cond, msg)    \
    typedef char msg[(cond) ? 1 : -1] __unused

/**
 * @brief        - Compare the two input buffer content
 * @param        - src: Source buffer to be compared
 * @dest         - dest: Destination buffer to be compared

 * @return       - Zero if buffer content are equal, else non-zero
**/
int pal_memcmp(void *src, void *dest, size_t len);

/**
 * @brief        - Fill a buffer with a known specified input value
 * @param        - dst: Pointer to the buffer to fill
 * @param        - value: Value to fill buffer with
 * @param        - count: Number of bytes in buffer to fill
 * @return       - dst
**/
void *pal_memset(void *dst, int val, size_t count);

/**
 * @brief        - Copy src buffer content into dst
 * @param        - dst: Pointer to the destination buffer
 * @param        - src: Pointer to the source buffer
 * @param        - len: Number of bytes in buffer to copy
 * @return       - dst
**/
void *pal_memcpy(void *dst, const void *src, size_t len);

size_t pal_strlen(char *str);

void pal_assert(const char *e, uint64_t line, const char *file);
#endif
