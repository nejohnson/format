/* ***************************************************************************
 * Format - lightweight string formatting library.
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

/*****************************************************************************/
/* System Includes                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(__AVR__)
  #include <avr/io.h>
  #include <avr/pgmspace.h>
#endif

#include "microformat.h"
#include "format_config.h"

/*****************************************************************************/
/* Project Includes                                                          */
/*****************************************************************************/

/**
    Set the size of the test buffers
**/
#define BUF_SZ      ( 1024 )

#define UNUSED      0

static char buf[BUF_SZ];
static unsigned int f = 0;

/**
    Wrapper macro to standardise checking of test results.

    @param exs              Expected result string
    @param rtn              Expected return value
    @param fmt              Test format string
    @param args...          Argument list
**/
#define TEST(exs, rtn, fmt, args...)   do {                                 \
            int r = test_sprintf(buf,(fmt), ## args);                       \
            printf( "[Test  @ %3d] ", __LINE__ );                           \
            if ( r != (rtn) )                                               \
                {printf("########### FAIL: produced \"%s\", returned %d, expected %d.", buf,r, (rtn) );f+=1;} \
            else if ( strcmp( (exs), buf ) )                                \
                {printf("########### FAIL: produced \"%s\", expected \"%s\".", buf,(exs));f+=1;}\
            else                                                            \
                printf("PASS");                                             \
            printf("\n");                                                   \
            } while( 0 );

/**
    Wrapper macro to run a test that is expected to fail

    @param fmt              Test format string
    @param args...          Argument list
**/
#define FAIL(fmt, args...)        do {                                      \
            int r = test_sprintf(buf,(fmt),## args);                        \
            printf( "[Test  @ %3d] ", __LINE__ );                           \
            if ( r != EXBADFORMAT )                                         \
                {printf("**** FAIL: returned %d, expected EXBADFORMAT", r);f+=1;} \
            else                                                            \
                printf("PASS");                                             \
            printf("\n");                                                   \
            } while( 0 );

/**
    Check if two integers are the same and print out accordingly.
**/
#define CHECK(a,b)      do { printf("[Check @ %3d] ", __LINE__ );           \
                            if ((a)==(b))                                   \
                                printf( "PASS");                            \
                            else {printf("**** FAIL: got %d, expected %d",(a),(b));f+=1;}\
                            printf("\n");                                   \
                        }while(0);

/*****************************************************************************/
/* Private functions.  Declare as static.                                    */
/*****************************************************************************/

static char *g_linebuf;
static size_t g_bufidx;

/*****************************************************************************/
/**
    Microformat test output function

    @param c        The character to output

    @returns The character output, or -1 if failed.
**/
int format_putchar( int c )
{
    g_linebuf[g_bufidx++] = c;
    return c;
}

/*****************************************************************************/
/**
    Example use of format() to implement the standard sprintf()

    @param pbuf     Pointer to receiving buffer
    @param fmt      Format string

    @returns Number of characters printed, or -1 if failed.
**/
static int test_sprintf( char *pbuf, const char *fmt, ... )
{
    va_list arg;
    int done;

    g_linebuf = pbuf;
    g_bufidx = 0;
    g_linebuf[g_bufidx] = '\0';
    
    va_start ( arg, fmt );
    done = microformat( fmt, arg );
    if ( 0 <= done )
        pbuf[done] = '\0';
    va_end ( arg );

    return done;
}

/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/**
    Execute tests on plain strings
**/
static void test( void )
{
    printf( "Testing basic strings\n" );

    TEST( "", EXBADFORMAT, NULL, UNUSED );

    /* Empty string */
    TEST( "", 0, "", UNUSED );

    /* Basic tests */
    TEST( "a", 1, "a", UNUSED );
    TEST( "abc", 3, "abc", UNUSED );

    /* Long string */
    TEST( "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghij"
          "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghij",
          100,
          "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghij"
          "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghij", UNUSED );

    /* Escape characters */
    TEST( "\a\b\f\n\r\t\v", 7, "\a\b\f\n\r\t\v", UNUSED );
    TEST( "\'\"\\\?", 4, "\'\"\\\?", UNUSED );
    TEST( "\123\x69", 2, "\123\x69", UNUSED );
}

/*****************************************************************************/
/**
    Execute tests on '%' conversion specifier
**/
static void test_pc( void )
{
    printf( "Testing \"%%%%\"\n" );

    /* Basic test */
    TEST( "%", 1, "%%", UNUSED );

    /* Check all flags, precision, width, length are ignored */
    TEST( "%", 1, "%-+ 012.%", UNUSED );
    TEST( "%", 1, "%-+ 012.24%", UNUSED );

    /* Check sequential conversions */
    TEST( "%c", 2, "%%c", UNUSED );
    TEST( "%%%", 3, "%%%%%%", UNUSED );
    TEST( "% % %", 5, "%% %% %%", UNUSED );
}

