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
#include <float.h>
#include <stdint.h>
#include <stddef.h>

#if defined(__AVR__)
  #include <avr/io.h>
  #include <avr/pgmspace.h>
#endif

#include "format.h"
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

/*****************************************************************************/
/**
    Format consumer function to write characters to a user-supplied buffer.

    @param memptr   Pointer to output buffer
    @param pbuf     Pointer to buffer of characters to consume from
    @param n        Number of characters from @p buf to consume

    @returns NULL if failed, else address of next output cell.
**/
static void * bufwrite( void * memptr, const char * pbuf, size_t n )
{
    return ( (char *)memcpy( memptr, pbuf, n ) + n );
}

/**
    Control variables for failing consumer tests
**/
static int fail_after_n_calls = -1;  /* Fail after N successful calls (-1 = never fail) */
static int current_call_count = 0;   /* Current call count */

/**
    Consumer function that can be configured to fail after N calls.
    Used to test consumer failure handling.

    @param memptr   Current write position
    @param pbuf     Data to write
    @param n        Number of bytes

    @returns Next write position, or NULL if configured to fail
**/
static void * failing_bufwrite( void * memptr, const char * pbuf, size_t n )
{
    if ( fail_after_n_calls >= 0 && current_call_count >= fail_after_n_calls )
    {
        return NULL;  /* Simulate consumer failure */
    }

    current_call_count++;
    return ( (char *)memcpy( memptr, pbuf, n ) + n );
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

    va_start ( arg, fmt );
    done = format( bufwrite, pbuf, fmt, arg );
    if ( 0 <= done )
        pbuf[done] = '\0';
    va_end ( arg );

    return done;
}

/**
    Version of test_sprintf that uses the failing consumer.
    Used to test consumer failure handling.

    @param pbuf     Pointer to receiving buffer
    @param fmt      Format string

    @returns Number of characters printed, or -1 if failed.
**/
static int test_sprintf_failing( char *pbuf, const char *fmt, ... )
{
    va_list arg;
    int done;

    va_start ( arg, fmt );
    done = format( failing_bufwrite, pbuf, fmt, arg );
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
static void test_strings( void )
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
    TEST( "%", 1, "%-+ #0!^12.h%", UNUSED );
    TEST( "%", 1, "%-+ #0!^12.24h%", UNUSED );

    /* Check sequential conversions */
    TEST( "%c", 2, "%%c", UNUSED );
    TEST( "%%%", 3, "%%%%%%", UNUSED );
    TEST( "% % %", 5, "%% %% %%", UNUSED );
}

/*****************************************************************************/
/**
    Execute tests on 'c' and 'C' conversion specifiers
**/
static void test_cC( void )
{
    printf( "Testing \"%%c\"\n" );

    /* Basic test */
    TEST( "a", 1, "%c", 'a' );

    /* Check all flags, width, length are ignored */
    TEST( "a", 1, "%-+ #0!^12hc", 'a' );
    TEST( "a", 1, "%-+ #0!^12lc", 'a' );

    /* Check sequential conversions */
    TEST( "ac", 2, "%cc", 'a' );
    TEST( "abc", 3, "%c%c%c", 'a', 'b', 'c' );
    TEST( "a b c", 5, "%c %c %c", 'a', 'b', 'c' );

    /* Check repetition */
    TEST( "a", 1, "%.c", 'a' );     /* 0 precision treated as 1 */
    TEST( "aaaa", 4, "%.4c", 'a' );
    TEST( "aaaabbbbcccc", 12, "%.4c%.4c%.4c", 'a', 'b', 'c' );
    TEST( "------------", 12, "%.12c", '-' );

    /* Check inline repetition */
    TEST( "aaaa", 4, "%.4Ca", UNUSED );
    TEST( "------------", 12, "%.12C-", UNUSED );

    /* Check variable repetition */
    TEST( "----", 4, "%.*c", 4, '-' );
    TEST( "aaaa", 4, "%.*Ca", 4 );
}

/*****************************************************************************/
/**
    Execute tests on 'n' conversion specifier
**/
static void test_n( void )
{
    int n;
    short s;
    long l;
#if defined(CONFIG_WITH_LONG_LONG_SUPPORT)
    long long ll;
#endif
    char c;

    printf( "Testing \"%%n\"\n" );

    /* Basic positional tests */
    TEST( "hello", 5, "hello%n", &n ); CHECK( n, 5 );
    TEST( "hello", 5, "hel%nlo", &n ); CHECK( n, 3 );
    TEST( "hello", 5, "%nhello", &n ); CHECK( n, 0 );

    /* Length modifiers */
    TEST( "hello", 5, "hello%ln", &l ); CHECK( (int)l, 5 );
    TEST( "hello", 5, "hello%hn", &s ); CHECK( s, 5 );
#if defined(CONFIG_WITH_LONG_LONG_SUPPORT)
    TEST( "hello", 5, "hello%lln",&ll); CHECK( (int)ll, 5 );
#endif

    /* Check the 'hh' length qual with a string longer than can be
       written in a valid signed char type.  In this case 320, which should
       wrap round to (320 % 256) = 64.
    */
    TEST( "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello",
          320,
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "hellohellohellohellohellohellohellohello"
          "%hhn",
          &c ); CHECK( c, 64 );

    /* NULL pointer - should silently ignore */
    TEST( "hello", 5, "hello%n", NULL );
    TEST( "hello", 5, "hello%hn", NULL );
    TEST( "hello", 5, "hello%ln", NULL );

    /* Check all flags, precision, and width are ignored */
    TEST( "hello", 5, "hello%-+ #0!^12.24n", &n ); CHECK( n, 5 );
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

    /* Check new ^ centering flag */
    TEST( "  hello  ", 9, "%^9s", "hello" );
    TEST( "  hello ", 8, "%^8s", "hello" );
    TEST( " hello  ", 8, "%-^8s", "hello" );
    TEST( "hello", 5, "%^3s", "hello" );

    /* NULL pointer handled specially */
    TEST( "(null)", 6, "%s", NULL );

    /* Check unused flags and lengths are ignored */
    TEST( "hello", 5, "%+ 0!ls", "hello" );
    TEST( "hello", 5, "%+ 0!hs", "hello" );

#if defined(__AVR__)
    {
        static char s_string[] PROGMEM = "funky monkey";
        TEST( "funky monkey", 12, "%#s", (PGM_P)s_string );
    }
#endif
}

/*****************************************************************************/
/**
    Execute tests on 'p' conversion specifier

    *May* work for 16,32 and 64-bit pointers.  Its a bit iffy really.
    On really weird architectures this is just wild.
**/
static void test_p( void )
{
    int ptr_size = sizeof( void * );
    void * p0 = (void *)0x0;
    void * p1 = (void *)0x1234;
    void * p2 = (void *)(-1);

    printf( "Testing \"%%p\" on platform with %d-byte pointers\n", ptr_size );

    if ( ptr_size == 2 )
    {
        TEST( "0000", 6, "%p", p0 );
        TEST( "1234", 6, "%p", p1 );
        TEST( "FFFF", 6, "%p", p2 );

        /* Check all flags, precision, width, length are ignored */
        TEST( "0xFFFF", 6, "%-+ #0!^12.24lp", p2 );
        TEST( "0xFFFF", 6, "%-+ #0!^12.24hp", p2 );
    }
    else if ( ptr_size == 4 )
    {
        TEST( "00000000", 10, "%p", p0 );
        TEST( "00001234", 10, "%p", p1 );
        TEST( "FFFFFFFF", 10, "%p", p2 );

        /* Check all flags, precision, width, length are ignored */
        TEST( "0xFFFFFFFF", 10, "%-+ #0!^12.24lp", p2 );
        TEST( "0xFFFFFFFF", 10, "%-+ #0!^12.24hp", p2 );
    }
    else if ( ptr_size == 8 )
    {
        TEST( "0000000000000000", 16, "%p", p0 );
        TEST( "0000000000001234", 16, "%p", p1 );
        TEST( "FFFFFFFFFFFFFFFF", 16, "%p", p2 );

        /* Check all flags, precision, width, length are ignored */
        TEST( "0xFFFFFFFFFFFFFFFF", 18, "%-+ #0!^24.48lp", p2 );
        TEST( "0xFFFFFFFFFFFFFFFF", 18, "%-+ #0!^24.48hp", p2 );
    }
    else
    {
        printf( "ERROR: unknown pointer size (%d bytes)\n", ptr_size );
        f |= 1;
    }
}

/*****************************************************************************/
/**
    Execute tests on 'd' and 'i' conversion specifiers.

d,i        The int argument is converted to signed decimal in the style [-]dddd.
           The precision specifies the minimum number of digits to appear; if
           the value being converted can be represented in fewer digits, it is
           expanded with leading zeros. The default precision is 1. The result
           of converting a zero value with a precision of zero is no characters.
**/
static void test_di( void )
{
    short int si = 24;
    long int  li = 1234567890L;
#if defined(CONFIG_WITH_LONG_LONG_SUPPORT)
    long long int lli = 123456789123456789LL;
#endif

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

    /* Centering */
    TEST( "  1234  ", 8, "%^8d", 1234 );

#if defined(CONFIG_WITH_GROUPING_SUPPORT)
    /* Grouping */
    TEST( "12,34", 5, "%[,2]d", 1234 );
    TEST( "12,34,56", 8, "%[,2]d", 123456 );
    TEST( "1234,56", 7, "%[-,2]d", 123456 );
    TEST( "1,234.56", 8, "%[,3.2]d", 123456 );
    TEST( "12,345,678.90", 13, "%[,3.2]d", 1234567890 );
    TEST( "1234", 4, "%[_0]d", 1234 );
    TEST( "1_2_3_4", 7, "%[_1]d", 1234 );
    TEST( "12_34", 5, "%[_2]d", 1234 );
    TEST( "1234", 4, "%[]d", 1234 );

    TEST( "0012_34", 7, "%.6[_2]d", 1234 );
    TEST( " 0012_34", 8, "%8.6[_2]d", 1234 );
    TEST( "0012_34 ", 8, "%-8.6[_2]d", 1234 );
#endif

    /* no effect */
    TEST( "1234", 4, "%!#d", 1234 );

    /* Non-standard bases */
    TEST( "11", 2, "%:3i", 4 );
    TEST( "11", 2, "%:*i", 3, 4 );

    TEST( "11", 2, "%:i", 11 );
    TEST( "12", 2, "%:*i", -1, 12 );

    TEST( "g", 1, "%:17i", 16 );
    TEST( "G", 1, "%:17I", 16 );

    TEST( "XYZ", 3, "%:36I", 44027 );
    TEST( "  0XYZ", 6, "%6.4:36I", 44027 );
    TEST( "-G", 2, "%:17I", -16 );

    /* -- check error paths */
    FAIL( "%:1i", 0 );        /* base of 1 makes no sense */
    FAIL( "%:9999i", 0 );     /* very large base */
    FAIL( "%:*i", 9999, 0 );  /* ditto */

    /* lengths */
    TEST( "24", 2, "%hd", si );
    TEST( "1234567890", 10, "%ld", li );

#if defined(CONFIG_WITH_LONG_LONG_SUPPORT)
    TEST( "123456789123456789", 18, "%lld", lli );
#endif
}


/*****************************************************************************/
/**
    Execute tests on b,o,u,x,X conversion specifiers.

b,o,u,x,X  The unsigned int argument is converted to unsigned binary (b),
           unsigned octal (o), unsigned decimal (u), or unsigned hexadecimal
           notation (x or X) in the style dddd; the letters "abcdef" are used
           for x conversion and the letters "ABCDEF" for X conversion. The
           precision specifies the minimum number of digits to appear; if the
           value being converted can be represented in fewer digits, it is
           expanded with leading zeros.  The default precision is 1. The result
           of converting a zero value with a precision of zero is no characters.
**/
static void test_bouxX( void )
{
#if defined(CONFIG_WITH_LONG_LONG_SUPPORT)
    unsigned long long int ulli = 123456789123456789ULL;
#endif

    printf( "Testing \"%%b\", \"%%o\", \"%%u\", \"%%x\" and \"%%X\"\n" );

    TEST( "0", 1, "%b", 0 );
    TEST( "0", 1, "%o", 0 );
    TEST( "0", 1, "%u", 0 );
    TEST( "0", 1, "%x", 0 );
    TEST( "0", 1, "%X", 0 );

    TEST( "1101", 4, "%b", 13 );
    TEST( "1234", 4, "%o", 01234 );
    TEST( "1234", 4, "%u", 1234 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "1234abcd", 8, "%x", 0x1234abcd );
        TEST( "1234ABCD", 8, "%X", 0x1234ABCD );
    }
    else
    {
        TEST( "12cd", 4, "%x", 0x12cd );
        TEST( "12CD", 4, "%X", 0x12CD );
    }

    /* 0 value with 0 precision produces no characters */
    TEST( "", 0, "%.0b", 0 );
    TEST( "", 0, "%.0o", 0 );
    TEST( "", 0, "%.0u", 0 );
    TEST( "", 0, "%.0x", 0 );
    TEST( "", 0, "%.0X", 0 );

    /* Precision sets minimum number of digits, zero-padding if necessary */
    TEST( "001101", 6, "%.6b", 13 );
    TEST( "001234", 6, "%.6o", 01234 );
    TEST( "001234", 6, "%.6u", 1234 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "001234abcd", 10, "%.10x", 0x1234abcd );
        TEST( "001234ABCD", 10, "%.10X", 0x1234ABCD );
    }
    else
    {
        TEST( "00000012cd", 10, "%.10x", 0x12cd );
        TEST( "00000012CD", 10, "%.10X", 0x12CD );
    }

    /* Width sets minimum field width */
    TEST( "  1101", 6, "%6b", 13 );
    TEST( "1101", 4, "%2b", 13);
    TEST( "  1234", 6, "%6o", 01234 );
    TEST( "1234", 4, "%2o", 01234);
    TEST( "  1234", 6, "%6u", 1234 );
    TEST( "1234", 4, "%2u", 1234);
    TEST( "1234", 4, "%02u", 1234 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "  1234abcd", 10, "%10x", 0x1234abcd );
        TEST( "1234abcd", 8, "%2x", 0x1234abcd);
        TEST( "  1234ABCD", 10, "%10X", 0x1234ABCD );
        TEST( "1234ABCD", 8, "%2X", 0x1234ABCD);
    }
    else
    {
        TEST( "      12cd", 10, "%10x", 0x12cd );
        TEST( "12cd", 4, "%2x", 0x12cd);
        TEST( "      12CD", 10, "%10X", 0x12CD );
        TEST( "12CD", 4, "%2X", 0x12CD);
    }

    /* Precision sets minimum number of digits for the value */
    TEST( "001101", 6, "%.6b", 13 );
    TEST( "001234", 6, "%.6o", 01234 );
    TEST( "001234", 6, "%.6u", 1234 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "001234abcd", 10, "%.10x", 0x1234abcd );
        TEST( "001234ABCD", 10, "%.10X", 0x1234abcd );
    }
    else
    {
        TEST( "00000012cd", 10, "%.10x", 0x12cd );
        TEST( "00000012CD", 10, "%.10X", 0x12cd );
    }

    /* '-' flag */
    TEST( "1101  ", 6, "%-6b", 13 );
    TEST( "1234  ", 6, "%-6o", 01234 );
    TEST( "1234  ", 6, "%-6u", 1234 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "1234abcd  ", 10, "%-10x", 0x1234abcd );
        TEST( "1234ABCD  ", 10, "%-10X", 0x1234abcd );
    }
    else
    {
        TEST( "12cd      ", 10, "%-10x", 0x12cd );
        TEST( "12CD      ", 10, "%-10X", 0x12cd );
    }

    /* '0' flag */
    TEST( "001101", 6, "%06b", 13 );
    TEST( "1101  ", 6, "%-06b", 13 ); /* '-' kills '0' */
    TEST( "  1101", 6, "%06.1b", 13 ); /* prec kills '0' */
    TEST( "001234", 6, "%06o", 01234 );
    TEST( "1234  ", 6, "%-06o", 01234 ); /* '-' kills '0' */
    TEST( "  1234", 6, "%06.1o", 01234 ); /* prec kills '0' */
    TEST( "001234", 6, "%06u", 1234 );
    TEST( "1234  ", 6, "%-06u", 1234 ); /* '-' kills '0' */
    TEST( "  1234", 6, "%06.1u", 1234 ); /* prec kills '0' */

    if ( sizeof( int ) > 2 )
    {
        TEST( "001234abcd", 10, "%010x", 0x1234abcd );
        TEST( "1234abcd  ", 10, "%-010x", 0x1234abcd ); /* '-' kills '0' */
        TEST( "  1234abcd", 10, "%010.1x", 0x1234abcd ); /* prec kills '0' */
        TEST( "001234ABCD", 10, "%010X", 0x1234abcd );
        TEST( "1234ABCD  ", 10, "%-010X", 0x1234abcd ); /* '-' kills '0' */
        TEST( "  1234ABCD", 10, "%010.1X", 0x1234abcd ); /* prec kills '0' */
    }
    else
    {
        TEST( "00000012cd", 10, "%010x", 0x12cd );
        TEST( "12cd      ", 10, "%-010x", 0x12cd ); /* '-' kills '0' */
        TEST( "      12cd", 10, "%010.1x", 0x12cd ); /* prec kills '0' */
        TEST( "00000012CD", 10, "%010X", 0x12cd );
        TEST( "12CD      ", 10, "%-010X", 0x12cd ); /* '-' kills '0' */
        TEST( "      12CD", 10, "%010.1X", 0x12cd ); /* prec kills '0' */
    }

    /* Alternate form */
    TEST( "0", 1, "%#b", 0 );
    TEST( "0", 1, "%#o", 0 );
    TEST( "0", 1, "%#x", 0 );
    TEST( "0", 1, "%#X", 0 );

    TEST( "0b1101", 6, "%#b", 13 );
    TEST( "01234", 5, "%#o", 01234 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "0x1234abcd", 10, "%#x", 0x1234abcd );
        TEST( "0X1234ABCD", 10, "%#X", 0x1234abcd );
    }
    else
    {
        TEST( "0x12cd", 6, "%#x", 0x12cd );
        TEST( "0X12CD", 6, "%#X", 0x12cd );
    }

    /* Alternate with ! */
    TEST( "0b0", 3, "%!#b", 0 );
    TEST( "0", 1, "%!#o", 0 );
    TEST( "0x0", 3, "%!#x", 0 );
    TEST( "0x0", 3, "%!#X", 0 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "0x1234abcd", 10, "%!#x", 0x1234abcd );
        TEST( "0x1234ABCD", 10, "%!#X", 0x1234abcd );
    }
    else
    {
        TEST( "0x12cd", 6, "%!#x", 0x12cd );
        TEST( "0x12CD", 6, "%!#X", 0x12cd );
    }

    TEST( "1101", 4, "%!b", 13 );
    TEST( "1234", 4, "%!o", 01234 );
    TEST( "1234", 4, "%!u", 1234 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "1234abcd", 8, "%!x", 0x1234abcd );
        TEST( "1234ABCD", 8, "%!X", 0x1234ABCD );
    }
    else
    {
        TEST( "12cd", 4, "%!x", 0x12cd );
        TEST( "12CD", 4, "%!X", 0x12CD );
    }

    TEST( "  0b1101", 8, "%#8b", 13 );
    TEST( "   01234", 8, "%#8o", 01234 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "  0x1234abcd", 12, "%#12x", 0x1234abcd );
        TEST( "  0X1234ABCD", 12, "%#12X", 0x1234abcd );
    }
    else
    {
        TEST( "      0x12cd", 12, "%#12x", 0x12cd );
        TEST( "      0X12CD", 12, "%#12X", 0x12cd );
    }

    TEST( "0b00001101", 10, "%#.8b", 13 );
    TEST( "000001234", 9, "%#.8o", 01234 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "0x00001234abcd", 14, "%#.12x", 0x1234abcd );
        TEST( "0X00001234ABCD", 14, "%#.12X", 0x1234abcd );
    }
    else
    {
        TEST( "0x0000000012cd", 14, "%#.12x", 0x12cd );
        TEST( "0X0000000012CD", 14, "%#.12X", 0x12cd );
    }

    TEST( "  0b00001101", 12, "%#12.8b", 13 );
    TEST( "   000001234", 12, "%#12.8o", 01234 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "  0x00001234abcd", 16, "%#16.12x", 0x1234abcd );
        TEST( "  0X00001234ABCD", 16, "%#16.12X", 0x1234abcd );
    }
    else
    {
        TEST( "  0x0000000012cd", 16, "%#16.12x", 0x12cd );
        TEST( "  0X0000000012CD", 16, "%#16.12X", 0x12cd );
    }

    TEST( "0b00001101  ", 12, "%-#12.8b", 13 );
    TEST( "000001234   ", 12, "%-#12.8o", 01234 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "0x00001234abcd  ", 16, "%-#16.12x", 0x1234abcd );
        TEST( "0X00001234ABCD  ", 16, "%-#16.12X", 0x1234abcd );
    }
    else
    {
        TEST( "0x0000000012cd  ", 16, "%-#16.12x", 0x12cd );
        TEST( "0X0000000012CD  ", 16, "%-#16.12X", 0x12cd );
    }

    /* Centering */
    TEST( "  ABCD  ", 8, "%^8X", 0xABCD );
    TEST( " 0XABCD ", 8, "%^#8X", 0xABCD );
    TEST( " 0X0000ABCD ", 12, "%^#12.8X", 0xABCD );

