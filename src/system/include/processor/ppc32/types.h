/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef KERNEL_PROCESSOR_PPC32_TYPES_H
#define KERNEL_PROCESSOR_PPC32_TYPES_H

/** @addtogroup kernelprocessorx86
 * @{ */

/** Define an 8bit signed integer type */
typedef signed char PPC32int8_t;
/** Define an 8bit unsigned integer type */
typedef unsigned char PPC32uint8_t;
/** Define an 16bit signed integer type */
typedef signed short PPC32int16_t;
/** Define an 16bit unsigned integer type */
typedef unsigned short PPC32uint16_t;
/** Define a 32bit signed integer type */
typedef signed int PPC32int32_t;
/** Define a 32bit unsigned integer type */
typedef unsigned int PPC32uint32_t;
/** Define a 64bit signed integer type */
typedef signed long long PPC32int64_t;
/** Define a 64bit unsigned integer type */
typedef unsigned long long PPC32uint64_t;

/** Define a signed integer type for pointer arithmetic */
typedef PPC32int32_t PPC32intptr_t;
/** Define an unsigned integer type for pointer arithmetic */
typedef PPC32uint32_t PPC32uintptr_t;

/** Define a unsigned integer type for physical pointer arithmetic */
typedef PPC32uint32_t PPC32physical_uintptr_t;

/** Define an unsigned integer type for the processor registers */
typedef PPC32uint32_t PPC32processor_register_t;

/** Define ssize_t */
typedef PPC32int32_t PPC32ssize_t;
/** Define size_t */
typedef PPC32uint32_t PPC32size_t;

/** Define the size of one physical page */
#define PAGE_SIZE 4096

/** @} */

#endif