/*****************************************************************************/
/**
    Execute tests on 'c' conversion specifier
**/
static void test_c( void )
{
    printf( "Testing \"%%c\"\n" );

    /* Basic test */
    TEST( "a", 1, "%c", 'a' );

    /* Check all flags, width, length are ignored */
    TEST( "a", 1, "%-+ 012c", 'a' );
    TEST( "a", 1, "%-+ 012c", 'a' );

    /* Check sequential conversions */
    TEST( "ac", 2, "%cc", 'a' );
    TEST( "abc", 3, "%c%c%c", 'a', 'b', 'c' );
    TEST( "a b c", 5, "%c %c %c", 'a', 'b', 'c' );

    /* Check failure cases */
    FAIL( "%.81c", '-');
}

/*****************************************************************************/
/**
    Execute tests on 's' conversion specifier
**/
static void test_s( void )
{
    printf( "Testing \"%%s\"\n" );

    /* Basic string operations */
    TEST( "hello", 5, "%s", "hello" );
    TEST( "goodbye", 7, "%sbye", "good" );

    TEST( "   hello", 8, "%8s", "hello" );
    TEST( "hello   ", 8, "%-8s", "hello" );
    TEST( "     hel", 8, "%8.3s", "hello" );
    TEST( "hel     ", 8, "%-8.3s", "hello" );
    TEST( "hel", 3, "%.3s", "hello" );

    /* NULL pointer handled specially */
    TEST( "?", 1, "%s", NULL );

    /* Check unused flags and lengths are ignored */
    TEST( "hello", 5, "%+ 0s", "hello" );
    TEST( "hello", 5, "%+ 0s", "hello" );

    /* Checking padding */
    TEST( "                                                                               x", 80, "%80s", "x");
    TEST( "01234567890123456789012345678901234567890123456789012345678901234567890123456789", 80, "%.80s",
          "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");

    /* Check failure cases */
    FAIL( "%81s", "x" );
    FAIL( "%.81s", "x");
}

/*****************************************************************************/
/**
    Execute tests on 'p' conversion specifier

    *May* work for 16,32 and 64-bit pointers.  Its a bit iffy really.
    On really weird architectures this is just wild.
**/
static void test_p( void )
{
    int ptr_size = 2; //sizeof( int * );
    int * p0 = (int *)0x0;
    int * p1 = (int *)0x1234;
    int * p2 = (int *)(-1);

    printf( "Testing \"%%p\" on platform with %d-byte pointers\n", ptr_size );

    if ( ptr_size == 2 )
    {
        TEST( "0x0000", 6, "0x%p", p0 );
        TEST( "0x1234", 6, "0x%p", p1 );
        TEST( "0xFFFF", 6, "0x%p", p2 );

        /* Check all flags, precision, width, length are ignored */
        TEST( "0xFFFF", 6, "0x%-+ 012.24p", p2 );
    }
}

/*****************************************************************************/
/**
    Execute tests on 'd' conversion specifier.

d        The int argument is converted to signed decimal in the style [-]dddd.
           The precision specifies the minimum number of digits to appear; if
           the value being converted can be represented in fewer digits, it is
           expanded with leading zeros. The default precision is 1. The result
           of converting a zero value with a precision of zero is no characters.
**/
static void test_d( void )
{
    printf( "Testing \"%%d\" and \"%%i\"\n" );

    TEST( "0", 1, "%d", 0 );
    TEST( "1234", 4, "%d", 1234 );
    TEST( "-1234", 5, "%d", -1234 );

    /* 0 value with 0 precision produces no characters */
    TEST( "", 0, "%.0d", 0 );

    /* Precision sets minimum number of digits, zero-padding if necessary */
    TEST( "001234", 6, "%.6d", 1234 );

    /* Width sets minimum field width */
    TEST( "  1234", 6, "%6d", 1234 );
    TEST( " -1234", 6, "%6d", -1234 );
    TEST( "1234", 4, "%2d", 1234);
    TEST( "1234", 4, "%02d", 1234 );

    /* Precision sets minimum number of digits for the value */
    TEST( "001234", 6, "%.6d", 1234 );

    /* '-' flag */
    TEST( "1234  ", 6, "%-6d", 1234 );
    TEST( "-1234 ", 6, "%-6d", -1234 );

    /* '0' flag */
    TEST( "001234", 6, "%06d", 1234 );
    TEST( "1234  ", 6, "%-06d", 1234 ); /* '-' kills '0' */
    TEST( "  1234", 6, "%06.1d", 1234 ); /* prec kills '0' */

    /* '+' */
    TEST( "+1234", 5, "%+d", 1234 );
    TEST( "-1234", 5, "%+d", -1234);

    /* space */
    TEST( " 1234", 5, "% d", 1234 );
    TEST( "-1234", 5, "% d", -1234 );
    TEST( " ", 1, "% .0d", 0 );

    TEST( "+1234", 5, "%+ d", 1234 ); /* '+' kills space */
    TEST( "-1234", 5, "%+ d", -1234); /* '+' kills space */
    TEST( "+", 1, "%+ .0d", 0 ); /* '+' kills space */
}