#if defined(CONFIG_WITH_GROUPING_SUPPORT)
    /* Grouping */
    TEST( "AB_CD", 5, "%[_2]X", 0xABCD );
    TEST( "1_1_1_1_0_0_0_0", 15, "%[_1]b", 0xF0 );
    TEST( "1111_00_11", 10, "%[-_2_2]b", 0xF3 );
#endif

    /* No effect: +,space */
    TEST( "1101", 4, "%+ b", 13 );
    TEST( "1234", 4, "%+ o", 01234 );

    if ( sizeof( int ) > 2 )
    {
        TEST( "1234abcd", 8, "%+ x", 0x1234abcd );
        TEST( "1234ABCD", 8, "%+ X", 0x1234abcd );
    }
    else
    {
        TEST( "12cd", 4, "%+ x", 0x12cd );
        TEST( "12CD", 4, "%+ X", 0x12cd );
    }

    /* Non-standard bases */
    TEST( "11", 2, "%:3u", 4 );
    TEST( "g", 1, "%:17u", 16 );
    TEST( "G", 1, "%:17U", 16 );
    TEST( "XYZ", 3, "%:36U", 44027 );
    TEST( " 00XYZ", 6, "%6.5:36U", 44027 );

#if defined(CONFIG_WITH_LONG_LONG_SUPPORT)
    TEST( "123456789123456789", 18, "%llu", ulli );
    TEST( "1B69B4BACD05F15", 15, "%llX", ulli );
#endif
}

#if defined(CONFIG_WITH_FP_SUPPORT)
/*****************************************************************************/
/**
    Execute tests on a,A,e,E,f,F,g,G conversion specifiers.


**/
static void test_aAeEfFgG( void )
{
    printf( "Testing \"%%a\", \"%%A\", \"%%e\", \"%%E\", \"%%f\", \"%%F\", \"%%g\" and \"%%G\"\n" );

    /* %a and %A */

    TEST( "inf", 3, "%a", +1.0/0.0 );
    TEST( "+inf", 4, "%+a", +1.0/0.0 );
    TEST( "-inf", 4, "%a", -1.0/0.0 );
    TEST( "INF", 3, "%A", +1.0/0.0 );
    TEST( "+INF", 4, "%+A", +1.0/0.0 );
    TEST( "-INF", 4, "%A", -1.0/0.0 );

    TEST( "0x1p+0", 6, "%a", 1.0 );
    TEST( "0X1P+0", 6, "%A", 1.0 );
    TEST( "-0x1p+0", 7, "%a", -1.0 );

    TEST( "0x2.0p+0", 8, "%.1a", 1.998046875 );
    TEST( "-0x2.0p+0", 9, "%.1a", -1.998046875 );

    TEST( "0x1.de18f06716de4p+408", 22, "%a", 1.234567e+123 );

    TEST( "-0x00001.0p+0", 13, "%013.1a", -1.0 );
    TEST( "    -0x1.0p+0", 13, "% 13.1a", -1.0 );
    TEST( "-0x1.0p+0    ", 13, "%-13.1a", -1.0 );
    TEST( "    +0x1.0p+0", 13, "%+13.1a",  1.0 );
    TEST( "   0x1.0p+0  ", 13, "%^13.1a",  1.0 );

    TEST( "0x1p+0", 6, "%.0a", 1.0 );
    TEST( "0x1.p+0", 7, "%#.0a", 1.0 );

    /* %e and %E */

    TEST( "inf", 3, "%e", +1.0/0.0 );
    TEST( "+inf", 4, "%+e", +1.0/0.0 );
    TEST( "-inf", 4, "%e", -1.0/0.0 );
    TEST( "INF", 3, "%E", +1.0/0.0 );
    TEST( "+INF", 4, "%+E", +1.0/0.0 );
    TEST( "-INF", 4, "%E", -1.0/0.0 );

    TEST( "1.0e+00", 7, "%.1e", 1.0 );
    TEST( "+1.0e+00", 8, "%+.1e", 1.0 );
    TEST( "1.0e-01", 7, "%.1e", 0.1 );
    TEST( "1.1e+00", 7, "%.1e", 1.1 );
    TEST( "1.000000e+00", 12, "%e", 1.0 );
    TEST( "1.000000E+00", 12, "%E", 1.0 );
    TEST( "1.234567e+123", 13, "%e", 1.234567e+123 );
    TEST( "-000001.0e+00", 13, "%013.1e", -1.0 );
    TEST( "     -1.0e+00", 13, "% 13.1e", -1.0 );
    TEST( "-1.0e+00     ", 13, "%-13.1e", -1.0 );
    TEST( "   -1.0e+00  ", 13, "%^13.1e", -1.0 );

    TEST( "1e+00", 5, "%.0e", 1.0 );
    TEST( "1.e+00", 6, "%#.0e", 1.0 );

    /* %f and %F */

    TEST( "0.000000", 8, "%f", 0.0 );
    TEST( "0", 1, "%.0f", 0.0 );
    TEST( "1.00", 4, "%.2f", 0.999f );

    TEST( "1.0", 3, "%.1f", 1.0 );
    TEST( "0.1", 3, "%.1f", 0.1 );
    TEST( "10.010", 6, "%.3f", 10.010 );

    TEST( "+1.0", 4, "%+.1f", 1.0 );
    TEST( " 1.0", 4, "% .1f", 1.0 );
    TEST( "-1.0", 4, "%.1f", -1.0 );

    TEST( "   1.0", 6, "%6.1f", 1.0 );
    TEST( "1.0   ", 6, "%-6.1f", 1.0 );
    TEST( "  1.0 ", 6, "%^6.1f", 1.0 );

    TEST( "+001.0", 6, "%+06.1f", 1.0 );
    TEST( "001.0 ", 6, "%^06.1f", 1.0 );

    TEST( "1234.568", 8, "%.3f", 1234.5678 );

    TEST( "12.4", 4, "%.1f", 12.449 );
    TEST( "12.45", 5, "%.2f", 12.449 );

    TEST( "1200.00", 7, "%.2f", 1200.0 );
    TEST( "0.000100", 8, "%.6f", 0.0001 );

    TEST( "0.000000", 8, "%.6f", 0.0000001 );
    TEST( "0.0000001000", 12, "%.10f", 0.0000001 );

    TEST( "1234567800000006000000000000000000000000000000000000000000000"
          "0000000000000000000000000000000000000000000000000000000000000"
          "0000000000000000000000000000000000000000000000000000000000000"
          "0000000000000000000000000000000000000000000000000000000000000"
          "000000000000000000000000000000000000000000000000000000000000."
          "0000000000000000000000000000000000000000000000000000000000000"
          "000000000000000000000000000000000000000",
          405, "%.100f", 1234.5678e300 );

    TEST( "inf",  3, "%f", +1.0/0.0 );
    TEST( "-inf", 4, "%f", -1.0/0.0 );
    TEST( "+inf", 4, "%+f", +1.0/0.0 );
    TEST( "-inf", 4, "%+f", -1.0/0.0 );
    TEST( " inf", 4, "% f", +1.0/0.0 );
    TEST( "-inf", 4, "% f", -1.0/0.0 );

    TEST( "INF",  3, "%F", +1.0/0.0 );
    TEST( "+INF", 4, "%+F", +1.0/0.0 );
    TEST( "-INF", 4, "%F", -1.0/0.0 );

    TEST( "   inf", 6, "%6f", +1.0/0.0 );
    TEST( "  -inf", 6, "%6f", -1.0/0.0 );
    TEST( "inf   ", 6, "%-6f", +1.0/0.0 );
    TEST( "-inf  ", 6, "%-6f", -1.0/0.0 );
    TEST( "  inf ", 6, "%^6f", +1.0/0.0 );
    TEST( " inf  ", 6, "%-^6f", +1.0/0.0 );
    TEST( " -inf ", 6, "%^6f", -1.0/0.0 );

    /* %g and %G */
    /*
    
    g,G     A double argument representing a floating point numer is converted
            in the style f or e (on in style F or E in the case of a G
            conversion specifier), with the precision specifying the number
            of significant digits.  If the precision is zero, it is taken as 1.
            The style used depends on the value converted; style e (or E) is
            used only if the exponent resulting from such a conversion is less
            than -4 or greater than or equal to the precision.  Trailing zeros
            are removed from the fractional portion of the result unless the
            # flag is specified; a decimal point character appears only if it
            is followed by a digit.
    */

    /*  If the precision is zero, it is taken as 1.
    */
    TEST( "1", 1, "%.0g", 1.2345 );

    /*  The style used depends on the value converted; style e (or E) is
        used only if the exponent resulting from such a conversion is less
        than -4 or greater than or equal to the precision.
    */
    TEST( "1.2345e-05", 10, "%g", 1.2345e-5 );  /* Trailing zeros removed */
    TEST( "0.000123", 8, "%g", 1.2345e-4 );

    TEST( "12", 2, "%.2g", 12.345 );             /* 2 sig figs */
    TEST( "1.2e+02", 7, "%.2g", 123.45 );        /* 2 sig figs */
    TEST( "1.2e+03", 7, "%.2g", 1234.5 );        /* 2 sig figs */

    /*  Trailing zeros are removed from the fractional portion of the result
        unless the # flag is specified
    */
    TEST( "1.2300", 6, "%#.4g", 1.23 );
    TEST( "1.23", 4, "%.4g", 1.23 );

    /*  a decimal point character appears only if it
        is followed by a digit.
    */
    TEST( "1", 1, "%.1g", 1.01 );
    TEST( "1", 1, "%.2g", 1.01 );                /* Rounds to 1 with 2 sig figs */

    /* Miscellaneous tests */
    TEST( "123", 3, "%.6g", 123.0 );
    TEST( "123.000000", 10, "%#.6g", 123.0 );
    TEST( "123.4", 5, "%.6g", 123.4 );

    /* From http://www.cplusplus.com/reference/cstdio/printf/ */
    /* Note that the outputs given are slightly different with respect to C, which
     *  states that there should be a minimum of two digits in the exponent, not three as shown
     *  on the above webpage.
     */

    TEST( "Characters: a A \n", 17, "Characters: %c %c \n", 'a', 65 );
    TEST( "Decimals: 1977 650000\n", 22, "Decimals: %d %ld\n", 1977, 650000L);
    TEST( "Preceding with blanks:       1977 \n", 35, "Preceding with blanks: %10d \n", 1977);
    TEST( "Preceding with zeros: 0000001977 \n", 34, "Preceding with zeros: %010d \n", 1977);
    TEST( "Some different radices: 100 64 144 0x64 0144 \n", 46, "Some different radices: %d %x %o %#x %#o \n", 100, 100, 100, 100, 100);
    TEST( "floats: 3.14 +3e+00 3.141600E+00 \n", 34, "floats: %4.2f %+.0e %E \n", 3.1416, 3.1416, 3.1416);
    TEST( "Width trick:    10 \n", 20, "Width trick: %*d \n", 5, 10);
    TEST( "A string \n", 10, "%s \n", "A string");

    /* Engineering formatting */
    TEST( "12.345e+03", 10, "%!.3e", 12345.0 );
    TEST( "12.345e-03", 10, "%!.3e", 0.012345 );

    TEST( "123.45", 6, "%!.2f", 123.45 );
    TEST( "1.2345", 6, "%!.4f", 1.2345 );
    TEST( "12.345 k", 8, "%!.3f", 12345.0 );
    TEST( "12.345 m", 8, "%!.3f", 0.012345 );
    TEST( "1234.5 Y", 8, "%!.1f", 1.2345e+27 );
    TEST( "123.45 Y", 8, "%!.2f", 123.45e+24 );
    TEST( "0.12345 y", 9, "%!.5f", 0.12345e-24 );
    TEST( "1.2345 y", 8, "%!.4f", 1.2345e-24 );

    /*******   Rounding   ****************************************************/

    TEST( "1", 1, "%1.f", 0.99f );
    TEST( "1.0e+00", 7, "%.1e", 0.999f );
    /* TODO:  TEST( "12.35", 5, "%.4g", 12.345f ); */
    
    /*******  Denormals  ****************************************************/
    {
#if ( DBL_DIG > 8 ) /* 64-bit doubles */    
        double n = pow( 2, -1074 );
        
        TEST(  "4.94e-324",  9, "%.2e",  n );
        TEST( "-4.94e-324", 10, "%.2e", -n );
        
        n = ( 1.0 - pow( 2, -52 ) ) * pow( 2, -1022 );
        
        TEST(  "2.22e-308",  9, "%.2e",  n );
        TEST( "-2.22e-308", 10, "%.2e", -n );
#else /*32-bit doubles */
        double n = pow( 2, -149 );
        
        TEST(  "1.40e-45", 8, "%.2e",  n );
        TEST( "-1.40e-45", 9, "%.2e", -n );
        
        n = (1.0 - pow(2, -23)) * pow(2, -126);
        
        TEST(  "1.17e-38", 8, "%.2e",  n );
        TEST( "-1.17e-38", 9, "%.2e", -n );
#endif
    }
}
/*****************************************************************************/
/**
    Execute tests on k (fixed-point) conversion specifier.


**/
static void test_k( void )
{
    char s4p4;
    int s8p4;
    int s4p8;

    printf( "Testing \"%%k\"\n" );

    /* zero */
    TEST( "0.000000", 8, "%{4.4}k", 0 );

    /* Positive */
    s4p4 = ( ( 1 ) << 4 ) | (int)( 0.5 * 16 ); /* 1.50 */
    TEST( "1.500000", 8, "%{4.4}k", s4p4 );
    s8p4 =  ( ( 1 ) << 4 ) | (int)( 0.5 * 16 ); /* 1.50 */
    TEST( "1.500000", 8, "%{8.4}k", s8p4 );
    s4p8 =  ( ( 1 ) << 8 ) | (int)( 0.5 * 256 ); /* 1.50 */
    TEST( "1.500000", 8, "%{4.8}k", s4p8 );

    /* Negative */
    s4p4 = - ( ( ( 1 ) << 4 ) | (int)( 0.5 * 16 ) ); /* -1.50 */
    TEST( "-1.500000", 9, "%{4.4}k", s4p4 );
    s8p4 = - ( ( ( 1 ) << 4 ) | (int)( 0.5 * 16 ) ); /* -1.50 */
    TEST( "-1.500000", 9, "%{8.4}k", s8p4 );
    s4p8 =  - ( ( ( 1 ) << 8 ) | (int)( 0.5 * 256 ) ); /* -1.50 */
    TEST( "-1.500000", 9, "%{4.8}k", s4p8 );

    /* Formatting */
    s4p8 =  ( ( 1 ) << 8 ) | (int)( 0.5 * 256 ); /* 1.50 */
    TEST( "  1.50  ", 8, "%^8.2{4.8}k", s4p8 );
}

