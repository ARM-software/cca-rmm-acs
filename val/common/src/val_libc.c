/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_libc.h"

/**
  @brief  Compare the two input buffer content
  @param  src   - Source buffer to be compared
  @param  dest  - Destination buffer to be compared
  @return Zero if buffer content are equal, else non-zero
**/
int val_memcmp(void *src, void *dest, size_t len)
{
  return pal_memcmp(src, dest, len);
}

/**
  @brief  Copy src buffer content into dst
  @param  dst  - Pointer to the destination buffer
  @param  src  - Pointer to the source buffer
  @param  len  - Number of bytes in buffer to copy
  @return dst
**/
void *val_memcpy(void *dst, const void *src, size_t len)
{
  return pal_memcpy(dst, src, len);
}

/**
  @brief  Fill a buffer with a known specified input value
  @param  - dst   : Pointer to the buffer to fill
          - value : Value to fill buffer with
          - count : Number of bytes in buffer to fill
  @return None
**/
void val_memset(void *dst, int val, size_t count)
{
  pal_memset(dst, val, count);
}

/**
  @brief  Appends the string pointed to by str2 to the
          end of the string pointed to by str1
  @param  str1  - Pointer of destination string
  @param  str2  - Pointer of string to be appended
  @param  output_buff_size - Size of str1 string
  @return Pointer of destination string
**/

char *val_strcat(char *str1, char *str2, size_t output_buff_size)
{
  size_t length = 0, i;

  while (str1[length] != '\0')
  {
    ++length;
  }

  /* Concatenate str1 to str2 */
  for (i = 0; str2[i] != '\0'; ++i, ++length)
  {
     if (length < output_buff_size)
     {
         str1[length] = str2[i];
     } else
     {
         VAL_PANIC("\tError: Buffer overflow detected\n");
     }
  }

  str1[length] = '\0';

  return str1;
}

/**
  @brief  Compare two strings
  @param  str1  - Pointer of first string
  @param  str2  - Pointer of second string
  @return Zero if strings are equal, else non-zero
**/
int val_strcmp(char *str1, char *str2)
{
  int ret = 0;

  /* Compare str1 to str2 */
  while (*str1)
    {
        if (*str1 != *str2) {
            ret = 1;
            break;
        }

        str1++;
        str2++;
    }

    if (ret == 0)
    {
      return 0;
    }
    else
    {
      return 1;
    }

}

/**
  @brief  Count string length
  @param  str  - Pointer of string
  @return Returns string length
**/
size_t val_strlen(char *str)
{
  return pal_strlen(str);
}