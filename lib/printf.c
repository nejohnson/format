/* ****************************************************************************
 * Format - lightweight string formatting library.
 * Copyright (C) 2011, Neil Johnson
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

/** The system must provide an implementation of the putchar function **/
extern int putchar( int );

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
    General output consumer function.
    
    This function calls the generic putchar() function, which it is assumed is
    available in the target environment.
    
    @param op      Opaque pointer (UNUSED).
    @param buf     Pointer to input buffer.
    @param n       Number of characters from buffer to send to output.
    
    @return non-NULL.
**/
static void * outfunc( void * op, const char * buf, size_t n )
{
    while ( n-- )
        putchar( *buf++ );

    return (void *)( !NULL );
}

/*****************************************************************************/
/* Public functions.  Declared as per header file.                           */
/*****************************************************************************/

/*****************************************************************************/
/**
    Produce output according to a format string, with optional argument list.
    
    @param fmt      Format specifier.
    @param ap       Argument list.
    
    @return Number of characters printed to the output, or -1.
**/
int vprintf ( const char *fmt, va_list ap )
{
    return format( outfunc, NULL, fmt, ap );
}

/*****************************************************************************/
/**
    Produce output according to a format string, with optional arguments.
    
    @param fmt      Format specifier.
    
    @return Number of characters printed to the output, or -1.
**/
int printf ( const char *fmt, ... )
{
    va_list arg;
    int done;
    
    va_start ( arg, fmt );
    done = vprintf( fmt, arg );
    va_end ( arg );
    
    return done;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