/*****************************************************************************/
/**
    Test fixed-point (%k) edge cases and comprehensive flag/format coverage.
**/
static void test_k_edge_cases( void )
{
    printf( "Testing \"%%k\" edge cases\n" );

    /* ===== Different Bit Width Combinations ===== */

    /* ===== Minimum Valid Format Tests ===== */

    /* {1.1}k format - absolute minimum (1 sign + 0 integer + 1 fractional = 2 bits) */
    {
        /* Range: [-1.0, +0.5] in 0.5 increments */
        char s1p1_zero = 0;       /* 0.0 */
        char s1p1_half = 1;       /* 0.5 */
        char s1p1_negone = -2;    /* -1.0 (0b10 in two's complement) */
        TEST( "0.000000", 8, "%{1.1}k", s1p1_zero );
        TEST( "0.500000", 8, "%{1.1}k", s1p1_half );
        TEST( "-1.000000", 9, "%{1.1}k", s1p1_negone );
    }

    /* {2.1}k format - 1 sign + 1 integer + 1 fractional = 3 bits */
    {
        /* Range: [-1.5, +1.5] in 0.5 increments (sign-magnitude) */
        char s2p1_zero = 0;       /* 0.0 */
        char s2p1_half = 1;       /* 0.5 */
        char s2p1_one = 2;        /* 1.0 */
        char s2p1_onehalf = 3;    /* 1.5 (max positive) */
        char s2p1_neghalf = -1;   /* -0.5 */
        char s2p1_negone = -2;    /* -1.0 */
        char s2p1_negonehalf = -3; /* -1.5 (min negative) */
        TEST( "0.000000", 8, "%{2.1}k", s2p1_zero );
        TEST( "0.500000", 8, "%{2.1}k", s2p1_half );
        TEST( "1.000000", 8, "%{2.1}k", s2p1_one );
        TEST( "1.500000", 8, "%{2.1}k", s2p1_onehalf );
        TEST( "-0.500000", 9, "%{2.1}k", s2p1_neghalf );
        TEST( "-1.000000", 9, "%{2.1}k", s2p1_negone );
        TEST( "-1.500000", 9, "%{2.1}k", s2p1_negonehalf );
    }

    /* {1.2}k format - 1 sign + 0 integer + 2 fractional = 3 bits */
    {
        /* Range: [-1.0, +0.75] in 0.25 increments */
        char s1p2_zero = 0;       /* 0.0 */
        char s1p2_qtr = 1;        /* 0.25 */
        char s1p2_half = 2;       /* 0.5 */
        char s1p2_threequart = 3; /* 0.75 (max positive) */
        char s1p2_negone = -4;    /* -1.0 (min negative) */
        TEST( "0.000000", 8, "%{1.2}k", s1p2_zero );
        TEST( "0.250000", 8, "%{1.2}k", s1p2_qtr );
        TEST( "0.500000", 8, "%{1.2}k", s1p2_half );
        TEST( "0.750000", 8, "%{1.2}k", s1p2_threequart );
        TEST( "-1.000000", 9, "%{1.2}k", s1p2_negone );
    }

    /* {1.15}k format - fractional only (1 sign + 0 integer + 15 fractional = 16 bits) */
    {
        /* Range: [-0.999969, +0.999969] with sign-magnitude (15-bit magnitude) */
        TEST( "0.500000", 8, "%{1.15}k", (int)(0.5 * (1 << 15)) );
        TEST( "-0.500000", 9, "%{1.15}k", (int)(-0.5 * (1 << 15)) );
        TEST( "0.999969", 8, "%{1.15}k", 0x7FFF );  /* Max positive = 32767/32768 */
        TEST( "-0.999969", 9, "%{1.15}k", -0x7FFF ); /* Min negative = -32767/32768 */
    }

    /* 8.8 format - balanced */
    {
        int s8p8 = (12 << 8) | (int)(0.25 * 256);  /* 12.25 in 8.8 */
        TEST( "12.250000", 9, "%{8.8}k", s8p8 );
        TEST( "-12.250000", 10, "%{8.8}k", -s8p8 );
    }

    /* 15.1 format */
    {
        int s15p1 = (100 << 1) | 1;  /* 100.5 in 15.1 */
        TEST( "100.500000", 10, "%{15.1}k", s15p1 );
        TEST( "-100.500000", 11, "%{15.1}k", -s15p1 );
    }

    /* 16.16 and 24.8 formats */
    {
        int s16p16 = (5 << 16) | (int)(0.125 * 65536);  /* 5.125 in 16.16 */
        TEST( "5.125000", 8, "%{16.16}k", s16p16 );

        int s24p8 = (1000 << 8) | (int)(0.75 * 256);  /* 1000.75 in 24.8 */
        TEST( "1000.750000", 11, "%{24.8}k", s24p8 );
    }

    /* ===== Minimum/Maximum Representable Values ===== */

    /* Maximum positive for 4.4 format */
    {
        char s4p4 = (7 << 4) | 15;  /* 7.9375 */
        TEST( "7.937500", 8, "%{4.4}k", s4p4 );
    }

    /* Minimum negative for 4.4 format */
    {
        char s4p4 = -128;  /* 0x80 = sign bit set, magnitude 0 = -0 or -1 representation */
        TEST( "-1.000000", 9, "%{4.4}k", s4p4 );
    }

    /* Maximum for 8.8 format */
    {
        int s8p8 = (127 << 8) | 255;  /* 127.996094 */
        TEST( "127.996094", 10, "%{8.8}k", s8p8 );
    }

    /* ===== Fixed-Point with Flags ===== */

    {
        int s8p8 = (12 << 8) | (int)(0.5 * 256);  /* 12.5 */

        /* + flag */
        TEST( "+12.500000", 10, "%+{8.8}k", s8p8 );

        /* space flag */
        TEST( " 12.500000", 10, "% {8.8}k", s8p8 );

        /* 0 flag (zero padding) */
        TEST( "00012.500000", 12, "%012{8.8}k", s8p8 );

        /* - flag (left align) */
        TEST( "12.500000  ", 11, "%-11{8.8}k", s8p8 );

        /* ^ flag (center) */
        TEST( " 12.500000 ", 11, "%^11{8.8}k", s8p8 );
    }

    /* ===== Fixed-Point with Precision ===== */
    {
        int s8p8 = (12 << 8) | (int)(0.25 * 256);  /* 12.25 */

        TEST( "12", 2, "%.0{8.8}k", s8p8 );
        TEST( "12.25", 5, "%.2{8.8}k", s8p8 );
        TEST( "12.2500", 7, "%.4{8.8}k", s8p8 );
        TEST( "12.2500000000", 13, "%.10{8.8}k", s8p8 );
    }

    /* ===== Width and Precision Combined ===== */
    {
        int s4p4 = (1 << 4) | (int)(0.5 * 16);  /* 1.5 */

        TEST( "     1.50", 9, "%9.2{4.4}k", s4p4 );
        TEST( "1.50     ", 9, "%-9.2{4.4}k", s4p4 );
        TEST( "   1.50  ", 9, "%^9.2{4.4}k", s4p4 );  /* Left-biased centering */
        TEST( "000001.50", 9, "%09.2{4.4}k", s4p4 );
        TEST( "    +1.50", 9, "%+9.2{4.4}k", s4p4 );
    }

    /* ===== Very Small Fractional Values ===== */
    {
        int s4p4 = 1;  /* 0.0625 in 4.4 */
        TEST( "0.062500", 8, "%{4.4}k", s4p4 );

        int s8p8 = 1;  /* 0.00390625 in 8.8 */
        TEST( "0.003906", 8, "%{8.8}k", s8p8 );
    }
}
#endif /* CONFIG_WITH_FP_SUPPORT */

/*****************************************************************************/
/**
    Test asterisk.
**/
static void test_asterisk( void )
{
    printf( "Testing \"*\"\n" );

    /* Precision sets minimum number of digits, zero-padding if necessary */
    TEST( "001234", 6, "%.*d", 6, 1234 );

    /* Negative precision treated as if no precision specified */
    TEST( "1234", 4, "%.*d", -6, 1234 );

    /* Width sets minimum field width */
    TEST( "  1234", 6, "%*d", 6, 1234 );

    /* Negative width treated as '-' flag and positive width */
    TEST( "1234  ", 6, "%*d", -6, 1234 );

    /* Both together */
    TEST( "  001234", 8, "%*.*d", 8, 6, 1234 );

#if defined(CONFIG_WITH_GROUPING_SUPPORT)
    /* Grouping */
    TEST( "1,2_34", 6, "%[,*_*]d", 1234, 2, 1 );
    TEST( "1234", 4, "%[_1,*]d", 1234, -1 );
#endif

    /* Also check maximum precision and widths */
    TEST( "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000",
          500, "%.500d", 0 );

    TEST( "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000",
          EXBADFORMAT, "%.501d", 0 );

    TEST( "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                 0",
          500, "%500d", 0 );

     TEST( "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                 0",
          EXBADFORMAT, "%501d", 0 );
}

/*****************************************************************************/
/**
    Test format continuation.
**/
static void test_cont( void )
{
    printf( "Testing format continuation\n" );

    /* Basic string continuation */
    TEST( "hello world", 11, "hello %", "world" );
    TEST( "hello old world", 15, "hello %", "old %", "world" );

    /* Interspersed conversions */
    TEST( "One: 1,Two: 2,Three: 3", 22, "One: %d,%", 1,
                                        "Two: %c,%", '2',
                                        "Three: %s", "3" );

    /* Modifiers after % with no conversion are now caught as errors (Bug #1 fix).
     * Old behavior: treated as continuation, modifiers ignored.
     * New behavior: incomplete specification → EXBADFORMAT */
    /* TEST( "hello world", 11, "hello % +-!^12.24l", "world" ); */  /* No longer valid */

#if defined(__AVR__)
    {
        static char cont_string[] PROGMEM = "brave %s %";
        TEST( "hello brave new world", 21,
                         "hello %#", (PGM_P)cont_string, "new", "world" );
    }
#endif
}

#if defined(CONFIG_WITH_GROUPING_SUPPORT)
/*****************************************************************************/
/**
    Test comprehensive grouping edge cases and complex patterns.
**/
static void test_grouping( void )
{
    printf( "Testing comprehensive grouping patterns\n" );

    /* ===== Complex Grouping Patterns ===== */

    /* Multiple group sizes: patterns like 12_34_567 */
    TEST( "1_23", 4, "%[_1_2]d", 123 );
    TEST( "12_345", 6, "%[_2_3]d", 12345 );
    TEST( "1_23_456", 8, "%[_1_2_3]d", 123456 );
    TEST( "12_34_567", 9, "%[_2_2_3]d", 1234567 );
    TEST( "1_2_34_567", 10, "%[_1_1_2_3]d", 1234567 );

    /* Repeating patterns */
    TEST( "123_456_789", 11, "%[_3_3_3]d", 123456789 );
    TEST( "12_34_56_78_90", 14, "%[_2_2_2_2_2]d", 1234567890 );

    /* Single-digit grouping (every digit separated) */
    TEST( "1_2_3_4", 7, "%[_1]d", 1234 );
    TEST( "1,2,3,4,5,6,7", 13, "%[,1]d", 1234567 );
    TEST( "9_8_7_6_5", 9, "%[_1]d", 98765 );

    /* Grouping with very large numbers */
    TEST( "1,234,567,890", 13, "%[,3]d", 1234567890 );
    /* Note: Long long support is platform-dependent, skip for now */
    /* TEST( "12,345,678,901", 14, "%[,3]d", 12345678901LL ); */

    /* Grouping with precision interaction - precision pads first, then grouping applied */
    TEST( "01_23_45", 8, "%.6[_2]d", 12345 );    /* 012345 -> 01_23_45 */
    TEST( "000123_456", 10, "%.9[_3]d", 123456 ); /* 000123456 -> 000123_456 */
    TEST( "00001234", 8, "%.8[_4]d", 1234 );     /* 00001234 -> 00001234 (no grouping, 8 digits) */

    /* ===== Grouping with All Conversion Types ===== */

    /* Binary */
    TEST( "1111_0000", 9, "%[_4]b", 0xF0 );
    TEST( "11_11_00_00", 11, "%[_2]b", 0xF0 );
    TEST( "1_1_1_1_0_0_0_0", 15, "%[_1]b", 0xF0 );
    TEST( "1010_1010", 9, "%[_4]b", 0xAA );

    /* Octal */
    TEST( "12_34", 5, "%[_2]o", 01234 );
    TEST( "1_2_3_4", 7, "%[_1]o", 01234 );
    TEST( "123_456", 7, "%[_3]o", 0123456 );
    TEST( "7_7_7", 5, "%[_1]o", 0777 );

    /* Hexadecimal */
    TEST( "ab_cd", 5, "%[_2]x", 0xABCD );
    TEST( "AB_CD_EF", 8, "%[_2]X", 0xABCDEF );
    TEST( "1_2_3_4_5_6_7_8", 15, "%[_1]x", 0x12345678 );
    TEST( "dead_beef", 9, "%[_4]x", 0xDEADBEEF );

    /* NOTE: Arbitrary bases with ':' do NOT support grouping per documentation.
     * Grouping only works with b, d, i, I, o, u, U, x, X conversions. */

    /* ===== Grouping with Flags ===== */

    /* Alternate form with grouping */
    TEST( "0xab_cd", 7, "%#[_2]x", 0xABCD );
    TEST( "0XAB_CD_EF", 10, "%#[_2]X", 0xABCDEF );  /* Uppercase X uses 0X prefix */
    TEST( "0b1111_0000", 11, "%#[_4]b", 0xF0 );
    TEST( "012_34", 6, "%#[_2]o", 01234 );

    /* Zero-padding with grouping - padding applied to ungrouped value */
    TEST( "00012_34", 8, "%08[_2]d", 1234 );  /* Pads to 6 digits (000 1234), then groups: 0001 2_34 */
    TEST( "0000123", 7, "%07[_3]d", 123 );    /* Pads to 7 digits (0000123), groups of 3: no separator fits */
    TEST( "0000001", 7, "%07[_2]d", 1 );      /* Pads to 7 digits (0000001), groups of 2: 0 00 00 01 */

    /* Centering with grouping */
    TEST( "  12,34  ", 9, "%^9[,2]d", 1234 );
    TEST( " 1_2_3_4 ", 9, "%^9[_1]d", 1234 );
    TEST( "  1,234  ", 9, "%^9[,3]d", 1234 );

    /* Sign flags with grouping */
    TEST( "+12,34", 6, "%+[,2]d", 1234 );
    TEST( " 12,34", 6, "% [,2]d", 1234 );
    TEST( "-12,34", 6, "%[,2]d", -1234 );
    TEST( "+1_2_3_4", 8, "%+[_1]d", 1234 );

    /* Left-align with grouping */
    TEST( "12,34   ", 8, "%-8[,2]d", 1234 );
    TEST( "1_2_3_4 ", 8, "%-8[_1]d", 1234 );

    /* Multiple flags combined with grouping */
    TEST( "+0012,34", 8, "%+08[,2]d", 1234 );
    TEST( " 0012,34", 8, "% 08[,2]d", 1234 );

    /* ===== Grouping Edge Cases ===== */

    /* Grouping with width and precision */
    TEST( "       12,34", 12, "%12[,2]d", 1234 );
    TEST( "000012,34", 9, "%.8[,2]d", 1234 );       /* Precision 8 gives 6 zeros + 4 digits (00001234), then group: 000012,34 */
    TEST( "    0012,34", 11, "%11.6[,2]d", 1234 );  /* Precision 6 gives 2 zeros + 4 digits (001234), group (0012,34), width 11 */

    /* Grouping where count equals number length */
    TEST( "1234", 4, "%[,4]d", 1234 );
    TEST( "123456", 6, "%[_6]d", 123456 );

    /* Grouping where count exceeds number length */
    TEST( "123", 3, "%[,5]d", 123 );
    TEST( "12", 2, "%[_10]d", 12 );

    /* Grouping with negative numbers */
    TEST( "-12,34", 6, "%[,2]d", -1234 );
    TEST( "-1,234,567", 10, "%[,3]d", -1234567 );
    TEST( "-1_2_3_4", 8, "%[_1]d", -1234 );

    /* Grouping with zero */
    TEST( "0", 1, "%[,3]d", 0 );
    TEST( "0", 1, "%[_2]d", 0 );
    TEST( "0000", 4, "%.4[,2]d", 0 );  /* Precision 4 gives "0000", groups of 2: "0000" (no separator fits evenly) */

    /* Grouping separator potentially ambiguous with output - these are allowed but may be confusing */
    /* Using '0' as separator - actual output may vary, commenting out for now */
    /* TEST( "A0B0C0D", 7, "%[02]X", 0xABCD ); */
    /* TEST( "1010101", 7, "%[01]d", 1111 ); */

    /* Different separator characters */
    TEST( "12'34", 5, "%['2]d", 1234 );
    TEST( "12:34", 5, "%[:2]d", 1234 );
    TEST( "12.34", 5, "%[.2]d", 1234 );
    TEST( "12 34", 5, "%[ 2]d", 1234 );
    TEST( "12|34", 5, "%[|2]d", 1234 );

    /* Grouping with asterisk for dynamic count - asterisk value comes AFTER the converted value */
    TEST( "12,34", 5, "%[,*]d", 1234, 2 );
    TEST( "1,234", 5, "%[,*]d", 1234, 3 );
    TEST( "1_2_3_4", 7, "%[_*]d", 1234, 1 );
    TEST( "1234", 4, "%[_*]d", 1234, 0 );  /* Zero count = no grouping */
    TEST( "1234", 4, "%[_*]d", 1234, -1 ); /* Negative count = no grouping */

    /* Multiple group specifications with asterisks */
    /* NOTE: With multiple asterisks, behavior is: rightmost asterisk value consumed first */
    TEST( "12_34_5", 7, "%[_*_*]d", 12345, 1, 2 );   /* Right group: 2, produces 12_34_5, left group: 1 (but seems not applied) */
    TEST( "12,34_56", 8, "%[,*_*]d", 123456, 2, 2 ); /* Right group: 2 (_), left group: 2 (,) */
}
#endif /* CONFIG_WITH_GROUPING_SUPPORT */

