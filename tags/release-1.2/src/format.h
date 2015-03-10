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

#ifndef FORMAT_H
#define FORMAT_H

#include <stdarg.h>

/* Error code returned when problem with format specification */

#define EXBADFORMAT             (-1)

/**
    Interpret format specification passing formatted text to consumer function.
    
    Executes the printf-compatible format specification @a fmt, referring to 
    optional arguments @a ap.  Any output text is passed to caller-provided
    consumer function @a cons, which also takes caller-provided opaque pointer
    @a arg.
    
    @param cons         Pointer to caller-provided consumer function.
    @param arg          Opaque pointer passed through to @a cons.
    @param fmt          printf-compatible format specifier.
    @param ap           List of optional format string arguments
    
    @returns            Number of characters sent to @a cons, or EXBADFORMAT.
**/
extern int format( void * (* /* cons */) (void *, const char *, size_t),
             void *          /* arg  */,
             const char *    /* fmt  */,
             va_list         /* ap   */
);

/*    The Consumer Function
 *
 * The consumer function 'cons' must have the following type:
 *
 *   void * cons ( void * arg, const char * s, size_t n )
 *
 * You may give your function any valid C name.
 *
 * It takes an opaque pointer argument, which may be modified by the call.
 * The second and third arguments specify the source string and the number of
 *  characters to take from the string.
 * If the call is successful then a new value of the opaque pointer is returned,
 *  which will be passed on the next time the function is called.
 * In case of an error, the function returns NULL.
 */

#endif
