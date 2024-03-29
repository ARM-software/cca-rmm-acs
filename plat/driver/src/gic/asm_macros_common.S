/*
 * Copyright (c) 2018-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ASM_MACROS_COMMON_S__
#define __ASM_MACROS_COMMON_S__

	/*
	 * This macro is used to create a function label and place the
	 * code into a separate text section based on the function name
	 * to enable elimination of unused code during linking. It also adds
	 * basic debug information to enable call stack printing most of the
	 * time.
	 */
	.macro func _name
	/*
	 * Add Call Frame Information entry in the .debug_frame section for
	 * debugger consumption. This enables callstack printing in debuggers.
	 * This does not use any space in the final loaded binary, only in the
	 * ELF file.
	 * Note that a function manipulating the CFA pointer location (i.e. the
	 * x29 frame pointer on AArch64) should declare it using the
	 * appropriate .cfi* directives, or be prepared to have a degraded
	 * debugging experience.
	 */
	.cfi_sections .debug_frame
	.section .text.\_name, "ax"
	.type \_name, %function
	.func \_name
	/*
	 * .cfi_startproc and .cfi_endproc are needed to output entries in
	 * .debug_frame
	 */
	.cfi_startproc
	\_name:
	.endm

	/*
	 * This macro is used to mark the end of a function.
	 */
	.macro endfunc _name
	.endfunc
	.cfi_endproc
	.size \_name, . - \_name
	.endm

	/*
	 * This macro declares an array of 1 or more stacks, properly
	 * aligned and in the requested section
	 */
#define STACK_ALIGN	6

	.macro declare_stack _name, _section, _size, _count
	.if ((\_size & ((1 << STACK_ALIGN) - 1)) <> 0)
	  .error "Stack size not correctly aligned"
	.endif
	.section    \_section, "aw", %nobits
	.align STACK_ALIGN
	\_name:
	.space ((\_count) * (\_size)), 0
	.endm

#endif /* __ASM_MACROS_COMMON_S__ */

