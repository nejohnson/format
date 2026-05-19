/* ****************************************************************************
 * Format - lightweight string formatting library.
 * Copyright (C) 2011-2026, Neil Johnson
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

/*****************************************************************************/
/* System Includes                                                           */
/*****************************************************************************/

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

/*****************************************************************************/
/* Project Includes                                                          */
/*****************************************************************************/

#include "format.h"

#include "printf.h"

/*****************************************************************************/
/* Private function prototypes.  Declare as static.                          */
/*****************************************************************************/

/*****************************************************************************/
/**
    Null consumer - discards all output.

    @param op      Opaque pointer (returned unchanged).
    @param buf     Pointer to input buffer (unused).
    @param n       Number of characters (unused).

    @return op unchanged.
**/
static void * nullcons( void * op, const char * buf, size_t n )
{
    (void)buf;
    (void)n;
    return op;
}

/*****************************************************************************/
/**
    Copy characters from buffer into memory.

    Do not use memcpy as it is not available in a freestanding implementation.

    @param memptr  Pointer to output buffer.
    @param buf     Pointer to input buffer.
    @param n       Number of characters from buffer to copy to output buffer.

    @return Address of next empty cell in output buffer.
**/
static void * bufwrite( void * memptr, const char * buf, size_t n )
{
    char *dst = (char *)memptr;

    while ( n-- )
        *dst++ = *buf++;

    return dst;
}

/*****************************************************************************/
/* Public functions.  Declared as per header file.                           */
/*****************************************************************************/

/*****************************************************************************/
/**
    Produce output according to a format string, with argument list.

    If the result (including null terminator) fits within @a *lengthp bytes of
    @a resultbuf, @a resultbuf is used directly.  Otherwise a new buffer is
    allocated with malloc() and the caller is responsible for freeing it.

    On success @a *lengthp is set to the number of characters written
    (excluding the null terminator), matching the GNU extension convention.

    A va_copy of @a ap is taken before @a ap is consumed by the counting pass,
    so the caller's va_list state is consumed exactly once as expected.

    @param resultbuf    Caller-supplied buffer, or NULL.
    @param lengthp      On entry: capacity of @a resultbuf.
                        On success: length of formatted result.
    @param fmt          Format specifier.
    @param ap           Argument list.

    @return Pointer to the formatted result (either @a resultbuf or a newly
            allocated buffer), or NULL on error.
**/
char * vasnprintf( char *resultbuf, size_t *lengthp, const char *fmt, va_list ap )
{
    va_list ap2;
    int len;
    char *buf;

    va_copy( ap2, ap );

    len = format( nullcons, (void *)!NULL, fmt, ap );
    if ( len < 0 )
    {
        va_end( ap2 );
        return NULL;
    }

    if ( resultbuf != NULL && (size_t)len < *lengthp )
    {
        buf = resultbuf;
    }
    else
    {
        buf = (char *)malloc( (size_t)len + 1 );
        if ( !buf )
        {
            va_end( ap2 );
            return NULL;
        }
    }

    format( bufwrite, buf, fmt, ap2 );
    buf[len] = '\0';

    va_end( ap2 );
    *lengthp = (size_t)len;
    return buf;
}

/*****************************************************************************/
/**
    Produce output according to a format string, with optional arguments.

    If the result (including null terminator) fits within @a *lengthp bytes of
    @a resultbuf, @a resultbuf is used directly.  Otherwise a new buffer is
    allocated with malloc() and the caller is responsible for freeing it.

    On success @a *lengthp is set to the number of characters written
    (excluding the null terminator), matching the GNU extension convention.

    @param resultbuf    Caller-supplied buffer, or NULL.
    @param lengthp      On entry: capacity of @a resultbuf.
                        On success: length of formatted result.
    @param fmt          Format specifier.

    @return Pointer to the formatted result (either @a resultbuf or a newly
            allocated buffer), or NULL on error.
**/
char * asnprintf( char *resultbuf, size_t *lengthp, const char *fmt, ... )
{
    va_list arg;
    char *result;

    va_start( arg, fmt );
    result = vasnprintf( resultbuf, lengthp, fmt, arg );
    va_end( arg );

    return result;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
