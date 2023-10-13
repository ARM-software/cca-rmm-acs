/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _PAL_CDEFS_H
#define _PAL_CDEFS_H

#define __dead2        __attribute__((__noreturn__))
#define __deprecated    __attribute__((__deprecated__))
#define __packed    __attribute__((__packed__))
#define __used        __attribute__((__used__))
#define __unused    __attribute__((__unused__))
#define __aligned(x)    __attribute__((__aligned__(x)))
#define __section(x)    __attribute__((__section__(x)))

#define __STRING(x)    #x
#define __XSTRING(x)    __STRING(x)

/*
 * For those constants to be shared between C and other sources, apply a 'U',
 * 'UL', 'ULL', 'L' or 'LL' suffix to the argument only in C, to avoid
 * undefined or unintended behaviour.
 *
 * The GNU assembler and linker do not support these suffixes (it causes the
 * build process to fail) therefore the suffix is omitted when used in linker
 * scripts and assembler files.
*/
#if defined(__LINKER__) || defined(__ASSEMBLY__)
# define   U(_x)    (_x)
# define  UL(_x)    (_x)
# define ULL(_x)    (_x)
# define   L(_x)    (_x)
# define  LL(_x)    (_x)
#else
# define   U(_x)    (_x##U)
# define  UL(_x)    (_x##UL)
# define ULL(_x)    (_x##ULL)
# define   L(_x)    (_x##L)
# define  LL(_x)    (_x##LL)
#endif

#define INPLACE(regfield, val) \
	(((val) + UL(0)) << (regfield##_SHIFT))

#define EXTRACT(regfield, reg) \
    (((reg) & MASK(regfield)) >> (regfield##_SHIFT))

#define MASK(regfield) \
    ((~0UL >> (64UL - (regfield##_WIDTH))) << (regfield##_SHIFT))

#define BIT_MASK_ULL(_msb, _lsb) \
	((~ULL(0) >> (63UL - (_msb))) & (~ULL(0) << (_lsb)))

#define ALIGNED(_size, _alignment)		\
			(((unsigned long)(_size) % (_alignment)) == UL(0))

#define ARRAY_SIZE(a)	\
	(sizeof(a) / sizeof((a)[0]))

#define COMPILER_BARRIER() __asm__ volatile ("" ::: "memory")

#define IS_POWER_OF_TWO(x)			\
	(((x) & ((x) - 1)) == 0)

/*
 * The round_up() macro rounds up a value to the given boundary in a
 * type-agnostic yet type-safe manner. The boundary must be a power of two.
 * In other words, it computes the smallest multiple of boundary which is
 * greater than or equal to value.
 *
 * round_down() is similar but rounds the value down instead.
 */
#define round_boundary(value, boundary)		\
	((__typeof__(value))((boundary) - 1))

#define round_up(value, boundary)		\
	((((value) - 1) | round_boundary(value, boundary)) + 1)

#define round_down(value, boundary)		\
	((value) & ~round_boundary(value, boundary))

#endif /* _PAL_CDEFS_H */
