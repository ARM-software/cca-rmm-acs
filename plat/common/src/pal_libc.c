/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_interfaces.h"

void *memcpy(void *dst, const void *src, size_t len);
void *memset(void *dst, int val, size_t count);
int memcmp(void *s1, void *s2, size_t len);

int pal_memcmp(void *src, void *dest, size_t len)
{
  return memcmp(src, dest, len);
}

void *pal_memcpy(void *dst, const void *src, size_t len)
{
  return memcpy(dst, src, len);
}

void *pal_memset(void *dst, int val, size_t count)
{
  memset(dst, val, count);
  return dst;
}

/* Libc functions definition */

void *memcpy(void *dst, const void *src, size_t len)
{
    const char *s = src;
    char *d = dst;

    while (len--)
    {
        *d++ = *s++;
    }

    return dst;
}

void *memset(void *dst, int val, size_t count)
{
    unsigned char *ptr = dst;

    while (count--)
    {
        *ptr++ = (unsigned char)val;
    }

    return dst;
}

int memcmp(void *s1, void *s2, size_t len)
{
    unsigned char *s = s1;
    unsigned char *d = s2;
    unsigned char sc;
    unsigned char dc;

    while (len--)
    {
        sc = *s++;
        dc = *d++;
        if (sc - dc)
            return (sc - dc);
    }

    return 0;
}