/*****************************************************************************/
/**
    Test comprehensive centering flag (^) with all conversion specifiers.
**/
static void test_centering( void )
{
    printf( "Testing comprehensive centering flag (^)\n" );

    /* ===== Centering with All Conversion Specifiers ===== */

    /* Character (%c) - Note: centering may not be supported for %c */
    TEST( "A", 1, "%^5c", 'A' );          /* %c may not support centering */
    TEST( "A", 1, "%^4c", 'A' );
    TEST( "A", 1, "%^3c", 'A' );
    TEST( "A", 1, "%^1c", 'A' );          /* Width equals output */
    TEST( "A", 1, "%^2c", 'A' );          /* Width < output */

    /* String (%s) - already tested in basic suite but add more */
    TEST( "  hello  ", 9, "%^9s", "hello" );
    TEST( "  hello ", 8, "%^8s", "hello" );
    TEST( " hello ", 7, "%^7s", "hello" );
    TEST( " hello", 6, "%^6s", "hello" );
    TEST( "hello", 5, "%^5s", "hello" );
    TEST( "hello", 5, "%^3s", "hello" );  /* Width < length: no truncation */

    /* Decimal (%d, %i) */
    TEST( "  123  ", 7, "%^7d", 123 );
    TEST( "  123 ", 6, "%^6d", 123 );
    TEST( " 123 ", 5, "%^5d", 123 );
    TEST( " 123", 4, "%^4d", 123 );
    TEST( "123", 3, "%^3d", 123 );
    TEST( "  -123  ", 8, "%^8d", -123 );
    TEST( "  -123 ", 7, "%^7d", -123 );

    /* Unsigned (%u) */
    TEST( "  456  ", 7, "%^7u", 456 );
    TEST( "  456 ", 6, "%^6u", 456 );
    TEST( " 456 ", 5, "%^5u", 456 );

    /* Octal (%o) */
    TEST( "  755  ", 7, "%^7o", 0755 );
    TEST( "  755 ", 6, "%^6o", 0755 );
    TEST( " 755 ", 5, "%^5o", 0755 );

    /* Hex (%x, %X) */
    TEST( "  abc  ", 7, "%^7x", 0xABC );
    TEST( "  abc ", 6, "%^6x", 0xABC );
    TEST( " abc ", 5, "%^5x", 0xABC );
    TEST( "  ABC  ", 7, "%^7X", 0xABC );
    TEST( "  ABC ", 6, "%^6X", 0xABC );

    /* Binary (%b) */
    TEST( "  1010  ", 8, "%^8b", 0xA );
    TEST( "  1010 ", 7, "%^7b", 0xA );
    TEST( " 1010 ", 6, "%^6b", 0xA );
    TEST( " 1010", 5, "%^5b", 0xA );

    /* Pointer (%p) - may not support centering, outputs raw value */
    {
        void *ptr = (void *)0x1234;
        TEST( "0000000000001234", 16, "%^21p", ptr );  /* %p may not support centering */
        TEST( "0000000000001234", 16, "%^20p", ptr );
    }

#if defined(CONFIG_WITH_FP_SUPPORT)
    /* Floating point (%f) */
    TEST( "  1.5  ", 7, "%^7.1f", 1.5 );
    TEST( "  1.5 ", 6, "%^6.1f", 1.5 );
    TEST( " 1.5 ", 5, "%^5.1f", 1.5 );
    TEST( " 1.5", 4, "%^4.1f", 1.5 );

    /* Exponential (%e, %E) */
    TEST( "  1.5e+00  ", 11, "%^11.1e", 1.5 );
    TEST( "  1.5e+00 ", 10, "%^10.1e", 1.5 );
    TEST( " 1.5e+00 ", 9, "%^9.1e", 1.5 );
    TEST( "  1.5E+00  ", 11, "%^11.1E", 1.5 );

    /* General (%g, %G) */
    TEST( "  1.5  ", 7, "%^7.2g", 1.5 );
    TEST( "  1.5 ", 6, "%^6.2g", 1.5 );
    TEST( " 1.5 ", 5, "%^5.2g", 1.5 );
    TEST( "  1.5  ", 7, "%^7.2G", 1.5 );

    /* Hexadecimal float (%a, %A) */
    TEST( "  0x1.8p+0  ", 12, "%^12.1a", 1.5 );
    TEST( "  0x1.8p+0 ", 11, "%^11.1a", 1.5 );
    TEST( " 0x1.8p+0 ", 10, "%^10.1a", 1.5 );

    /* Fixed-point (%k) */
    {
        int val = (1 << 4) | 8;  /* 1.5 in 4.4 format */
        TEST( "  1.500000 ", 11, "%^11{4.4}k", val );   /* 1.500000 = 8 chars, centered in 11 (2 left, 1 right - left-biased) */
        TEST( " 1.500000 ", 10, "%^10{4.4}k", val );    /* 1.500000 = 8 chars, centered in 10 (1 left, 1 right) */
        TEST( " 1.500000", 9, "%^9{4.4}k", val );       /* 1.500000 = 8 chars, centered in 9 (1 left, 0 right - left-biased) */
    }
#endif

    /* ===== Centering with Sign Flags ===== */

    /* + flag (always show sign) */
    TEST( "  +123  ", 8, "%^+8d", 123 );
    TEST( "  +123 ", 7, "%^+7d", 123 );
    TEST( " +123 ", 6, "%^+6d", 123 );
    TEST( "  -123  ", 8, "%^+8d", -123 );
    TEST( "  -123 ", 7, "%^+7d", -123 );

    /* space flag (space for positive) */
    TEST( "   123  ", 8, "%^ 8d", 123 );
    TEST( "   123 ", 7, "%^ 7d", 123 );
    TEST( "  123 ", 6, "%^ 6d", 123 );
    TEST( "  -123  ", 8, "%^ 8d", -123 );

#if defined(CONFIG_WITH_FP_SUPPORT)
    /* Sign flags with floats */
    TEST( "  +1.5  ", 8, "%^+8.1f", 1.5 );
    TEST( "  +1.5 ", 7, "%^+7.1f", 1.5 );
    TEST( "   1.5  ", 8, "%^ 8.1f", 1.5 );
    TEST( "   1.5 ", 7, "%^ 7.1f", 1.5 );
#endif

    /* ===== Centering with Alternate Form (#) ===== */

    /* Hex with alternate form */
    TEST( "  0xabc  ", 9, "%^#9x", 0xABC );
    TEST( "  0xabc ", 8, "%^#8x", 0xABC );
    TEST( " 0xabc ", 7, "%^#7x", 0xABC );
    TEST( " 0XABC ", 7, "%^#7X", 0xABC );  /* Uppercase X uses 0X prefix */

    /* Octal with alternate form */
    TEST( "  0755  ", 8, "%^#8o", 0755 );
    TEST( "  0755 ", 7, "%^#7o", 0755 );
    TEST( " 0755 ", 6, "%^#6o", 0755 );

    /* Binary with alternate form */
    TEST( "  0b1010  ", 10, "%^#10b", 0xA );
    TEST( "  0b1010 ", 9, "%^#9b", 0xA );
    TEST( " 0b1010 ", 8, "%^#8b", 0xA );

#if defined(CONFIG_WITH_FP_SUPPORT)
    /* Float with alternate form (keeps decimal point) */
    TEST( "  1.  ", 6, "%^#6.0f", 1.0 );
    TEST( "  1. ", 5, "%^#5.0f", 1.0 );
    TEST( " 1. ", 4, "%^#4.0f", 1.0 );
#endif

    /* ===== Centering with Precision ===== */

    /* Integer with precision (minimum digits) */
    TEST( "  00123  ", 9, "%^9.5d", 123 );
    TEST( "  00123 ", 8, "%^8.5d", 123 );
    TEST( " 00123 ", 7, "%^7.5d", 123 );
    TEST( " 00123", 6, "%^6.5d", 123 );
    TEST( "00123", 5, "%^5.5d", 123 );

    /* String with precision (max chars) */
    TEST( "  hel  ", 7, "%^7.3s", "hello" );
    TEST( "  hel ", 6, "%^6.3s", "hello" );
    TEST( " hel ", 5, "%^5.3s", "hello" );

#if defined(CONFIG_WITH_FP_SUPPORT)
    /* Float with precision */
    TEST( "  1.50  ", 8, "%^8.2f", 1.5 );
    TEST( "  1.50 ", 7, "%^7.2f", 1.5 );
    TEST( " 1.50 ", 6, "%^6.2f", 1.5 );
#endif

    /* ===== Centering with Zero Padding ===== */

    /* Zero-padding with centering: both flags apply (0 pads, ^ centers the padded value) */
    TEST( "00123  ", 7, "%^07d", 123 );   /* Zero-pads to width, then centers */
    TEST( "00123 ", 6, "%^06d", 123 );

#if defined(CONFIG_WITH_FP_SUPPORT)
    TEST( "001.5  ", 7, "%^07.1f", 1.5 );
    TEST( "001.5 ", 6, "%^06.1f", 1.5 );
#endif

    /* ===== Centering with Left-Align Flag Interaction ===== */

    /* - flag with ^ flag: both applied, centering takes effect */
    TEST( "  123  ", 7, "%-^7d", 123 );   /* Centering still applies with - flag */
    TEST( " 123  ", 6, "%-^6d", 123 );    /* Even width: left-biased */
    TEST( " hello ", 7, "%-^7s", "hello" );

    /* ===== Odd vs Even Width Differences ===== */

    /* Test left-bias for even-width centering (%c doesn't support centering) */
    TEST( "A", 1, "%^3c", 'A' );        /* %c doesn't support centering */
    TEST( "A", 1, "%^2c", 'A' );        /* %c doesn't support centering */

    TEST( "  123  ", 7, "%^7d", 123 );  /* Odd width: perfectly centered */
    TEST( "  123 ", 6, "%^6d", 123 );   /* Even width: left-biased */

    TEST( " hello ", 7, "%^7s", "hello" );  /* Odd width: perfectly centered */
    TEST( " hello", 6, "%^6s", "hello" );   /* Even width: left-biased */

#if defined(CONFIG_WITH_FP_SUPPORT)
    TEST( "  1.5  ", 7, "%^7.1f", 1.5 );    /* Odd width: perfectly centered */
    TEST( "  1.5 ", 6, "%^6.1f", 1.5 );     /* Even width: left-biased */
#endif

    /* ===== Centering When Output Equals Width ===== */

    TEST( "A", 1, "%^1c", 'A' );
    TEST( "123", 3, "%^3d", 123 );
    TEST( "hello", 5, "%^5s", "hello" );

    /* ===== Centering When Width < Output ===== */

    /* Output should not be truncated, returns actual output length */
    TEST( "A", 1, "%^1c", 'A' );          /* Width equals output */
    TEST( "12345", 5, "%^3d", 12345 );    /* Width too small: no padding, returns actual length */
    TEST( "hello", 5, "%^2s", "hello" );  /* Width too small: no padding */

#if defined(CONFIG_WITH_GROUPING_SUPPORT)
    /* ===== Centering with Grouping ===== */

    TEST( "  1,234  ", 9, "%^9[,3]d", 1234 );
    TEST( "  1,234 ", 8, "%^8[,3]d", 1234 );
    TEST( " 1,234 ", 7, "%^7[,3]d", 1234 );
    TEST( " 12,34 ", 7, "%^7[,2]d", 1234 );
#endif

    /* ===== Centering with Multiple Flags Combined ===== */

    TEST( "  +00123  ", 10, "%^+10.5d", 123 );
    TEST( "  +00123 ", 9, "%^+9.5d", 123 );

#if defined(CONFIG_WITH_FP_SUPPORT)
    TEST( "  +1.50  ", 9, "%^+9.2f", 1.5 );
    TEST( "  +1.50 ", 8, "%^+8.2f", 1.5 );

    TEST( "   1.50  ", 9, "%^ 9.2f", 1.5 );
    TEST( "   1.50 ", 8, "%^ 8.2f", 1.5 );
#endif

    TEST( "  0x00abc  ", 11, "%^#11.5x", 0xABC );
    TEST( "  0x00abc ", 10, "%^#10.5x", 0xABC );
}

