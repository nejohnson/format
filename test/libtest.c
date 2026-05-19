/* ***************************************************************************
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
#include <string.h>

/*****************************************************************************/
/* Project Includes                                                          */
/*****************************************************************************/

#include "printf.h"

/*****************************************************************************/
/* Test infrastructure                                                       */
/*****************************************************************************/

#define BUF_SZ  256

static unsigned int num_fail = 0;

/**
    Check that two integers are equal.
**/
#define CHECK_INT(got, exp) do {                                              \
    int _g = (int)(got), _e = (int)(exp);                                    \
    if ( _g == _e )                                                           \
        printf( "[Test @ %3d] PASS\n", __LINE__ );                           \
    else {                                                                    \
        printf( "[Test @ %3d] FAIL: got %d, expected %d\n",                  \
                __LINE__, _g, _e );                                           \
        num_fail++;                                                           \
    }                                                                         \
} while(0)

/**
    Check that two strings are equal.
**/
#define CHECK_STR(got, exp) do {                                              \
    const char *_g = (got), *_e = (exp);                                     \
    if ( strcmp( _g, _e ) == 0 )                                             \
        printf( "[Test @ %3d] PASS\n", __LINE__ );                           \
    else {                                                                    \
        printf( "[Test @ %3d] FAIL: got \"%s\", expected \"%s\"\n",          \
                __LINE__, _g, _e );                                           \
        num_fail++;                                                           \
    }                                                                         \
} while(0)

/**
    Check that a pointer is not NULL.
**/
#define CHECK_NONNULL(p) do {                                                 \
    if ( (void *)(p) != NULL )                                               \
        printf( "[Test @ %3d] PASS\n", __LINE__ );                           \
    else {                                                                    \
        printf( "[Test @ %3d] FAIL: unexpected NULL\n", __LINE__ );          \
        num_fail++;                                                           \
    }                                                                         \
} while(0)

/**
    Check that two pointers are equal.
**/
#define CHECK_PTR_EQ(got, exp) do {                                           \
    if ( (void *)(got) == (void *)(exp) )                                    \
        printf( "[Test @ %3d] PASS\n", __LINE__ );                           \
    else {                                                                    \
        printf( "[Test @ %3d] FAIL: pointers unexpectedly differ\n",         \
                __LINE__ );                                                   \
        num_fail++;                                                           \
    }                                                                         \
} while(0)

/**
    Check that two pointers differ.
**/
#define CHECK_PTR_NE(got, exp) do {                                           \
    if ( (void *)(got) != (void *)(exp) )                                    \
        printf( "[Test @ %3d] PASS\n", __LINE__ );                           \
    else {                                                                    \
        printf( "[Test @ %3d] FAIL: pointers unexpectedly equal\n",          \
                __LINE__ );                                                   \
        num_fail++;                                                           \
    }                                                                         \
} while(0)

/*****************************************************************************/
/* v-variant call helpers                                                    */
/**
    Each helper accepts variadic arguments and forwards them through the
    corresponding v-prefixed function, exercising the va_list code path.
**/
/*****************************************************************************/

static int do_vsprintf( char *buf, const char *fmt, ... )
{
    va_list ap;
    int r;
    va_start( ap, fmt );
    r = vsprintf( buf, fmt, ap );
    va_end( ap );
    return r;
}

static int do_vsnprintf( char *buf, size_t n, const char *fmt, ... )
{
    va_list ap;
    int r;
    va_start( ap, fmt );
    r = vsnprintf( buf, n, fmt, ap );
    va_end( ap );
    return r;
}

static int do_vscprintf( const char *fmt, ... )
{
    va_list ap;
    int r;
    va_start( ap, fmt );
    r = vscprintf( fmt, ap );
    va_end( ap );
    return r;
}

static int do_vasprintf( char **strp, const char *fmt, ... )
{
    va_list ap;
    int r;
    va_start( ap, fmt );
    r = vasprintf( strp, fmt, ap );
    va_end( ap );
    return r;
}

static char * do_vasnprintf( char *resultbuf, size_t *lengthp,
                              const char *fmt, ... )
{
    va_list ap;
    char *result;
    va_start( ap, fmt );
    result = vasnprintf( resultbuf, lengthp, fmt, ap );
    va_end( ap );
    return result;
}

