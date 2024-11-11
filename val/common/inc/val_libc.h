/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_LIBC_H_
#define _VAL_LIBC_H_

#include "val.h"

int val_memcmp(void *src, void *dest, size_t len);
void *val_memcpy(void *dst, const void *src, size_t len);
void val_memset(void *dst, int val, size_t count);
char *val_strcat(char *str1, char *str2, size_t output_buff_size);
int val_strcmp(char *str1, char *str2);
size_t val_strlen(char *str);
void val_assert(const char *e, uint64_t line, const char *file);

#endif /* _VAL_LIBC_H_ */