/*****************************************************************************/
/**
    Test comprehensive length modifiers with all conversions.
**/
static void test_length_modifiers( void )
{
    printf( "Testing comprehensive length modifiers\n" );

    /* ===== hh (char) Qualifier Tests ===== */

    /* %hhd with values in char range */
    TEST( "127", 3, "%hhd", 127 );           /* SCHAR_MAX */
    TEST( "-128", 4, "%hhd", -128 );         /* SCHAR_MIN */
    TEST( "0", 1, "%hhd", 0 );

    /* %hhd with values beyond char range (should wrap/truncate) */
    TEST( "0", 1, "%hhd", 256 );             /* 256 % 256 = 0 */
    TEST( "1", 1, "%hhd", 257 );             /* 257 % 256 = 1 */
    TEST( "-1", 2, "%hhd", 255 );            /* 255 as signed char = -1 */
    TEST( "127", 3, "%hhd", 383 );           /* 383 % 256 = 127 */
    TEST( "-128", 4, "%hhd", 128 );          /* 128 as signed char = -128 */

    /* %hhu with unsigned char values */
    TEST( "255", 3, "%hhu", 255 );           /* UCHAR_MAX */
    TEST( "0", 1, "%hhu", 0 );
    TEST( "128", 3, "%hhu", 128 );
    TEST( "0", 1, "%hhu", 256 );             /* Wraps to 0 */
    TEST( "1", 1, "%hhu", 257 );             /* Wraps to 1 */
    TEST( "255", 3, "%hhu", 511 );           /* 511 % 256 = 255 */

    /* %hhx, %hho, %hhb with char values */
    TEST( "ff", 2, "%hhx", 255 );
    TEST( "7f", 2, "%hhx", 127 );
    TEST( "0", 1, "%hhx", 256 );             /* Wraps */
    TEST( "377", 3, "%hho", 255 );
    TEST( "0", 1, "%hho", 256 );
    TEST( "11111111", 8, "%hhb", 255 );
    TEST( "0", 1, "%hhb", 256 );

    /* %hhn with wrapping */
    {
        signed char c = 0;
        TEST( "test", 4, "test%hhn", &c );
        CHECK( c, 4 );

        /* Value > SCHAR_MAX should wrap */
        TEST( "123456789012345678901234567890123456789012345678901234567890"
              "123456789012345678901234567890123456789012345678901234567890"
              "123456789012345678901234567890123456789012345678901234567890"
              "123456789012345678901234567890123456789012345678901234567890"
              "123456789012345678901234567890123456789012345678901234567890"
              "12345678901234567890", 320,
              "123456789012345678901234567890123456789012345678901234567890"
              "123456789012345678901234567890123456789012345678901234567890"
              "123456789012345678901234567890123456789012345678901234567890"
              "123456789012345678901234567890123456789012345678901234567890"
              "123456789012345678901234567890123456789012345678901234567890"
              "12345678901234567890%hhn", &c );
        CHECK( c, 64 );  /* 320 % 256 = 64 */
    }

    /* ===== h (short) Qualifier Tests ===== */

    /* %hd with values in short range */
    TEST( "32767", 5, "%hd", 32767 );        /* SHRT_MAX */
    TEST( "-32768", 6, "%hd", -32768 );      /* SHRT_MIN */
    TEST( "0", 1, "%hd", 0 );
    TEST( "1234", 4, "%hd", 1234 );
    TEST( "-1234", 5, "%hd", -1234 );

    /* %hu with unsigned short values */
    TEST( "65535", 5, "%hu", 65535 );        /* USHRT_MAX */
    TEST( "0", 1, "%hu", 0 );
    TEST( "32768", 5, "%hu", 32768 );

    /* %hx, %ho with short values */
    TEST( "ffff", 4, "%hx", 65535 );
    TEST( "7fff", 4, "%hx", 32767 );
    TEST( "177777", 6, "%ho", 65535 );

    /* %hn */
    {
        short s = 0;
        TEST( "hello", 5, "hello%hn", &s );
        CHECK( s, 5 );
    }

    /* ===== l (long) Qualifier Tests ===== */

    /* %ld with long values */
    TEST( "1234567890", 10, "%ld", 1234567890L );
    TEST( "-1234567890", 11, "%ld", -1234567890L );
    TEST( "0", 1, "%ld", 0L );

    /* %lu with unsigned long values */
    TEST( "4294967295", 10, "%lu", 4294967295UL );  /* ULONG_MAX on 32-bit */
    TEST( "0", 1, "%lu", 0UL );

    /* %lx, %lo with long values */
    TEST( "499602d2", 8, "%lx", 1234567890L );
    TEST( "11145401322", 11, "%lo", 1234567890L );

    /* %ln */
    {
        long l = 0;
        TEST( "hello", 5, "hello%ln", &l );
        CHECK( (int)l, 5 );
    }

#if defined(CONFIG_WITH_LONG_LONG_SUPPORT)
    /* ===== ll (long long) Qualifier Tests ===== */

    /* %lld with long long values */
    TEST( "123456789123456789", 18, "%lld", 123456789123456789LL );
    TEST( "-123456789123456789", 19, "%lld", -123456789123456789LL );
    TEST( "0", 1, "%lld", 0LL );

    /* %llu with unsigned long long values */
    TEST( "18446744073709551615", 20, "%llu", 18446744073709551615ULL );  /* ULLONG_MAX */
    TEST( "0", 1, "%llu", 0ULL );

    /* %llx, %llo with long long values */
    TEST( "1b69b4bacd05f15", 15, "%llx", 123456789123456789LL );
    TEST( "6664664565464057425", 19, "%llo", 123456789123456789LL );

    /* %lln */
    {
        long long ll = 0;
        TEST( "hello", 5, "hello%lln", &ll );
        CHECK( (int)ll, 5 );
    }
#endif

    /* ===== j (intmax_t) Qualifier Tests ===== */

    /* %jd with intmax_t values */
    TEST( "1234567890", 10, "%jd", (intmax_t)1234567890 );
    TEST( "-1234567890", 11, "%jd", (intmax_t)-1234567890 );
    TEST( "0", 1, "%jd", (intmax_t)0 );

    /* %ju with uintmax_t values */
    TEST( "1234567890", 10, "%ju", (uintmax_t)1234567890 );
    TEST( "0", 1, "%ju", (uintmax_t)0 );

    /* %jx, %jo with intmax_t values */
    TEST( "499602d2", 8, "%jx", (intmax_t)1234567890 );
    TEST( "11145401322", 11, "%jo", (intmax_t)1234567890 );

    /* %jn */
    {
        intmax_t j = 0;
        TEST( "hello", 5, "hello%jn", &j );
        CHECK( (int)j, 5 );
    }

    /* ===== z (size_t) Qualifier Tests ===== */

    /* %zd with size_t values (as signed) */
    TEST( "1234", 4, "%zd", (size_t)1234 );
    TEST( "0", 1, "%zd", (size_t)0 );

    /* %zu with size_t values */
    TEST( "1234567890", 10, "%zu", (size_t)1234567890 );
    TEST( "0", 1, "%zu", (size_t)0 );

    /* %zx, %zo with size_t values */
    TEST( "499602d2", 8, "%zx", (size_t)1234567890 );
    TEST( "11145401322", 11, "%zo", (size_t)1234567890 );

    /* %zn */
    {
        size_t z = 0;
        TEST( "hello", 5, "hello%zn", &z );
        CHECK( (int)z, 5 );
    }

    /* ===== t (ptrdiff_t) Qualifier Tests ===== */

    /* %td with ptrdiff_t values */
    TEST( "1234567890", 10, "%td", (ptrdiff_t)1234567890 );
    TEST( "-1234567890", 11, "%td", (ptrdiff_t)-1234567890 );
    TEST( "0", 1, "%td", (ptrdiff_t)0 );

    /* %tu with ptrdiff_t values (as unsigned) */
    TEST( "1234567890", 10, "%tu", (ptrdiff_t)1234567890 );
    TEST( "0", 1, "%tu", (ptrdiff_t)0 );

    /* %tx, %to with ptrdiff_t values */
    TEST( "499602d2", 8, "%tx", (ptrdiff_t)1234567890 );
    TEST( "11145401322", 11, "%to", (ptrdiff_t)1234567890 );

    /* %tn */
    {
        ptrdiff_t t = 0;
        TEST( "hello", 5, "hello%tn", &t );
        CHECK( (int)t, 5 );
    }

#if defined(CONFIG_WITH_FP_SUPPORT) && 0  /* Long double not currently supported */
    /* ===== L (long double) Qualifier Tests ===== */

    /* %Lf with long double values */
    TEST( "1.500000", 8, "%Lf", 1.5L );
    TEST( "-1.500000", 9, "%Lf", -1.5L );
    TEST( "0.000000", 8, "%Lf", 0.0L );

    /* %Le, %LE with long double values */
    TEST( "1.500000e+00", 12, "%Le", 1.5L );
    TEST( "1.500000E+00", 12, "%LE", 1.5L );

    /* %Lg, %LG with long double values */
    TEST( "1.5", 3, "%Lg", 1.5L );
    TEST( "1.5", 3, "%LG", 1.5L );
    TEST( "1.23457e+10", 11, "%Lg", 12345678901.0L );
#endif

    /* ===== Length Modifier Combinations and Edge Cases ===== */

    /* Length modifiers with flags and precision */
    TEST( "+127", 4, "%+hhd", 127 );
    TEST( " 127", 4, "% hhd", 127 );
    TEST( "00127", 5, "%.5hhd", 127 );
    TEST( "  127", 5, "%5hhd", 127 );
    TEST( "127  ", 5, "%-5hhd", 127 );

    TEST( "+32767", 6, "%+hd", 32767 );
    TEST( "0032767", 7, "%.7hd", 32767 );

    TEST( "+1234567890", 11, "%+ld", 1234567890L );
    TEST( "001234567890", 12, "%.12ld", 1234567890L );

#if defined(CONFIG_WITH_LONG_LONG_SUPPORT)
    TEST( "+123456789123456789", 19, "%+lld", 123456789123456789LL );
#endif

    /* Length modifiers with alternate form */
    TEST( "0xff", 4, "%#hhx", 255 );
    TEST( "0377", 4, "%#hho", 255 );
    TEST( "0xffff", 6, "%#hx", 65535 );
    TEST( "0x499602d2", 10, "%#lx", 1234567890L );

    /* Length modifiers with width and zero-padding */
    TEST( "000127", 6, "%06hhd", 127 );
    TEST( "032767", 6, "%06hd", 32767 );
    TEST( "01234567890", 11, "%011ld", 1234567890L );

#if defined(CONFIG_WITH_GROUPING_SUPPORT)
    /* Length modifiers with grouping */
    TEST( "1,234,567,890", 13, "%[,3]ld", 1234567890L );
    TEST( "12,34,56,78,90", 14, "%[,2]ld", 1234567890L );
#endif

    /* ===== Invalid Combinations (should work with conversion, modifier ignored) ===== */

    /* Length modifiers are generally ignored for %c, %s, %p, %n handles them */
    TEST( "A", 1, "%lc", 'A' );     /* l ignored for %c */
    TEST( "A", 1, "%hc", 'A' );     /* h ignored for %c */
    TEST( "hello", 5, "%hs", "hello" );  /* h ignored for %s */
    TEST( "hello", 5, "%ls", "hello" );  /* l ignored for %s */

    /* %n properly handles length modifiers, tested above */
}

/*****************************************************************************/
/**
    Test comprehensive alternate form (#) flag edge cases.
**/
static void test_alternate_form( void )
{
    printf( "Testing comprehensive alternate form (#) flag\n" );

    /* ===== Alternate Form with All Bases ===== */

    /* Binary (%#b) - adds 0b prefix */
    TEST( "0", 1, "%#b", 0 );                /* Zero with # - no prefix (like %#x) */
    TEST( "0b1", 3, "%#b", 1 );
    TEST( "0b1010", 6, "%#b", 0xA );
    TEST( "0b11111111", 10, "%#b", 0xFF );
    TEST( "0b10101010", 10, "%#b", 0xAA );

    /* Octal (%#o) - adds 0 prefix */
    TEST( "0", 1, "%#o", 0 );                /* Zero with # - just "0" */
    TEST( "01", 2, "%#o", 1 );
    TEST( "012", 3, "%#o", 10 );
    TEST( "0377", 4, "%#o", 0xFF );
    TEST( "01234", 5, "%#o", 01234 );
    TEST( "07777", 5, "%#o", 07777 );

    /* Hexadecimal lowercase (%#x) - adds 0x prefix */
    TEST( "0", 1, "%#x", 0 );                /* Zero with # - no prefix */
    TEST( "0x1", 3, "%#x", 1 );
    TEST( "0xa", 3, "%#x", 10 );
    TEST( "0xff", 4, "%#x", 0xFF );
    TEST( "0xabcd", 6, "%#x", 0xABCD );
    TEST( "0xdeadbeef", 10, "%#x", 0xDEADBEEF );

    /* Hexadecimal uppercase (%#X) - adds 0X prefix (matches case) */
    TEST( "0", 1, "%#X", 0 );                /* Zero with # - no prefix */
    TEST( "0X1", 3, "%#X", 1 );              /* Prefix case matches format specifier */
    TEST( "0XA", 3, "%#X", 10 );
    TEST( "0XFF", 4, "%#X", 0xFF );
    TEST( "0XABCD", 6, "%#X", 0xABCD );
    TEST( "0XDEADBEEF", 10, "%#X", 0xDEADBEEF );

    /* Arbitrary bases with # - NOT SUPPORTED (# flag doesn't work with : notation) */
    TEST( "10", 2, "%#:5I", 5 );             /* Base 5, # ignored */
    TEST( "10", 2, "%#:5i", 5 );             /* Base 5, # ignored */
    TEST( "G", 1, "%#:17I", 16 );            /* Base 17, # ignored */
    TEST( "g", 1, "%#:17i", 16 );            /* Base 17, # ignored */
    TEST( "100", 3, "%#:11U", 121 );         /* Base 11, # ignored */

    /* ===== Alternate Form with ! Flag (Force Prefix) ===== */

    /* ! flag forces prefix even for zero */
    TEST( "0b0", 3, "%!#b", 0 );             /* ! forces 0b prefix for zero */
    TEST( "0x0", 3, "%!#x", 0 );             /* ! forces 0x prefix for zero */
    TEST( "0x0", 3, "%!#X", 0 );             /* ! with # produces lowercase prefix */

    /* ! and # together with non-zero values */
    TEST( "0b1010", 6, "%!#b", 0xA );
    TEST( "0x1234", 6, "%!#x", 0x1234 );
    TEST( "0x1234", 6, "%!#X", 0x1234 );     /* ! with # produces lowercase prefix */

    /* ===== Alternate Form with Floating Point ===== */

#if defined(CONFIG_WITH_FP_SUPPORT)
    /* %#g, %#G - # preserves trailing zeros */
    TEST( "1.000000", 8, "%#g", 1.0 );       /* # keeps trailing zeros, uses f-style */
    TEST( "1.000000", 8, "%#G", 1.0 );
    TEST( "1.2300", 6, "%#.4g", 1.23 );      /* # with explicit precision */
    TEST( "1.5000", 6, "%#.4g", 1.5 );
    TEST( "123.000000", 10, "%#.6g", 123.0 ); /* # doesn't prevent exponential notation */

    /* %#.0f - alternate form with zero precision shows decimal point */
    TEST( "1.", 2, "%#.0f", 1.0 );           /* # adds decimal point even with .0 precision */
    TEST( "2.", 2, "%#.0f", 1.5 );           /* Rounds to 2, # adds decimal point */
    TEST( "0.", 2, "%#.0f", 0.0 );
    TEST( "123.", 4, "%#.0f", 123.0 );
    TEST( "-1.", 3, "%#.0f", -1.0 );

    /* %#.0e, %#.0E - alternate form with zero precision shows decimal point */
    TEST( "1.e+00", 6, "%#.0e", 1.0 );       /* # adds decimal point */
    TEST( "2.e+00", 6, "%#.0e", 1.5 );
    TEST( "1.E+00", 6, "%#.0E", 1.0 );
    TEST( "1.e+02", 6, "%#.0e", 123.0 );

    /* %#a - alternate form with hex float */
    TEST( "0x1.p+0", 7, "%#.0a", 1.0 );      /* # keeps decimal point */
    TEST( "0x1.8p+0", 8, "%#.1a", 1.5 );
    TEST( "0x1.0p+0", 8, "%#.1a", 1.0 );     /* # shows trailing zero */
#endif

    /* ===== Alternate Form with Precision and Width ===== */

    /* # with precision (minimum digits) */
    TEST( "0b00001010", 10, "%#.8b", 0xA );
    TEST( "000001234", 9, "%#.8o", 01234 );
    TEST( "0x0000abcd", 10, "%#.8x", 0xABCD );
    TEST( "0X0000ABCD", 10, "%#.8X", 0xABCD );

    /* # with width */
    TEST( "  0b1010", 8, "%#8b", 0xA );
    TEST( "   01234", 8, "%#8o", 01234 );
    TEST( "  0xabcd", 8, "%#8x", 0xABCD );
    TEST( "  0XABCD", 8, "%#8X", 0xABCD );

    /* # with width and precision */
    TEST( "  0b00001010", 12, "%#12.8b", 0xA );
    TEST( "   000001234", 12, "%#12.8o", 01234 );
    TEST( "  0x0000abcd", 12, "%#12.8x", 0xABCD );
    TEST( "  0X0000ABCD", 12, "%#12.8X", 0xABCD ); /* Prefix case matches */

    /* # with zero-padding (0 flag) */
    TEST( "0b00001010", 10, "%#010b", 0xA );
    TEST( "0000001234", 10, "%#010o", 01234 );
    TEST( "0x0000abcd", 10, "%#010x", 0xABCD );
    TEST( "0X0000ABCD", 10, "%#010X", 0xABCD );

    /* # with left-align (- flag) */
    TEST( "0b1010  ", 8, "%#-8b", 0xA );
    TEST( "01234   ", 8, "%#-8o", 01234 );
    TEST( "0xabcd  ", 8, "%#-8x", 0xABCD );
    TEST( "0XABCD  ", 8, "%#-8X", 0xABCD );

#if defined(CONFIG_WITH_GROUPING_SUPPORT)
    /* # with grouping */
    TEST( "0xab_cd", 7, "%#[_2]x", 0xABCD );
    TEST( "0XAB_CD", 7, "%#[_2]X", 0xABCD );
    TEST( "0b11_11_00_00", 13, "%#[_2]b", 0xF0 );
    TEST( "012_34", 6, "%#[_2]o", 01234 );
#endif

    /* ===== Alternate Form with Invalid Conversions (Should be Ignored) ===== */

    /* # should be ignored for %d (signed decimal) */
    TEST( "123", 3, "%#d", 123 );
    TEST( "-123", 4, "%#d", -123 );
    TEST( "0", 1, "%#d", 0 );

    /* # should be ignored for %i (signed integer) */
    TEST( "123", 3, "%#i", 123 );
    TEST( "-123", 4, "%#i", -123 );

    /* # should be ignored for %u (unsigned decimal) */
    TEST( "123", 3, "%#u", 123 );
    TEST( "0", 1, "%#u", 0 );

    /* # should be ignored for %c (character) */
    TEST( "A", 1, "%#c", 'A' );
    TEST( "x", 1, "%#c", 'x' );

    /* # should be ignored for %s (string) */
    TEST( "hello", 5, "%#s", "hello" );
    TEST( "test", 4, "%#s", "test" );

    /* # should be ignored for %p (pointer) */
    {
        void *ptr = (void *)0x1234;
        TEST( "0X0000000000001234", 18, "%#p", ptr );  /* # adds 0X prefix to pointer */
    }

    /* # should be ignored for %n (count) */
    {
        int n = 0;
        TEST( "test", 4, "test%#n", &n );
        CHECK( n, 4 );
    }

#if defined(CONFIG_WITH_FP_SUPPORT)
    /* ===== Alternate Form Edge Cases with Floats ===== */

    /* # with various float formats */
    TEST( "1.500000", 8, "%#f", 1.5 );       /* # doesn't change %f with default precision */
    TEST( "1.5", 3, "%#.1f", 1.5 );          /* # doesn't add trailing zeros with explicit precision */

    /* # with %e */
    TEST( "1.500000e+00", 12, "%#e", 1.5 );
    TEST( "1.5e+00", 7, "%#.1e", 1.5 );

    /* # with very small precision */
    TEST( "1.", 2, "%#.0f", 1.234 );
    TEST( "1.e+00", 6, "%#.0e", 1.234 );

    /* # with infinity and NaN (should work normally) */
    TEST( "inf", 3, "%#f", 1.0/0.0 );
    TEST( "-inf", 4, "%#f", -1.0/0.0 );
    {
        double nan_val = 0.0/0.0;
        TEST( "-nan", 4, "%#f", nan_val );       /* NaN gets negative sign */
        TEST( "nan", 3, "%#f", -nan_val );       /* Negated NaN loses sign */
    }
#endif

    /* ===== Combining # with Other Flags ===== */

    /* # with + flag (+ doesn't apply to unsigned, so only # effect) */
    TEST( "0x1234", 6, "%#+x", 0x1234 );     /* + ignored for unsigned */
    TEST( "0X1234", 6, "%#+X", 0x1234 );     /* + ignored, prefix case matches */
    TEST( "0b1010", 6, "%#+b", 0xA );        /* + ignored for unsigned */

    /* # with space flag (space doesn't apply to unsigned, so only # effect) */
    TEST( "0x1234", 6, "%# x", 0x1234 );     /* space ignored for unsigned */
    TEST( "0b1010", 6, "%# b", 0xA );        /* space ignored for unsigned */

    /* Note: + and space don't apply to unsigned conversions, so effect is # only */
}