static int do_vprintf( const char *fmt, ... )
{
    va_list ap;
    int r;
    va_start( ap, fmt );
    r = vprintf( fmt, ap );
    va_end( ap );
    return r;
}

/*****************************************************************************/
/* Test groups                                                               */
/*****************************************************************************/

/*****************************************************************************/
/**
    Tests for sprintf / vsprintf.

    These functions are thin wrappers around format() with an unbounded buffer
    consumer.  Tests verify content, null termination, and return value.
**/
static void test_sprintf_group( void )
{
    char buf[BUF_SZ];
    int r;

    printf( "Testing sprintf / vsprintf\n" );

    r = sprintf( buf, "hello" );
    CHECK_STR( buf, "hello" );
    CHECK_INT( r, 5 );

    r = sprintf( buf, "%d", 42 );
    CHECK_STR( buf, "42" );
    CHECK_INT( r, 2 );

    r = sprintf( buf, "%s %s", "hello", "world" );
    CHECK_STR( buf, "hello world" );
    CHECK_INT( r, 11 );

    /* vsprintf: exercise the va_list path */
    r = do_vsprintf( buf, "<%d>", -99 );
    CHECK_STR( buf, "<-99>" );
    CHECK_INT( r, 5 );
}

/*****************************************************************************/
/**
    Tests for snprintf / vsnprintf.

    Key behaviours: correct output when result fits; truncation leaves the
    buffer null-terminated and writes at most n-1 content characters; n=0
    writes nothing and still returns the full character count.

    Note: when the output is truncated this implementation returns n-1 rather
    than the full character count that C99 specifies.  The buffer content is
    correct in all cases.
**/
static void test_snprintf_group( void )
{
    char buf[BUF_SZ];
    int r;

    printf( "Testing snprintf / vsnprintf\n" );

    /* Fits comfortably */
    r = snprintf( buf, BUF_SZ, "hello" );
    CHECK_STR( buf, "hello" );
    CHECK_INT( r, 5 );

    /* Exact fit: n == len + 1 (just enough room for content + null) */
    r = snprintf( buf, 6, "hello" );
    CHECK_STR( buf, "hello" );
    CHECK_INT( r, 5 );

    /* Truncation: "hello world" (11 chars) into a 5-byte buffer */
    /* buf holds 4 content chars + null; return value is n-1 = 4  */
    /* Use %s so the compiler cannot perform compile-time truncation analysis */
    memset( buf, '#', BUF_SZ );
    r = snprintf( buf, 5, "%s", "hello world" );
    CHECK_STR( buf, "hell" );
    CHECK_INT( r, 4 );
    /* Verify null terminator is at buf[4] and buffer is not overrun */
    CHECK_INT( buf[4], '\0' );
    CHECK_INT( buf[5], '#' );

    /* n == 0: nothing written, returns the full character count */
    memset( buf, '#', BUF_SZ );
    r = snprintf( buf, 0, "hello" );
    CHECK_INT( r, 5 );
    CHECK_INT( buf[0], '#' );  /* buffer must be untouched */

    /* vsnprintf: exercise the va_list path */
    r = do_vsnprintf( buf, BUF_SZ, "<%d>", 7 );
    CHECK_STR( buf, "<7>" );
    CHECK_INT( r, 3 );
}

/*****************************************************************************/
/**
    Tests for scprintf / vscprintf.

    These functions run format() with a null consumer and return the character
    count directly.  The count must match what sprintf would write.
**/
static void test_scprintf_group( void )
{
    char buf[BUF_SZ];
    int r_sprintf, r_scprintf;

    printf( "Testing scprintf / vscprintf\n" );

    r_sprintf  = sprintf(  buf, "hello" );
    r_scprintf = scprintf(      "hello" );
    CHECK_INT( r_scprintf, r_sprintf );

    r_sprintf  = sprintf(  buf, "%d items", 42 );
    r_scprintf = scprintf(      "%d items", 42 );
    CHECK_INT( r_scprintf, r_sprintf );

    r_sprintf  = sprintf(  buf, "%s = %d", "answer", 99 );
    r_scprintf = scprintf(      "%s = %d", "answer", 99 );
    CHECK_INT( r_scprintf, r_sprintf );

    /* vscprintf: exercise the va_list path */
    r_sprintf   = sprintf(     buf, "<%s>", "test" );
    r_scprintf  = do_vscprintf(      "<%s>", "test" );
    CHECK_INT( r_scprintf, r_sprintf );
}

