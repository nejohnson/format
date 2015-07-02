/* ***************************************************************************
 * Format - lightweight string formatting library.
 * Copyright (C) 2010-2015, Neil Johnson
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

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "format.h"

/*****************************************************************************/
/* Project Includes                                                          */
/*****************************************************************************/

/**
    Set the size of the test buffers
**/
#define BUF_SZ      ( 1024 )


/*****************************************************************************/
/* Private functions.  Declare as static.                                    */
/*****************************************************************************/

/*****************************************************************************/
/**
    Format consumer function to write characters to a user-supplied buffer.

    @param memptr   Pointer to output buffer
    @param buf      Pointer to buffer of characters to consume from
    @param n        Number of characters from @p buf to consume

    @returns NULL if failed, else address of next output cell.
**/
static void * bufwrite( void * memptr, const char * buf, size_t n )
{
    return ( (char *)memcpy( memptr, buf, n ) + n );
}

/*****************************************************************************/
/**
    Example use of format() to implement the standard sprintf()

    @param buf      Pointer to receiving buffer
    @param fmt      Format string

    @returns Number of characters printed, or -1 if failed.
**/
static int test_sprintf( char *buf, const char *fmt, ... )
{
    va_list arg;
    int done;

    va_start ( arg, fmt );
    done = format( bufwrite, buf, fmt, arg );
    if ( 0 <= done )
        buf[done] = '\0';
    va_end ( arg );

    return done;
}

/*****************************************************************************/
/*****************************************************************************/

static int format_test( unsigned count, char *fmt, double val )
{
    static char buf[BUF_SZ];
    unsigned int i;

    for ( i = 0; i < count; i++ )
        test_sprintf( buf, fmt, val );

    return 0;
}

/*****************************************************************************/

static int native_test( unsigned int count, char *fmt, double val )
{
    static char buf[BUF_SZ];
    unsigned int i;

    for ( i = 0; i < count; i++ )
        sprintf( buf, fmt, val );

    return 0;
}

/*****************************************************************************/

static void run_timed_loop( char *name, int(*pf)(unsigned int, char *, double), unsigned int count, char *fmt, double val )
{
    struct timeval start, end, delta;

    if ( gettimeofday(&start, NULL) != 0 )
       exit(EXIT_FAILURE);

    if ( (pf)(count, fmt, val) != 0 )
       exit(EXIT_FAILURE);

    if ( gettimeofday(&end, NULL) != 0 )
       exit(EXIT_FAILURE);

    timersub(&end, &start, &delta);

    printf( "   %s took %u.%6.6u seconds (%fus per iteration)\n", 
            name, (unsigned int)delta.tv_sec, (unsigned int)delta.tv_usec,
            (((unsigned int)delta.tv_sec)*1000000.0 + (unsigned int)delta.tv_usec)/count );
}

/*****************************************************************************/

static void run_dual_test( unsigned int count, char *fmt, double val )
{
    run_timed_loop( "native", native_test, count, fmt, val );
    run_timed_loop( "format", format_test, count, fmt, val );
}

/*****************************************************************************/
/**
    Run all tests on format library.
**/

#define NUM_ITER	( 1000000 )

static void run_perf_tests( void )
{
    struct {
       unsigned int iter;
       char * fmt;
       double val;
    } tab[] = {
       { NUM_ITER, "%f",        1.0 },
       { NUM_ITER, "%20f",  4.0/3.0 },
       { NUM_ITER, "%.20f", 4.0/3.0 },
       /* Elephant */
       { 0,        NULL,        0.0 }
    };
    int i;
    static unsigned int id = 1;

    for ( i = 0; tab[i].fmt; i++ )
    {
        printf( "\n>> Test %u: %u iterations of \"", id++, tab[i].iter );
        printf( tab[i].fmt, tab[i].val );
        printf( "\"\n" );
        run_dual_test( tab[i].iter, tab[i].fmt, tab[i].val );
    }
}

/*****************************************************************************/
/* Public functions.                                                         */
/*****************************************************************************/

int main( int argc, char *argv[] )
{
    printf( ":: format performance test harness ::\n");
    run_perf_tests();
    return 0;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