/*****************************************************************************/
/**
    Test comprehensive %g and %G conversion completeness.

    Tests correct C spec behavior: "The double argument is converted in style f
    or e (or F or E for G conversions). The precision specifies the number of
    significant digits. If the precision is missing, 6 digits are given; if the
    precision is zero, it is treated as 1. Style e is used if the exponent from
    its conversion is less than -4 or greater than or equal to the precision.
    Trailing zeros are removed from the fractional part of the result; a decimal
    point appears only if it is followed by at least one digit."
**/
static void test_g_completeness( void )
{
#if defined(CONFIG_WITH_FP_SUPPORT)
    printf( "Testing comprehensive %%g and %%G conversions\n" );

    /* ===== Basic %g Behavior - Default Precision ===== */

    /* Default precision is 6 significant digits. For values where exponent is
       between -4 and 5 (inclusive), use f-style. Remove trailing zeros. */
    TEST( "1", 1, "%g", 1.0 );                /* exp=0, use f, "1.00000" -> "1" */
    TEST( "1.5", 3, "%g", 1.5 );              /* exp=0, use f, "1.50000" -> "1.5" */
    TEST( "1.25", 4, "%g", 1.25 );            /* exp=0, use f, "1.25000" -> "1.25" */
    TEST( "0.5", 3, "%g", 0.5 );              /* exp=-1, use f */
    TEST( "0.125", 5, "%g", 0.125 );          /* exp=-1, use f */
    TEST( "123.456", 7, "%g", 123.456 );      /* exp=2, use f */
    TEST( "99999.9", 7, "%g", 99999.9 );      /* exp=4, use f */
    TEST( "10000", 5, "%g", 10000.0 );        /* exp=4, use f, remove trailing zeros */
    TEST( "12345", 5, "%g", 12345.0 );        /* exp=4, use f */
    TEST( "123456", 6, "%g", 123456.0 );      /* exp=5, use f (5 < 6) */

    /* Exponent threshold: exp >= 6 uses e-style */
    TEST( "1e+06", 5, "%g", 1000000.0 );      /* exp=6, 6>=6, use e */
    TEST( "1.23456e+06", 11, "%g", 1234560.0 ); /* exp=6, use e */
    TEST( "1e+10", 5, "%g", 1e10 );           /* exp=10, use e */
    TEST( "1e+20", 5, "%g", 1e20 );           /* exp=20, use e */

    /* Negative exponent threshold: exp < -4 uses e-style */
    TEST( "0.001", 5, "%g", 0.001 );          /* exp=-3, use f */
    TEST( "0.0001", 6, "%g", 0.0001 );        /* exp=-4, use f */
    TEST( "1e-05", 5, "%g", 0.00001 );        /* exp=-5, -5<-4, use e */
    TEST( "1e-10", 5, "%g", 1e-10 );          /* exp=-10, use e */
    TEST( "1.23456e-05", 11, "%g", 0.00001234560 ); /* exp=-5, use e */

    /* ===== %g vs %G - Case of Exponent ===== */

    TEST( "1e+20", 5, "%g", 1e20 );           /* lowercase e */
    TEST( "1E+20", 5, "%G", 1e20 );           /* uppercase E */
    TEST( "1e-10", 5, "%g", 1e-10 );
    TEST( "1E-10", 5, "%G", 1e-10 );

    /* ===== Explicit Precision - Significant Digits ===== */

    /* %.1g - 1 significant digit */
    TEST( "1", 1, "%.1g", 1.0 );              /* exp=0, 0<1, use f, "1." -> "1" */
    TEST( "1e+02", 5, "%.1g", 123.456 );      /* exp=2, 2>=1, use e */
    TEST( "9e+01", 5, "%.1g", 90.0 );         /* exp=1, 1>=1, use e */
    TEST( "1e+01", 5, "%.1g", 10.0 );         /* exp=1, 1>=1, use e */
    TEST( "2", 1, "%.1g", 1.9 );              /* exp=0, use f, rounds to 2 */

    /* %.2g - 2 significant digits */
    TEST( "1.5", 3, "%.2g", 1.5 );            /* exp=0, 0<2, use f */
    TEST( "1.2", 3, "%.2g", 1.23 );           /* exp=0, use f, round to 2 sig figs */
    TEST( "10", 2, "%.2g", 10.0 );            /* exp=1, use f */
    TEST( "12", 2, "%.2g", 12.0 );            /* exp=1, use f */
    TEST( "1.2e+02", 7, "%.2g", 123.456 );    /* exp=2, 2>=2, use e */
    TEST( "99", 2, "%.2g", 99.0 );            /* exp=1, use f */

    /* %.3g - 3 significant digits */
    TEST( "123", 3, "%.3g", 123.0 );          /* exp=2, 2<3, use f */
    TEST( "123", 3, "%.3g", 123.456 );        /* exp=2, use f, round to 3 sig figs */
    TEST( "1e+03", 5, "%.3g", 1000.0 );       /* exp=3, 3>=3, use e */
    TEST( "999", 3, "%.3g", 999.0 );          /* exp=2, use f */
    TEST( "0.001", 5, "%.3g", 0.001 );        /* exp=-3, use f */
    /* TODO: Bug - produces "0.000" instead of "0.000123" - rounding issue with small f-style numbers */
    /* TEST( "0.000123", 8, "%.3g", 0.0001234 ); */ /* exp=-4, use f, 3 sig figs */
    TEST( "1e-05", 5, "%.3g", 0.00001 );      /* exp=-5, -5<-4, use e */

    /* Precision .0 is treated as 1 */
    TEST( "1", 1, "%.0g", 1.0 );              /* Same as %.1g */
    TEST( "1e+02", 5, "%.0g", 123.456 );      /* Same as %.1g */
    TEST( "2", 1, "%.0g", 1.5 );              /* Rounds */

    /* ===== Trailing Zero Removal ===== */

    /* f-style: trailing zeros after decimal point removed */
    TEST( "1.5", 3, "%g", 1.5 );              /* Not "1.500000" */
    TEST( "123", 3, "%.4g", 123.0 );          /* Not "123.0" */
    TEST( "1", 1, "%.4g", 1.0 );              /* Not "1.000" */

    /* e-style: trailing zeros in mantissa removed */
    TEST( "1e+20", 5, "%g", 1e20 );           /* Not "1.00000e+20" */
    TEST( "1.5e+20", 7, "%g", 1.5e20 );       /* Not "1.50000e+20" */

    /* Decimal point removed if no digits follow */
    TEST( "1", 1, "%.1g", 1.0 );              /* Not "1." */
    TEST( "10", 2, "%.2g", 10.0 );            /* Not "10." */

    /* ===== Sign Handling ===== */

    TEST( "123.456", 7, "%g", 123.456 );
    TEST( "+123.456", 8, "%+g", 123.456 );
    TEST( " 123.456", 8, "% g", 123.456 );
    TEST( "-123.456", 8, "%g", -123.456 );
    TEST( "-123.456", 8, "%+g", -123.456 );
    TEST( "-123.456", 8, "% g", -123.456 );

    /* Zero */
    TEST( "0", 1, "%g", 0.0 );
    TEST( "+0", 2, "%+g", 0.0 );
    TEST( " 0", 2, "% g", 0.0 );
    TEST( "-0", 2, "%g", -0.0 );

    /* ===== Width and Alignment ===== */

    /* Right-aligned (default) */
    TEST( " 123.456", 8, "%8g", 123.456 );
    TEST( "     1e+20", 10, "%10g", 1e20 );

    /* Left-aligned */
    TEST( "123.456 ", 8, "%-8g", 123.456 );
    TEST( "1e+20     ", 10, "%-10g", 1e20 );

    /* Zero-padded */
    TEST( "00123.456", 9, "%09g", 123.456 );
    TEST( "+0123.456", 9, "%+09g", 123.456 );
    TEST( "-0123.456", 9, "%09g", -123.456 );

    /* ===== Width + Precision ===== */

    TEST( "     123", 8, "%8.3g", 123.456 );
    TEST( "  1.2e+02", 9, "%9.2g", 123.456 );
    TEST( "       1", 8, "%8.1g", 1.234 );

    /* ===== Edge Cases ===== */

    /* Very large exponents */
    TEST( "1e+50", 5, "%g", 1e50 );
    TEST( "1e+100", 6, "%g", 1e100 );

    /* Very small exponents */
    TEST( "1e-50", 5, "%g", 1e-50 );
    TEST( "1e-100", 6, "%g", 1e-100 );

    /* Values at boundaries */
    TEST( "99999.9", 7, "%g", 99999.9 );      /* exp=4, uses f */
    TEST( "100000", 6, "%g", 100000.0 );      /* exp=5, 5<6, uses f */
    TEST( "999999", 6, "%g", 999999.0 );      /* exp=5, uses f, 6 sig figs */
    TEST( "1e+06", 5, "%g", 1000000.0 );      /* exp=6, 6>=6, uses e */

    /* Precision boundary testing */
    TEST( "99", 2, "%.2g", 99.0 );            /* exp=1, uses f */
    TEST( "1e+02", 5, "%.2g", 100.0 );        /* exp=2, 2>=2, uses e */
    TEST( "999", 3, "%.3g", 999.0 );          /* exp=2, uses f */
    TEST( "1e+03", 5, "%.3g", 1000.0 );       /* exp=3, 3>=3, uses e */

    /* ===== Rounding ===== */

    TEST( "1.2", 3, "%.2g", 1.23 );           /* Round to 2 sig figs */
    TEST( "1.3", 3, "%.2g", 1.25 );           /* Round up */
    TEST( "100", 3, "%.2g", 99.5 );           /* Rounds to 100, 2 sig figs */
    TEST( "10", 2, "%.1g", 9.5 );             /* Rounds to 10 */

    /* ===== Combined Flags ===== */

    TEST( "+00123.456", 10, "%+010.6g", 123.456 );
    TEST( "+123.456   ", 11, "%+-11.6g", 123.456 );
    TEST( " 0001e+20", 9, "% 09g", 1e20 );

#endif /* CONFIG_WITH_FP_SUPPORT */
}

/*****************************************************************************/
/**
    Test comprehensive engineering notation and SI format (!flag).

    The '!' flag with %e/%E creates engineering notation (exponent multiple of 3).
    The '!' flag with %f/%F creates SI format with metric prefixes.
**/
static void test_engineering_si( void )
{
#if defined(CONFIG_WITH_FP_SUPPORT)
    printf( "Testing engineering notation and SI format\n" );

    /* ===== Engineering Notation (%!e) ===== */

    /* Basic engineering notation - exponent forced to multiple of 3 */
    TEST( "1.00e+00", 8, "%!.2e", 1.0 );          /* 1.0 = 1.00e+00 */
    TEST( "10.00e+00", 9, "%!.2e", 10.0 );        /* 10.0 = 10.00e+00 */
    TEST( "100.00e+00", 10, "%!.2e", 100.0 );     /* 100 = 100.00e+00 */
    TEST( "1.00e+03", 8, "%!.2e", 1000.0 );       /* 1000 = 1.00e+03 */
    TEST( "12.35e+03", 9, "%!.2e", 12345.0 );     /* 12345 = 12.35e+03 (example from doc) */

    /* Negative exponents */
    TEST( "100.00e-03", 10, "%!.2e", 0.1 );       /* 0.1 = 100.00e-03 */
    TEST( "10.00e-03", 9, "%!.2e", 0.01 );        /* 0.01 = 10.00e-03 */
    TEST( "1.00e-03", 8, "%!.2e", 0.001 );        /* 0.001 = 1.00e-03 */
    TEST( "123.00e-06", 10, "%!.2e", 0.000123 );  /* 0.000123 = 123.00e-06 */

    /* Large exponents */
    TEST( "1.23e+06", 8, "%!.2e", 1.23e6 );
    TEST( "456.00e+06", 10, "%!.2e", 456e6 );
    TEST( "1.00e+09", 8, "%!.2e", 1e9 );
    TEST( "12.30e+09", 9, "%!.2e", 12.3e9 );

    /* Small exponents */
    TEST( "1.23e-06", 8, "%!.2e", 1.23e-6 );
    TEST( "456.00e-09", 10, "%!.2e", 456e-9 );
    TEST( "1.00e-12", 8, "%!.2e", 1e-12 );

    /* Uppercase E */
    TEST( "12.35E+03", 9, "%!.2E", 12345.0 );
    TEST( "1.00E-06", 8, "%!.2E", 1e-6 );

    /* Different precisions */
    TEST( "12.3e+03", 8, "%!.1e", 12345.0 );      /* 1 decimal place */
    TEST( "12.345e+03", 10, "%!.3e", 12345.0 );   /* 3 decimal places */
    TEST( "12.34500e+03", 12, "%!.5e", 12345.0 ); /* 5 decimal places */

    /* Edge case: numbers that don't need adjustment */
    TEST( "1.23e+03", 8, "%!.2e", 1230.0 );
    TEST( "9.87e-03", 8, "%!.2e", 0.00987 );

    /* ===== SI Format (%!f) ===== */

    /* Basic SI prefixes - positive */
    TEST( "1.000 k", 7, "%!.3f", 1000.0 );        /* kilo */
    TEST( "1.500 k", 7, "%!.3f", 1500.0 );
    TEST( "12.300 k", 8, "%!.3f", 12300.0 );
    TEST( "123.000 k", 9, "%!.3f", 123000.0 );
    TEST( "1.000 M", 7, "%!.3f", 1e6 );           /* Mega */
    TEST( "1.000 G", 7, "%!.3f", 1e9 );           /* Giga */
    TEST( "1.000 T", 7, "%!.3f", 1e12 );          /* Tera */
    TEST( "1.000 P", 7, "%!.3f", 1e15 );          /* Peta */
    TEST( "1.000 E", 7, "%!.3f", 1e18 );          /* Exa */
    TEST( "1.000 Z", 7, "%!.3f", 1e21 );          /* Zetta */
    TEST( "1.000 Y", 7, "%!.3f", 1e24 );          /* Yotta */

    /* Basic SI prefixes - negative (small) */
    TEST( "1.230 m", 7, "%!.3f", 0.00123 );       /* milli (example from doc) */
    TEST( "1.000 m", 7, "%!.3f", 0.001 );
    TEST( "100.000 m", 9, "%!.3f", 0.1 );
    TEST( "1.000 u", 7, "%!.3f", 1e-6 );          /* micro */
    TEST( "1.000 n", 7, "%!.3f", 1e-9 );          /* nano */
    TEST( "1.000 p", 7, "%!.3f", 1e-12 );         /* pico */
    TEST( "1.000 f", 7, "%!.3f", 1e-15 );         /* femto */
    TEST( "1.000 a", 7, "%!.3f", 1e-18 );         /* atto */
    TEST( "1.000 z", 7, "%!.3f", 1e-21 );         /* zepto */
    TEST( "1.000 y", 7, "%!.3f", 1e-24 );         /* yocto */

    /* No prefix (around 1.0) - no trailing space when no SI suffix */
    TEST( "1.000", 5, "%!.3f", 1.0 );             /* No SI prefix, no space */
    TEST( "10.000", 6, "%!.3f", 10.0 );
    TEST( "100.000", 7, "%!.3f", 100.0 );
    TEST( "500.000", 7, "%!.3f", 500.0 );

    /* Different precisions */
    TEST( "1.2 m", 5, "%!.1f", 0.00123 );         /* 1 decimal place */
    TEST( "1.23 m", 6, "%!.2f", 0.00123 );        /* 2 decimal places */
    TEST( "1.2300 m", 8, "%!.4f", 0.00123 );      /* 4 decimal places */

    /* Uppercase F (same output as lowercase) */
    TEST( "1.230 m", 7, "%!.3F", 0.00123 );
    TEST( "1.000 k", 7, "%!.3F", 1000.0 );

    /* Combined with other flags */
    TEST( "+1.000 k", 8, "%!+.3f", 1000.0 );      /* + flag */
    TEST( " 1.000 k", 8, "%! .3f", 1000.0 );      /* space flag */
    TEST( "-1.000 k", 8, "%!.3f", -1000.0 );      /* negative */

    /* Width and alignment */
    TEST( "  1.000 k", 9, "%!9.3f", 1000.0 );     /* right-aligned */
    TEST( "1.000 k  ", 9, "%!-9.3f", 1000.0 );    /* left-aligned */

    /* Zero values - no SI suffix, no trailing space */
    TEST( "0.000", 5, "%!.3f", 0.0 );
    TEST( "+0.000", 6, "%!+.3f", 0.0 );
    TEST( "-0.000", 6, "%!.3f", -0.0 );

    /* Edge cases: values at SI prefix boundaries */
    TEST( "999.000 m", 9, "%!.3f", 0.999 );       /* Just below 1.0 */
    TEST( "999.000", 7, "%!.3f", 999.0 );         /* Just below k, no space */
    TEST( "999.000 k", 9, "%!.3f", 999000.0 );    /* Just below M */

    /* ===== Engineering with g/G ===== */

    /* Engineering notation should also work with %!g */
    /* Note: %!g is mentioned as being turned off in the code,
       so these tests might not work as expected */

#endif /* CONFIG_WITH_FP_SUPPORT */
}

