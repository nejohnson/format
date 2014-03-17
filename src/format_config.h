/* ****************************************************************************
 * Format - lightweight string formatting library.
 * Copyright (C) 2010-2011, Neil Johnson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms,
 * with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ************************************************************************* */

#ifndef FORMAT_CONFIG_H
#define FORMAT_CONFIG_H

/*****************************************************************************/
/* Query the environment about what capabilities are available.              */
/*****************************************************************************/

/** Check if we are in the C hosted configuration **/

#if (__STDC_HOSTED__ == 1)
  #define CONFIG_HAVE_LIBC
#endif

/*****************************************************************************/
/**
    Some platforms va_list is an array type, on others it is a pointer (such as
    pointer to char).  These macros hide this important difference.

    The following is inspired from code from the kannel project.
    See http://www.kannel.org
**/

#if (defined(__linux__) && (defined(__arm__) \
                           || defined(__powerpc__)    \
                           || defined(__s390__)     \
                           || defined(__x86_64)))   \
    || (defined(__FreeBSD__) && defined(__amd64__)) \
    || (defined(DARWIN) && defined(__x86_64__))
  #define CONFIG_VA_LIST_AS_ARRAY_TYPE
#endif

/*****************************************************************************/
/** On some machines it is better (smaller/faster) to do the long div-10 math
    inline rather than calling out to compiler support library.  If this switch
    is defined then format will use an inline divide-by-ten.
    Only recommended for 32-bit machines (eg. ARM) as the necessary math
    produces a 64-bit (or larger) result.
**/
#if defined(__arm__) || defined(__i386__)
  #define CONFIG_USE_INLINE_DIV10
#endif

/*****************************************************************************/
/** Some machines have a separate memory space for constants.  This macro and
     type support addressing strings in this alternate data space.
    The default is treated as normal data accesses.
**/
#if defined(__GNUC__) && defined(__AVR__)
  #include <avr/pgmspace.h>
  #define ROM_CHAR(p)           (pgm_read_byte((PGM_P)(p)))
  #define ROM_PTR_T             PGM_P
  #define ROM_DECL(x)           x PROGMEM
  #define CONFIG_HAVE_ALT_PTR
#else /* Default */
  #define ROM_CHAR(p)           ( *(const char *)(p) )
  #define ROM_PTR_T             const char *
  #define ROM_DECL(x)           x
#endif

/*****************************************************************************/
/** Provide support for floating point output.  Many smaller embedded systems
    simply do not need this functionality so make it possible to remove it at
    build time.  If used at runtime the call to format will return EXBADFORMAT.
**/
/* NOTE: disable FP support while sorting out rounding issues.
   Has no effect on other conversion specifiers.
*/
#define CONFIG_WITH_FP_SUPPORT


#endif /* FORMAT_CONFIG_H */
