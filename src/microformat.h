/* ****************************************************************************
 * MicroFormat - extremely small string formatting library for 16-bit CPUs
 * Copyright (C) 2010-2023, Neil Johnson
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

#ifndef MICROFORMAT_H
#define MICROFORMAT_H

#include <stdarg.h>

/* Error code returned when problem with format specification */

#define EXBADFORMAT             (-1)

/**
    Interpret format specification passing formatted text to consumer function.
    
    Executes the printf-compatible format specification @a fmt, referring to 
    optional arguments @a ap.  Any output text is passed to the system-provided
    output function "format_putchar".
    
    @param fmt          printf-compatible format specifier.
    @param ap           List of optional format string arguments
    
    @returns            Number of characters sent to @a cons, or EXBADFORMAT.
**/
extern int microformat( const char *    /* fmt  */,
                        va_list         /* ap   */
);

/**
    Micro Format is hard-coded to only output characters through the external
    function "format_putchar", which must be provided by the implemenation.

    @param c     Writes the character c, cast to an unsigned char, to the output.
    
    @returns     The character written as an unsigned char cast to an int, or 
                 -1 on error.
**/

extern int format_putchar( int /* c */ );

#endif