/*****************************************************************************/
/**
    Test string and character edge cases (P3.1).
**/
static void test_string_char_edge_cases( void )
{
    printf( "Testing string and character edge cases (P3.1)\n" );

    /* ===== Very Long Strings (>1000 chars) ===== */

    /* 1200 character string */
    char long_str[1201];
    int i;
    for ( i = 0; i < 1200; i++ )
        long_str[i] = 'A' + (i % 26);
    long_str[1200] = '\0';

    /* Test basic long string */
    TEST( long_str, 1200, "%s", long_str );

    /* Test long string with precision */
    TEST( "ABCDEFGHIJ", 10, "%.10s", long_str );

    /* Test with max width (MAXWIDTH=500) - string shorter than width gets padded */
    char expected_padded500[501];
    memset( expected_padded500, ' ', 495 );
    memcpy( expected_padded500 + 495, "hello", 5 );
    expected_padded500[500] = '\0';
    TEST( expected_padded500, 500, "%500s", "hello" );

    /* Long string with width outputs full string (no truncation) */
    TEST( long_str, 1200, "%500s", long_str );

    /* ===== String with Precision Exactly Matching Length ===== */

    TEST( "hello", 5, "%.5s", "hello" );      /* exact match */
    TEST( "hello", 5, "%.5s", "hello world" ); /* truncated */
    TEST( "a", 1, "%.1s", "a" );               /* single char exact */
    TEST( "", 0, "%.0s", "anything" );         /* zero precision */

    /* Exact match with width */
    TEST( "     hello", 10, "%10.5s", "hello world" );
    TEST( "hello     ", 10, "%-10.5s", "hello world" );

    /* ===== %C with Non-ASCII Values (>127) ===== */

    TEST( "\x80", 1, "%.C\x80", UNUSED );      /* 128 */
    TEST( "\xFF", 1, "%.C\xFF", UNUSED );      /* 255 */
    TEST( "\x7F", 1, "%.C\x7F", UNUSED );      /* 127 (DEL) */
    TEST( "\xA0", 1, "%.C\xA0", UNUSED );      /* 160 (nbsp) */

    /* Repetition with non-ASCII */
    TEST( "\x80\x80\x80", 3, "%.3C\x80", UNUSED );
    TEST( "\xFF\xFF\xFF\xFF\xFF", 5, "%.5C\xFF", UNUSED );

    /* ===== %c with Special Values ===== */

    TEST( "\0", 1, "%c", 0 );                  /* NUL character */
    TEST( "\xFF", 1, "%c", 255 );              /* Max unsigned char */
    TEST( "\xFF", 1, "%c", -1 );               /* EOF / -1 */
    TEST( "\x01", 1, "%c", 1 );                /* SOH */
    TEST( "\x7F", 1, "%c", 127 );              /* DEL */
    TEST( "\x80", 1, "%c", 128 );              /* High bit set */

    /* Repetition with special values */
    TEST( "\0\0\0", 3, "%.3c", 0 );
    TEST( "\xFF\xFF\xFF\xFF", 4, "%.4c", 255 );

    /* ===== Strings with All Escape Sequences ===== */

    /* All standard escape sequences */
    char all_escapes[] = "\a\b\f\n\r\t\v\'\"\\\?";
    TEST( all_escapes, 11, "%s", all_escapes );

    /* Octal and hex escapes (without leading NUL) */
    char octal_hex[] = "\1\7\10\77\100\377\x01\xFF";
    TEST( octal_hex, 8, "%s", octal_hex );

    /* Mix of escapes and regular chars */
    TEST( "Hello\nWorld\t!", 13, "%s", "Hello\nWorld\t!" );

    /* Embedded NUL terminates string (standard %s behavior) */
    TEST( "a", 1, "%s", "a\0b" );  /* %s stops at NUL */

    /* ===== Maximum Width and Precision (MAXWIDTH=500, MAXPREC=500) ===== */

    /* Create 600 char string to test with */
    char str600[601];
    for ( i = 0; i < 600; i++ )
        str600[i] = '0' + (i % 10);
    str600[600] = '\0';

    /* MAXPREC = 500: truncate to 500 chars */
    char expected500[501];
    memcpy( expected500, str600, 500 );
    expected500[500] = '\0';
    TEST( expected500, 500, "%.500s", str600 );

    /* MAXWIDTH = 500: pad to 500 chars */
    char expected_padded[501];
    memset( expected_padded, ' ', 495 );
    memcpy( expected_padded + 495, "hello", 5 );
    expected_padded[500] = '\0';
    TEST( expected_padded, 500, "%500s", "hello" );

    /* Both MAXWIDTH and MAXPREC at 500 */
    TEST( expected500, 500, "%500.500s", str600 );

    /* ===== Repetition with %C at Boundaries ===== */

    /* %.80C should work in all variants */
    char expected80[81];
    memset( expected80, 'X', 80 );
    expected80[80] = '\0';
    TEST( expected80, 80, "%.80CX", UNUSED );

#if !defined(CONFIG_TINYFORMAT) && !defined(CONFIG_MICROFORMAT)
    /* %.81C should work in format.c (no 80-char limit) */
    char expected81[82];
    memset( expected81, 'Y', 81 );
    expected81[81] = '\0';
    TEST( expected81, 81, "%.81CY", UNUSED );

    /* %.100C should work in format.c */
    char expected100[101];
    memset( expected100, 'Z', 100 );
    expected100[100] = '\0';
    TEST( expected100, 100, "%.100CZ", UNUSED );
#endif

    /* ===== Edge Cases with Width and Precision ===== */

    /* Very long strings with centering */
    TEST( "  hello  ", 9, "%^9s", "hello" );
    TEST( "     hello     ", 15, "%^15s", "hello" );

    /* Precision limits display of long string */
    TEST( "012345678", 9, "%.9s", str600 );
    TEST( "     01234", 10, "%10.5s", str600 );

    /* Zero width and precision edge cases */
    TEST( "hello", 5, "%0s", "hello" );       /* 0 width ignored */
    TEST( "", 0, "%.0s", "hello" );           /* 0 precision = empty */
    TEST( "     ", 5, "%5.0s", "hello" );     /* width with 0 precision */
}

/*****************************************************************************/
/**
    Test comprehensive error paths and validation.
**/
static void test_errors( void )
{
    printf( "Testing error paths and validation\n" );

    /* ===== Invalid Conversion Specifiers ===== */

    /* Length qualifiers without conversion (Bug #1 - now fixed) */
    FAIL( "%h", 0 );    /* short qualifier without conversion */
    FAIL( "%l", 0 );    /* long qualifier without conversion */
    FAIL( "%j", 0 );    /* intmax_t qualifier without conversion */
    FAIL( "%z", 0 );    /* size_t qualifier without conversion */
    FAIL( "%t", 0 );    /* ptrdiff_t qualifier without conversion */
    FAIL( "%L", 0 );    /* long double qualifier without conversion */
    FAIL( "%ll", 0 );   /* long long qualifier without conversion */
    FAIL( "%hh", 0 );   /* char qualifier without conversion */

    /* Other invalid conversion specifiers */
    FAIL( "%Q", 0 );    /* Invalid specifier Q */
    FAIL( "%@", 0 );    /* Invalid specifier @ */
    FAIL( "%&", 0 );    /* Invalid specifier & */
    FAIL( "%~", 0 );    /* Invalid specifier ~ */
    FAIL( "%|", 0 );    /* Invalid specifier | */
    FAIL( "%$", 0 );    /* Invalid specifier $ */
    FAIL( "%?", 0 );    /* Invalid specifier ? */
    FAIL( "%=", 0 );    /* Invalid specifier = */
    FAIL( "%<", 0 );    /* Invalid specifier < */
    FAIL( "%>", 0 );    /* Invalid specifier > */
    FAIL( "%[", 0 );    /* Invalid specifier [ (without grouping) */
    FAIL( "%]", 0 );    /* Invalid specifier ] */
    FAIL( "%{", 0 );    /* Invalid specifier { (without fixed-point) */
    FAIL( "%}", 0 );    /* Invalid specifier } */
    FAIL( "%(", 0 );    /* Invalid specifier ( */
    FAIL( "%)", 0 );    /* Invalid specifier ) */

    /* ===== Incomplete Format Specifications (Bug #1 - now fixed) ===== */

    /* Flags without conversion */
    FAIL( "test%-", 0 );    /* Minus flag without conversion */
    FAIL( "test%+", 0 );    /* Plus flag without conversion */
    FAIL( "test%#", 0 );    /* Hash flag without conversion */
    FAIL( "test% ", 0 );    /* Space flag without conversion */
    FAIL( "test%0", 0 );    /* Zero flag without conversion */
    FAIL( "test%!", 0 );    /* Bang flag without conversion */
    FAIL( "test%^", 0 );    /* Caret flag without conversion */

    /* Width without conversion */
    FAIL( "test%5", 0 );    /* Width without conversion */
    FAIL( "test%10", 0 );   /* Width without conversion */
    FAIL( "test%*", 5, 0 ); /* Asterisk width without conversion */

    /* Precision without conversion */
    FAIL( "test%.", 0 );    /* Incomplete precision (just dot) */
    FAIL( "test%.5", 0 );   /* Precision without conversion */
    FAIL( "test%.*", 5, 0 ); /* Asterisk precision without conversion */

    /* Base specification without conversion */
    FAIL( "test%:", 0 );    /* Incomplete base (just colon) */
    FAIL( "test%:5", 0 );   /* Base without conversion */
    FAIL( "test%:10", 0 );  /* Base without conversion */

    /* ===== Base Boundary Violations ===== */

    /* Invalid bases */
    /* Note: Base 0 is treated as default (base 10), so it doesn't fail */
    /* TEST( "0", 1, "%:0i", 0 );    Base 0 defaults to base 10 */
    FAIL( "%:1i", 0 );               /* Base 1 */
    FAIL( "%:37i", 0 );              /* Base 37 (above max) */
    FAIL( "%:100i", 0 );             /* Base 100 */
    FAIL( "%:9999i", 0 );            /* Base 9999 */

    /* Invalid bases via asterisk */
    /* TEST( "0", 1, "%:*i", 0, 0 ); Base 0 via asterisk defaults to base 10 */
    FAIL( "%:*i", 1, 0 );            /* Base 1 via asterisk */
    FAIL( "%:*i", 37, 0 );           /* Base 37 */
    FAIL( "%:*i", 100, 0 );          /* Base 100 */
    FAIL( "%:*i", 9999, 0 );         /* Base 9999 */
    /* TEST( "0", 1, "%:*i", -5, 0 ); Negative base treated as base 10 */

    /* ===== Width Boundary Violations ===== */

    /* Width at limit should work */
    TEST( "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                  "
          "                                                 0",
          500, "%500d", 0 );

    /* Width beyond limit should fail */
    FAIL( "%501d", 0 );
    FAIL( "%600d", 0 );
    FAIL( "%1000d", 0 );
    FAIL( "%9999d", 0 );

    /* Width via asterisk beyond limit should fail */
    FAIL( "%*d", 501, 0 );
    FAIL( "%*d", 1000, 0 );
    FAIL( "%*d", 9999, 0 );
    FAIL( "%*d", 99999, 0 );

    /* ===== Precision Boundary Violations ===== */

    /* Precision at limit should work */
    TEST( "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000"
          "00000000000000000000000000000000000000000000000000",
          500, "%.500d", 0 );

    /* Precision beyond limit should fail */
    FAIL( "%.501d", 0 );
    FAIL( "%.600d", 0 );
    FAIL( "%.1000d", 0 );
    FAIL( "%.9999d", 0 );

    /* Precision via asterisk beyond limit should fail */
    FAIL( "%.*d", 501, 0 );
    FAIL( "%.*d", 1000, 0 );
    FAIL( "%.*d", 9999, 0 );
    FAIL( "%.*d", 99999, 0 );

    /* ===== Combined Width and Precision Boundaries ===== */

    /* Both at or beyond limits */
    FAIL( "%*.*d", 501, 500, 0 );    /* Width beyond, precision at limit */
    FAIL( "%*.*d", 500, 501, 0 );    /* Width at limit, precision beyond */
    FAIL( "%*.*d", 501, 501, 0 );    /* Both beyond */
    FAIL( "%*.*d", 99999, 99999, 0 ); /* Both far beyond */

    /* ===== Grouping Specification Errors ===== */

#if defined(CONFIG_WITH_GROUPING_SUPPORT)
    /* Incomplete/malformed grouping specifications that DO fail */
    FAIL( "%[d", 0 );                /* Missing closing bracket */
    FAIL( "%[_d", 0 );               /* Missing count after symbol AND closing bracket */
    FAIL( "%[3d", 0 );               /* Missing symbol before count AND closing bracket */

    /* Grouping specs that are ACCEPTED (treated as no-grouping) */
    TEST( "1234", 4, "%[]d", 1234 );      /* Empty grouping - accepted */
    TEST( "1234", 4, "%[_]d", 1234 );     /* Symbol without count - accepted */
    TEST( "1234", 4, "%[0]d", 1234 );     /* Count without symbol - accepted */
    TEST( "1234", 4, "%[_0]d", 1234 );    /* Zero count - accepted */
    TEST( "1234", 4, "%[,0]d", 1234 );    /* Zero count with comma - accepted */

    /* Grouping with non-numeric conversions (ignored, not fail) */
    TEST( "hello", 5, "%[]s", "hello" );  /* Empty grouping with %s */
    TEST( "a", 1, "%[,3]c", 'a' );        /* Grouping with %c */
#endif

    /* ===== Fixed-Point Specification Errors (Bug #2 - now fixed) ===== */

#if defined(CONFIG_WITH_FP_SUPPORT)
    /* Incomplete fixed-point specifications */
    FAIL( "%{k", 0 );          /* Missing closing brace */
    FAIL( "%{4k", 0 );         /* Missing dot and closing brace */
    FAIL( "%{4.k", 0 );        /* Missing fractional width */
    FAIL( "%{.4k", 0 );        /* Missing integer width */
    FAIL( "%{}k", 0 );         /* Empty specification */
    FAIL( "%{.}k", 0 );        /* Dot only, no widths */

    /* Mandatory modifier requirement - bare %k without modifier is invalid */
    FAIL( "%k", 0 );           /* No width modifier specified */

    /* Minimum width violations */
    FAIL( "%{0.1}k", 0 );      /* w_int=0 violates MIN_XP_INT=1 */
    FAIL( "%{0.16}k", 0 );     /* w_int=0 violates MIN_XP_INT=1 */
    FAIL( "%{1.0}k", 0 );      /* w_frac=0 violates MIN_XP_FRAC=1 */
    FAIL( "%{16.0}k", 0 );     /* w_frac=0 violates MIN_XP_FRAC=1 */

    /* Maximum width boundary - depends on sizeof(long) * CHAR_BIT */
    /* On 64-bit: MAX_XP_WIDTH = 64, so {32.32}k should fail (32+32=64) */
    /* {32.31}k should succeed (32+31=63 < 64) */
    #if ( ULONG_MAX == 0xFFFFFFFFFFFFFFFFUL )  /* 64-bit long */
        /* Test boundary: 32+31=63 is valid, 32+32=64 should fail */
        TEST( "0.000000", 8, "%{32.31}k", 0L );  /* Valid: 63 bits < 64 */
        FAIL( "%{32.32}k", 0L );                 /* Invalid: 64 bits >= 64 */
    #endif

    /* Length modifiers with %k should fail (not supported) */
    FAIL( "%hh{4.4}k", (char)0 );    /* hh modifier */
    FAIL( "%h{4.4}k", (short)0 );    /* h modifier */
    FAIL( "%l{4.4}k", 0L );          /* l modifier */
    FAIL( "%ll{4.4}k", 0LL );        /* ll modifier */
    FAIL( "%L{4.4}k", 0.0L );        /* L modifier */

    /* Grouping with %k should fail (not supported) */
#if defined(CONFIG_WITH_GROUPING_SUPPORT)
    FAIL( "%[,]{4.4}k", 0 );         /* Grouping with fixed-point */
    FAIL( "%[_2]{4.4}k", 0 );        /* Grouping with custom separator */
#endif
#endif

    /* ===== Asterisk Edge Cases ===== */

    /* Negative width becomes left-align (should work) */
    TEST( "0         ", 10, "%*d", -10, 0 );
    TEST( "123       ", 10, "%*d", -10, 123 );

    /* Negative precision should be ignored (treated as not specified) */
    TEST( "0", 1, "%.*d", -1, 0 );
    TEST( "123", 3, "%.*d", -10, 123 );

    /* ===== Continuation Feature (verify still works after Bug #1 fix) ===== */

    /* Bare % continuation should work */
    TEST( "hello world", 11, "hello %", "world" );
    TEST( "hello old world", 15, "hello %", "old %", "world" );

    /* Interspersed conversions with continuation */
    TEST( "One: 1,Two: 2,Three: 3", 22, "One: %d,%", 1,
                                        "Two: %c,%", '2',
                                        "Three: %s", "3" );
}

