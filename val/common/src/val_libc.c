/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
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

/**
 * @brief VAL abstraction to handle assertion failures.
 *
 * This function forwards the assertions to Platform Abstraction Layer.
 *
 * @param e The  assertion condition.
 * @param line The line number where the assertion is made.
 * @param file The name of the file where the assertion is made.
 */
void val_assert(const char *e, uint64_t line, const char *file)
{
    pal_assert(e, line, file);
}

/**
 * @brief Prints a string to a buffer while tracking the number of characters printed.
 *
 * @param s Pointer to the buffer where the string is stored.
 * @param n Maximum number of characters that can be written to the buffer.
 * @param chars_printed Pointer to a counter tracking the number of written characters.
 * @param str The input string to be printed.
 */
static void string_print(char **s, size_t n, size_t *chars_printed,
             const char *str)
{
    while (*str != '\0') {
        if (*chars_printed < n) {
            *(*s) = *str;
            (*s)++;
        }

        (*chars_printed)++;
        str++;
    }
}

/**
 * @brief Converts an unsigned integer to a string and appends it to the buffer.
 *
 * @param s Pointer to the buffer where the output string is stored.
 * @param n Maximum number of characters that can be written to the buffer.
 * @param count Pointer to a counter tracking the number of written characters.
 * @param unum The unsigned integer to be converted.
 * @param radix The numerical base (e.g., 10 for decimal, 16 for hexadecimal).
 * @param padc The padding character to be used if padding is required.
 * @param padn The number of padding characters to be inserted.
 */
static void unsigned_num_print(char **s, size_t n, size_t *count,
                  unsigned long long int unum, unsigned int radix,
                  char padc, int padn)
{
    /* Just need enough space to store 64 bit decimal integer */
    char num_buf[20];
    int i = 0;
    int width;
    unsigned int rem;

    do {
        rem = (unsigned int)unum % radix;
        if (rem < 0xa)
            num_buf[i] = '0' + (char)rem;
        else
            num_buf[i] = 'a' + (char)(rem - 0xa);
        i++;
        unum /= radix;
    } while (unum > 0U);

    width = i;

    if (padn > 0) {
        while (width < padn) {
            if (*count < n) {
                *(*s) = padc;
                (*s)++;
            }
            (*count)++;
            padn--;
        }
    }

    while (--i >= 0) {
        if (*count < n) {
            *(*s) = num_buf[i];
            (*s)++;
        }
        (*count)++;
    }

    if (padn < 0) {
        while (width < -padn) {
            if (*count < n) {
                *(*s) = padc;
                (*s)++;
            }
            (*count)++;
            padn++;
        }
    }
}

/**
 * @brief A simplified version of vsnprintf that formats and writes data into a string.
 *
 * @param s    Pointer to the output buffer where the formatted string is stored.
 * @param n    Maximum number of characters that can be written to the buffer, including
 *             null terminator.
 * @param fmt  The format string specifying how subsequent arguments are formatted.
 * @param args A variable argument list containing the values to be formatted.
 *
 * @return The number of characters written (excluding the null terminator), or -1 if an
 *         error occurs.
 */
int val_vsnprintf(char *s, size_t n, const char *fmt, va_list args)
{
    int l_count;
    int left;
    char *str;
    int num;
    unsigned long long int unum;
    char padc; /* Padding character */
    int padn; /* Number of characters to pad */
    size_t count = 0U;

    if (n == 0U) {
        /* There isn't space for anything. */
    } else if (n == 1U) {
        /* Buffer is too small to actually write anything else. */
        *s = '\0';
        n = 0U;
    } else {
        /* Reserve space for the terminator character. */
        n--;
    }

    while (*fmt != '\0') {
        l_count = 0;
        left = 0;
        padc = '\0';
        padn = 0;

        if (*fmt == '%') {
            fmt++;
            /* Check the format specifier. */
loop:
            switch (*fmt) {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                padc = ' ';
                for (padn = 0; *fmt >= '0' && *fmt <= '9'; fmt++)
                    padn = (padn * 10) + (*fmt - '0');
                if (left)
                    padn = -padn;
                goto loop;
            case '-':
                left = 1;
                fmt++;
                goto loop;
            case 'i':
            case 'd':
                num = get_num_va_args(args, l_count);

                if (num < 0) {
                    if (count < n) {
                        *s = '-';
                        s++;
                    }
                    count++;

                    unum = (unsigned int)-num;
                } else {
                    unum = (unsigned int)num;
                }

                unsigned_num_print(&s, n, &count, unum, 10,
                           padc, padn);
                break;
            case 'l':
                l_count++;
                fmt++;
                goto loop;
            case 's':
                str = va_arg(args, char *);
                string_print(&s, n, &count, str);
                break;
            case 'u':
                unum = get_unum_va_args(args, l_count);
                unsigned_num_print(&s, n, &count, unum, 10,
                           padc, padn);
                break;
            case 'x':
                unum = get_unum_va_args(args, l_count);
                unsigned_num_print(&s, n, &count, unum, 16,
                           padc, padn);
                break;
            case '0':
                padc = '0';
                padn = 0;
                fmt++;

                for (;;) {
                    char ch = *fmt;
                    if ((ch < '0') || (ch > '9')) {
                        goto loop;
                    }
                    padn = (padn * 10) + (ch - '0');
                    fmt++;
                }
            default:
                /*
                 * Exit on any other format specifier and abort
                 * when in debug mode.
                 */
                return -1;
            }
            fmt++;
            continue;
        }

        if (count < n) {
            *s = *fmt;
            s++;
        }

        fmt++;
        count++;
    }

    if (n > 0U)
        *s = '\0';

    return (int)count;
}