/*****************************************************************************/
/**
    Execute tests on b,u,x,X conversion specifiers.

b,u,x,X  The unsigned int argument is converted to unsigned binary (b),
           unsigned decimal (u), or unsigned hexadecimal
           notation (x or X) in the style dddd; the letters "abcdef" are used
           for x conversion and the letters "ABCDEF" for X conversion. The
           precision specifies the minimum number of digits to appear; if the
           value being converted can be represented in fewer digits, it is
           expanded with leading zeros.  The default precision is 1. The result
           of converting a zero value with a precision of zero is no characters.
**/
static void test_buxX( void )
{
    printf( "Testing \"%%b\", \"%%u\", \"%%x\" and \"%%X\"\n" );

    TEST( "0", 1, "%b", 0 );
    TEST( "0", 1, "%u", 0 );
    TEST( "0", 1, "%x", 0 );
    TEST( "0", 1, "%X", 0 );

    TEST( "1101", 4, "%b", 13 );
    TEST( "1234", 4, "%u", 1234 );

    TEST( "12cd", 4, "%x", 0x12cd );
    TEST( "12CD", 4, "%X", 0x12CD );

    /* 0 value with 0 precision produces no characters */
    TEST( "", 0, "%.0b", 0 );
    TEST( "", 0, "%.0u", 0 );
    TEST( "", 0, "%.0x", 0 );
    TEST( "", 0, "%.0X", 0 );

    /* Precision sets minimum number of digits, zero-padding if necessary */
    TEST( "001101", 6, "%.6b", 13 );
    TEST( "001234", 6, "%.6u", 1234 );

    TEST( "00000012cd", 10, "%.10x", 0x12cd );
    TEST( "00000012CD", 10, "%.10X", 0x12CD );

    /* Width sets minimum field width */
    TEST( "  1101", 6, "%6b", 13 );
    TEST( "1101", 4, "%2b", 13);
    TEST( "  1234", 6, "%6u", 1234 );
    TEST( "1234", 4, "%2u", 1234);
    TEST( "1234", 4, "%02u", 1234 );

    TEST( "      12cd", 10, "%10x", 0x12cd );
    TEST( "12cd", 4, "%2x", 0x12cd);
    TEST( "      12CD", 10, "%10X", 0x12CD );
    TEST( "12CD", 4, "%2X", 0x12CD);

    /* Precision sets minimum number of digits for the value */
    TEST( "001101", 6, "%.6b", 13 );
    TEST( "001234", 6, "%.6u", 1234 );

    TEST( "00000012cd", 10, "%.10x", 0x12cd );
    TEST( "00000012CD", 10, "%.10X", 0x12cd );

    /* '-' flag */
    TEST( "1101  ", 6, "%-6b", 13 );
    TEST( "1234  ", 6, "%-6u", 1234 );

    TEST( "12cd      ", 10, "%-10x", 0x12cd );
    TEST( "12CD      ", 10, "%-10X", 0x12cd );

    /* '0' flag */
    TEST( "001101", 6, "%06b", 13 );
    TEST( "1101  ", 6, "%-06b", 13 ); /* '-' kills '0' */
    TEST( "  1101", 6, "%06.1b", 13 ); /* prec kills '0' */
    TEST( "001234", 6, "%06u", 1234 );
    TEST( "1234  ", 6, "%-06u", 1234 ); /* '-' kills '0' */
    TEST( "  1234", 6, "%06.1u", 1234 ); /* prec kills '0' */

    TEST( "00000012cd", 10, "%010x", 0x12cd );
    TEST( "12cd      ", 10, "%-010x", 0x12cd ); /* '-' kills '0' */
    TEST( "      12cd", 10, "%010.1x", 0x12cd ); /* prec kills '0' */
    TEST( "00000012CD", 10, "%010X", 0x12cd );
    TEST( "12CD      ", 10, "%-010X", 0x12cd ); /* '-' kills '0' */
    TEST( "      12CD", 10, "%010.1X", 0x12cd ); /* prec kills '0' */

    /* No effect: +,space */
    TEST( "1101", 4, "%+ b", 13 );
    TEST( "12cd", 4, "%+ x", 0x12cd );
    TEST( "12CD", 4, "%+ X", 0x12cd );
}

/*****************************************************************************/
/**
    Run all tests on format library.
**/
static void run_tests( void )
{
    test();
    test_pc();
    test_c();
    test_s();
    test_p();
    test_d();
    test_buxX();
    
    printf( "-----------------------\n"
            "Summary: %s (%u failures)\n", f ? "FAIL" : "PASS", f );
}

/*****************************************************************************/
/* Public functions.                                                         */
/*****************************************************************************/

/* Target platform specific functionality */

#if defined(__AVR__)

/* For the simulator in AVRstudio we can talk to the UART and view on HAPSIM.
   Tested on the ATmega2560.
   Note that in the simulator we don't need to set up the baud rate.
*/
static int avr_putchar( char c, FILE *fp)
{
    UDR0 = c;
    return 0;
}

static void system_init( void )
{
    fdevopen(&avr_putchar,NULL);
    UCSR0B = 0x08;
}

#else

/* Default system initialiser does nothing */

static void system_init( void )
{
    /* empty */
}

#endif


int main( int argc, char *argv[] )
{
    system_init();

    printf( ":: format test harness ::\n");
    run_tests();
    return 0;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