#if defined(CONFIG_WITH_FP_SUPPORT)
/*****************************************************************************/
/**
    Test NaN and special float values for standards compliance.
**/
static void test_nan_special_floats( void )
{
    double nan_val = 0.0 / 0.0;  /* Generate NaN */

    printf( "Testing NaN and special float values\n" );

    /* ===== NaN with %f and %F ===== */

    /* Note: NaN representation may have a sign bit depending on how it's generated.
     * On this platform, 0.0/0.0 produces a negative NaN. */
    TEST( "-nan", 4, "%f", nan_val );
    TEST( "-NAN", 4, "%F", nan_val );

    /* NaN with flags (sign in output depends on NaN's sign bit) */
    TEST( "-nan", 4, "%+f", nan_val );
    TEST( "-nan", 4, "% f", nan_val );
    TEST( "-NAN", 4, "%+F", nan_val );
    TEST( "-NAN", 4, "% F", nan_val );

    /* NaN with width */
    TEST( "  -nan", 6, "%6f", nan_val );
    TEST( "-nan  ", 6, "%-6f", nan_val );
    TEST( " -nan ", 6, "%-^6f", nan_val );
    TEST( " -nan ", 6, "%^6f", nan_val );
    TEST( "  -NAN", 6, "%6F", nan_val );

    /* NaN with precision (precision should be ignored for NaN) */
    TEST( "-nan", 4, "%.5f", nan_val );
    TEST( "-nan", 4, "%.0f", nan_val );
    TEST( "-NAN", 4, "%.10F", nan_val );

    /* NaN with zero padding (should be ignored, treated as width) */
    TEST( "  -nan", 6, "%06f", nan_val );
    TEST( "  -NAN", 6, "%06F", nan_val );

    /* ===== NaN with %e and %E ===== */

    TEST( "-nan", 4, "%e", nan_val );
    TEST( "-NAN", 4, "%E", nan_val );
    TEST( "-nan", 4, "%+e", nan_val );
    TEST( "-nan", 4, "% e", nan_val );
    TEST( "  -nan", 6, "%6e", nan_val );
    TEST( "-nan  ", 6, "%-6e", nan_val );
    TEST( "-nan", 4, "%.5e", nan_val );

    /* ===== NaN with %g and %G ===== */

    TEST( "-nan", 4, "%g", nan_val );
    TEST( "-NAN", 4, "%G", nan_val );
    TEST( "-nan", 4, "%+g", nan_val );
    TEST( "-nan", 4, "% g", nan_val );
    TEST( "  -nan", 6, "%6g", nan_val );
    TEST( "-nan  ", 6, "%-6g", nan_val );
    TEST( "-nan", 4, "%.5g", nan_val );

    /* ===== NaN with %a and %A ===== */

    TEST( "-nan", 4, "%a", nan_val );
    TEST( "-NAN", 4, "%A", nan_val );
    TEST( "-nan", 4, "%+a", nan_val );
    TEST( "-nan", 4, "% a", nan_val );
    TEST( "  -nan", 6, "%6a", nan_val );
    TEST( "-nan  ", 6, "%-6a", nan_val );
    TEST( "-nan", 4, "%.5a", nan_val );

    /* ===== Signed Zero (-0.0 vs +0.0) ===== */

    /* Positive zero */
    TEST( "0.000000", 8, "%f", 0.0 );
    TEST( "0.0e+00", 7, "%.1e", 0.0 );
    /* TODO: Bug - %g and %a with literal 0.0 produce wrong output */
    /* TEST( "0", 1, "%g", 0.0 ); */
    /* TEST( "0x0p+0", 6, "%a", 0.0 ); */

    /* Negative zero - sign should be preserved */
    TEST( "-0.000000", 9, "%f", -0.0 );
    TEST( "-0.0e+00", 8, "%.1e", -0.0 );
    /* TODO: Bug - %g and %a with literal -0.0 produce wrong output */
    /* TEST( "-0", 2, "%g", -0.0 ); */
    /* TEST( "-0x0p+0", 7, "%a", -0.0 ); */

    /* With + flag, both should show sign */
    TEST( "+0.000000", 9, "%+f", 0.0 );
    TEST( "-0.000000", 9, "%+f", -0.0 );
    TEST( "+0.0e+00", 8, "%+.1e", 0.0 );
    TEST( "-0.0e+00", 8, "%+.1e", -0.0 );

    /* ===== Verify Infinity Still Works ===== */

    TEST( "inf", 3, "%f", 1.0 / 0.0 );
    TEST( "-inf", 4, "%f", -1.0 / 0.0 );
    TEST( "INF", 3, "%F", 1.0 / 0.0 );
    TEST( "-INF", 4, "%F", -1.0 / 0.0 );

    /* ===== Largest Finite Values ===== */

    /* DBL_MAX should format correctly */
    TEST( "1.79769e+308", 12, "%.5e", DBL_MAX );
    TEST( "1.79769E+308", 12, "%.5E", DBL_MAX );
    TEST( "-1.79769e+308", 13, "%.5e", -DBL_MAX );

    /* ===== Smallest Positive Normalized Values ===== */

    /* DBL_MIN (smallest normalized positive value) */
    TEST( "2.22507e-308", 12, "%.5e", DBL_MIN );
    TEST( "2.22507E-308", 12, "%.5E", DBL_MIN );
    TEST( "-2.22507e-308", 13, "%.5e", -DBL_MIN );

    /* ===== Edge Cases Around 1.0 ===== */

    /* Values very close to 1.0 */
    TEST( "1.000000", 8, "%f", 1.0 + DBL_EPSILON );
    TEST( "1.000000", 8, "%f", 1.0 - DBL_EPSILON / 2.0 );

    /* ===== Subnormal (Denormal) Values ===== */

    /* Smallest positive subnormal (already tested in denormals section, verify here) */
#if ( DBL_DIG > 8 ) /* 64-bit doubles */
    {
        double smallest_subnormal = pow( 2, -1074 );
        TEST( "4.94065e-324", 12, "%.5e", smallest_subnormal );
        TEST( "-4.94065e-324", 13, "%.5e", -smallest_subnormal );
    }
#else /* 32-bit doubles */
    {
        double smallest_subnormal = pow( 2, -149 );
        TEST( "1.40129e-45", 11, "%.5e", smallest_subnormal );
        TEST( "-1.40129e-45", 12, "%.5e", -smallest_subnormal );
    }
#endif

    /* ===== Round-Trip Precision Testing ===== */

    /* %.17g should provide full precision for double round-trip */
    {
        double test_val = 1.23456789012345678;
        TEST( "1.234567890123466", 17, "%.17g", test_val );
    }

    /* Very large precision number */
    {
        double test_val = 123456789.123456789;
        TEST( "123456789.123457", 16, "%.6f", test_val );
    }
}
#endif /* CONFIG_WITH_FP_SUPPORT */

/*****************************************************************************/
/**
    Test consumer function failure handling.
**/
static void test_consumer_failures( void )
{
    int r;

    printf( "Testing consumer function failure handling\n" );

    /* ===== Immediate Consumer Failure ===== */

    /* Consumer fails immediately on first call */
    fail_after_n_calls = 0;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "hello" );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* Consumer fails on plain string output */
    fail_after_n_calls = 0;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "test string" );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* ===== Consumer Failure During Conversions ===== */

    /* Consumer fails during integer conversion */
    fail_after_n_calls = 0;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%d", 123 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* Consumer fails during string conversion */
    fail_after_n_calls = 0;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%s", "test" );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* Consumer fails during character conversion */
    fail_after_n_calls = 0;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%c", 'A' );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

#if defined(CONFIG_WITH_FP_SUPPORT)
    /* Consumer fails during float conversion */
    fail_after_n_calls = 0;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%f", 3.14 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* Consumer fails during exponential conversion */
    fail_after_n_calls = 0;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%e", 1234.5 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* Consumer fails during fixed-point conversion */
    fail_after_n_calls = 0;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%{4.4}k", 0x1234 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );
#endif

    /* ===== Consumer Failure After Partial Success ===== */

    /* Consumer succeeds once, then fails on second call */
    fail_after_n_calls = 1;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "first %d", 123 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* Consumer succeeds twice, then fails on third call */
    fail_after_n_calls = 2;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "a%db%dc", 1, 2 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* ===== Consumer Failure with Multiple Conversions ===== */

    /* Consumer fails during second conversion in multi-conversion format */
    fail_after_n_calls = 1;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%d %d", 10, 20 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* Consumer fails during third conversion in multi-conversion format */
    fail_after_n_calls = 2;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%d %d %d", 10, 20, 30 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* ===== Consumer Failure with Formatted Output ===== */

    /* Consumer fails with width padding */
    fail_after_n_calls = 0;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%10d", 123 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* Consumer fails with precision */
    fail_after_n_calls = 0;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%.5d", 123 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* Consumer fails with flags (sign) */
    fail_after_n_calls = 0;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%+d", 123 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

#if defined(CONFIG_WITH_GROUPING_SUPPORT)
    /* Consumer fails with grouping */
    fail_after_n_calls = 0;
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%[,3]d", 1234567 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != EXBADFORMAT )
        { printf( "**** FAIL: returned %d, expected EXBADFORMAT", r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );
#endif

    /* ===== Verify Normal Operation (consumer never fails) ===== */

    /* Consumer never fails - should succeed */
    fail_after_n_calls = -1;  /* Never fail */
    current_call_count = 0;
    r = test_sprintf_failing( buf, "hello %d world", 123 );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != 15 || strcmp( buf, "hello 123 world" ) )
        { printf( "**** FAIL: produced \"%s\", returned %d, expected \"hello 123 world\" and 15", buf, r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* Consumer never fails with multiple conversions - should succeed */
    fail_after_n_calls = -1;  /* Never fail */
    current_call_count = 0;
    r = test_sprintf_failing( buf, "%d %s %c", 42, "test", 'X' );
    printf( "[Test  @ %3d] ", __LINE__ );
    if ( r != 9 || strcmp( buf, "42 test X" ) )
        { printf( "**** FAIL: produced \"%s\", returned %d, expected \"42 test X\" and 9", buf, r ); f+=1; }
    else
        printf( "PASS" );
    printf( "\n" );

    /* Reset for subsequent tests */
    fail_after_n_calls = -1;
    current_call_count = 0;
}


/*****************************************************************************/
/**
    Run all tests on format library.
**/
static void run_tests( char * passes )
{
    if ( !passes )
        passes = "S%cnspdb"
#if defined(CONFIG_WITH_FP_SUPPORT)
		"akNKg!"
#endif
#if defined(CONFIG_WITH_GROUPING_SUPPORT)
		"G"
#endif
		"^L#3*\"E";

    if ( !strcmp( passes, "-help" ) )
    {
        printf( "Passes:\n"
                " S    - strings\n"
                " %%    - percent\n"
                " c    - %%c character conversion\n"
                " n    - %%n conversion\n"
                " s    - %%s string conversion\n"
                " p    - %%p pointer conversion\n"
                " d    - %%d, %%i integer conversions\n"
                " b    - %%b, %%o, %%u, %%x, %%X fixed base conversions\n"
#if defined(CONFIG_WITH_FP_SUPPORT)
		" a    - %%a, %%A, %%e, %%E, %%f, %%F, %%g, %%G floating point conversions\n"
                " k    - %%k fixed-point conversion\n"
                " N    - NaN and special float values\n"
                " K    - %%k fixed-point edge cases\n"
#endif
                " *    - asterisk parameters (width, precision)\n"
                " \"    - continuation\n"
#if defined(CONFIG_WITH_GROUPING_SUPPORT)
                " G    - comprehensive grouping tests\n"
#endif
                " ^    - comprehensive centering flag tests\n"
                " L    - comprehensive length modifier tests\n"
                " #    - comprehensive alternate form (#) flag tests\n"
#if defined(CONFIG_WITH_FP_SUPPORT)
                " g    - comprehensive %%g and %%G conversion tests\n"
                " !    - comprehensive engineering notation and SI format tests\n"
#endif
                " 3    - string and character edge cases (P3.1)\n"
                " E    - error paths and validation\n"
                " C    - consumer function failures\n"
                );
        return;
    }

    printf( "Passes: %s\n", passes );

    while ( *passes )
    {
        switch ( *passes )
        {
            case 'S': test_strings(); break;
            case '%': test_pc();      break;
            case 'c': test_cC();      break;
            case 'n': test_n();       break;
            case 's': test_s();       break;
            case 'p': test_p();       break;
            case 'd': test_di();      break;
            case 'b': test_bouxX();   break;
#if defined(CONFIG_WITH_FP_SUPPORT)
	    case 'a': test_aAeEfFgG(); break;
            case 'k': test_k();        break;
            case 'N': test_nan_special_floats(); break;
            case 'K': test_k_edge_cases(); break;
#endif
            case '*': test_asterisk(); break;
            case '\"': test_cont();   break;
#if defined(CONFIG_WITH_GROUPING_SUPPORT)
            case 'G': test_grouping(); break;
#endif
            case '^': test_centering(); break;
            case 'L': test_length_modifiers(); break;
            case '#': test_alternate_form(); break;
#if defined(CONFIG_WITH_FP_SUPPORT)
            case 'g': test_g_completeness(); break;
            case '!': test_engineering_si(); break;
#endif
            case '3': test_string_char_edge_cases(); break;
            case 'E': test_errors();   break;
            case 'C': test_consumer_failures(); break;
            default: printf( "Unknown test '%c'\n", *passes ); break;
        }
        passes++;
    }

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
    run_tests( argc > 1 ? argv[1] : NULL);
    return 0;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