/*****************************************************************************/
/**
    Tests for asprintf / vasprintf.

    These functions allocate a buffer sized exactly for the result.  Tests
    verify the allocated content, the return value, and that the buffer must
    be freed by the caller.
**/
static void test_asprintf_group( void )
{
    char *p = NULL;
    int r;

    printf( "Testing asprintf / vasprintf\n" );

    r = asprintf( &p, "hello %s", "world" );
    CHECK_NONNULL( p );
    CHECK_STR( p, "hello world" );
    CHECK_INT( r, 11 );
    free( p );
    p = NULL;

    r = asprintf( &p, "%d", -42 );
    CHECK_NONNULL( p );
    CHECK_STR( p, "-42" );
    CHECK_INT( r, 3 );
    free( p );
    p = NULL;

    /* vasprintf: exercise the va_list path */
    r = do_vasprintf( &p, "n=%d", 7 );
    CHECK_NONNULL( p );
    CHECK_STR( p, "n=7" );
    CHECK_INT( r, 3 );
    free( p );
}

/*****************************************************************************/
/**
    Tests for asnprintf / vasnprintf.

    Three paths: result fits in the caller's buffer (no allocation); result
    does not fit (malloc used, returned pointer differs from resultbuf); and
    NULL resultbuf (always allocates, equivalent to asprintf).
**/
static void test_asnprintf_group( void )
{
    char fitbuf[32];
    char smallbuf[5];
    char *result;
    size_t len;

    printf( "Testing asnprintf / vasnprintf\n" );

    /* Fits: "hello" (5 chars) into a 32-byte buffer */
    len    = sizeof( fitbuf );
    result = asnprintf( fitbuf, &len, "hello" );
    CHECK_PTR_EQ( result, fitbuf );     /* no allocation */
    CHECK_STR( result, "hello" );
    CHECK_INT( (int)len, 5 );

    /* Does not fit: "hello world" (11 chars) into a 5-byte buffer */
    len    = sizeof( smallbuf );        /* 5: not enough for 11 + null */
    result = asnprintf( smallbuf, &len, "hello world" );
    CHECK_NONNULL( result );
    CHECK_PTR_NE( result, smallbuf );   /* new allocation */
    CHECK_STR( result, "hello world" );
    CHECK_INT( (int)len, 11 );
    free( result );

    /* NULL resultbuf: always allocates (equivalent to asprintf) */
    len    = 0;
    result = asnprintf( NULL, &len, "%d", 42 );
    CHECK_NONNULL( result );
    CHECK_STR( result, "42" );
    CHECK_INT( (int)len, 2 );
    free( result );

    /* vasnprintf: exercise the va_list path (fits case) */
    len    = sizeof( fitbuf );
    result = do_vasnprintf( fitbuf, &len, "<%s>", "ok" );
    CHECK_PTR_EQ( result, fitbuf );
    CHECK_STR( result, "<ok>" );
    CHECK_INT( (int)len, 4 );
}

/*****************************************************************************/
/**
    Tests for printf / vprintf.

    These functions route output through the system putchar() so their content
    cannot be captured automatically here.  Tests verify only the return value
    (character count) to confirm the function links and calls format() correctly.
    The formatted text will appear on stdout inline with the test output below.
**/
static void test_printf_group( void )
{
    int r;

    printf( "Testing printf / vprintf\n" );
    printf( "  [printf output]  " );

    r = printf( "<%d>\n", 99 );
    CHECK_INT( r, 5 );  /* "<99>\n" is 5 chars */

    printf( "  [vprintf output] " );
    r = do_vprintf( "<%s>\n", "ok" );
    CHECK_INT( r, 5 );  /* "<ok>\n" is 5 chars */
}

/*****************************************************************************/
/* Main                                                                      */
/*****************************************************************************/

int main( void )
{
    printf( ":: lib test harness ::\n" );

    test_sprintf_group();
    test_snprintf_group();
    test_scprintf_group();
    test_asprintf_group();
    test_asnprintf_group();
    test_printf_group();

    printf( "-----------------------\n"
            "Summary: %s (%u failure%s)\n",
            num_fail ? "FAIL" : "PASS",
            num_fail,
            num_fail == 1 ? "" : "s" );

    return (int)num_fail;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
