/* ****************************************************************************
 * TinyFormat - minimal lightweight string formatting library for 16-bit CPUs
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

#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>

/** Query the environment about what capabilities are available **/
#include "format_config.h"

#if defined(CONFIG_HAVE_LIBC)
  #include <string.h>
  #include <ctype.h>
#endif

/*****************************************************************************/
/* Project Includes                                                          */
/*****************************************************************************/

/** Pull in our public header **/
#include "format.h"

/*****************************************************************************/
/* Macros, constants                                                         */
/*****************************************************************************/

/**
    Define the field flags
**/
#define FSPACE          ( 0x01U )
#define FPLUS           ( 0x02U )
#define FMINUS          ( 0x04U )
#define FZERO           ( 0x10U )
#define F_IS_SIGNED     ( 0x80U )

/**
    Set limits.
**/
#define MAXWIDTH        ( 80 )
#define MAXPREC         ( 80 )
#define BUFLEN          ( 16 )   /* Must be long enough for 16-bit pointers
                                  * in binary */

/**
    Return the maximum/minimum of two scalar values.
**/
#define MAX(a,b)        ( (a) > (b) ? (a) : (b) )
#define MIN(a,b)        ( (a) < (b) ? (a) : (b) )

/**
    Return the absolute value of a signed scalar value.
**/
#define ABS(a)          ( (a) < 0 ? -(a) : (a) )

/*****************************************************************************/
/**
    Wrapper macro around isdigit().
**/
#if defined(CONFIG_HAVE_LIBC)
    #define ISDIGIT(c)      (isdigit((int)c))
#else
    #define ISDIGIT(c)      (('0' <= (c) && (c) <= '9') ? 1 : 0)
#endif

/*****************************************************************************/
/**
    Wrapper macro around strlen().
**/
#if defined(CONFIG_HAVE_LIBC)
    #define STRLEN(s)       (strlen(s))
#else
    #define STRLEN(s)       (xx_strlen(s))
#endif

/*****************************************************************************/
/**
    Wrapper macro around strchr().
**/
#if defined(CONFIG_HAVE_LIBC)
    #define STRCHR(s,c)     (strchr((s),(c)))
#else
    #define STRCHR(s,c)     (xx_strchr((s),(c)))
#endif

/*****************************************************************************/
/* Data types                                                                */
/*****************************************************************************/

/**
    Describe a format specification.
**/
typedef struct {
    unsigned int    nChars; /**< number of chars emitted so far     **/
    uint8_t         flags;  /**< flags                              **/
    uint8_t         width;  /**< width                              **/
    int8_t          prec;   /**< precision, -1 == default precision **/
} T_FormatSpec;

/*****************************************************************************/
/* Private function prototypes.  Declare as static.                          */
/*****************************************************************************/

static int do_conv( T_FormatSpec *, va_list *, char,
                    void *(*)(void *, const char *, size_t), void * * );

static int emit( const char *, size_t,
                 void * (*)(void *, const char *, size_t ), void * * );

static int pad( char, size_t,
                void * (*)(void *, const char *, size_t), void * * );

static int gen_out( void *(*)(void *, const char *, size_t), void * *,
                    size_t, const char *, size_t, size_t,
                    const char *, size_t, size_t );

static void calc_space_padding( T_FormatSpec *, size_t, size_t *, size_t * );

/* Only declare these prototypes in a freestanding environment */
#if !defined(CONFIG_HAVE_LIBC)
static size_t xx_strlen( const char * );
static char * xx_strchr( const char *, int );
#endif

#if defined(CONFIG_NEED_LOCAL_MEMCPY)
static void * memcpy( void *, const void *, size_t );
#endif

/** Conversion handlers **/
static int do_conv_c( T_FormatSpec *, va_list *,
                      void * (*)(void *, const char *, size_t), void * * );

static int do_conv_s( T_FormatSpec *, va_list *,
                      void * (*)(void *, const char *, size_t), void * * );

static int do_conv_numeric( T_FormatSpec *, va_list *, char,
                            void * (*)(void *, const char *, size_t), void * *,
                            unsigned int );

/*****************************************************************************/
/* Private functions.  Declare as static.                                    */
/*****************************************************************************/

/*****************************************************************************/
/**
    Local implementation of strlen().

    @param s        Pointer to string.

    @return Length of string s excluding the terminating null character.
**/
#if !defined(CONFIG_HAVE_LIBC)
static size_t xx_strlen( const char *s )
{
    const char *p;
    for ( p = s; *p != '\0'; p++ )
        ;
    return (size_t)(p-s);
}
#endif

/*****************************************************************************/
/**
    Local implementation of strchr().

    @param s        Pointer to pattern string.
    @param c        Character to find in s.

    @return Address of first matching character, or NULL if not found.
**/
#if !defined(CONFIG_HAVE_LIBC)
static char * xx_strchr( const char *s, int c )
{
    char ch = (char)c;
    for ( ; *s != ch; s++ )
        if ( *s == '\0' )
            return NULL;
    return (char *)s;
}
#endif

/*****************************************************************************/
/**
    Emit @p n characters from string @p s.

    @param s        Pointer to source string
    @param n        Number of characters to emit
    @param cons     Pointer to consumer function
    @param parg     Pointer to opaque pointer arg for @p cons

    @return 0 if successful, or EXBADFORMAT if failed.
**/
static int emit( const char *s, size_t n,
                 void * (* cons)(void *, const char *, size_t), void * * parg )
{
    if ( n > 0 && ( *parg = ( *cons )( *parg, s, n ) ) == NULL )
        return EXBADFORMAT;
    else
        return 0;
}

/*****************************************************************************/
/**
    Emit @p n padding characters @p c.

    @param c        Padding character.
    @param n        Number of padding characters to emit.
    @param cons     Pointer to consumer function
    @param parg     Pointer to opaque pointer arg for @p cons

    @return number of emitted characters, or EXBADFORMAT if failed.
**/
static int pad( char c, size_t n,
                void * (* cons)(void *, const char *, size_t), void * * parg )
{
    for ( size_t i = 0; i < n; i++ )
    {
        if ( ( *parg = ( *cons )( *parg, &c, 1 ) ) == NULL )
            return EXBADFORMAT;
    }
    return (int)n;
}

/*****************************************************************************/
/**
    Generate output with spacing, zero padding and prefixing.

    @param cons     Pointer to consumer function
    @param parg     Pointer to opaque pointer arg for @p cons
    @param ps1      Number of space padding to prefix
    @param pfx_s    Pointer to prefix string
    @param pfx_n    Length of prefix
    @param pz       Number of zero padding to prefix
    @param e_s      Pointer to emitted string
    @param e_n      Length of emitted string
    @param ps2      Number of space padding to suffix

    @return Number of emitted characters, or EXBADFORMAT if failure
**/
static int gen_out( void *(*cons)(void *, const char *, size_t), void * * parg,
                    size_t ps1,
                    const char *pfx_s, size_t pfx_n,
                    size_t pz,
                    const char *e_s, size_t e_n,
                    size_t ps2 )
{
    size_t n = ps1 + pz + e_n + ps2;

    if ( pad( ' ', ps1, cons, parg ) < 0 )
        return EXBADFORMAT;

    if ( pfx_s )
    {
        if ( emit( pfx_s, pfx_n, cons, parg ) < 0 )
            return EXBADFORMAT;
        n += pfx_n;
    }

    if ( pad( '0', pz, cons, parg ) < 0 )
        return EXBADFORMAT;

    if ( emit( e_s, e_n, cons, parg ) < 0 )
        return EXBADFORMAT;

    if ( pad( ' ', ps2, cons, parg ) < 0 )
        return EXBADFORMAT;

    return (int)n;
}

/*****************************************************************************/
/**
    Calculate the left and right space padding amount.

    @param pspec        Pointer to format specification.
    @param length       Length of item
    @param ps1          Pointer to store left padding.  May be NULL.
    @param ps2          Pointer to store right padding. May be NULL.
**/
static void calc_space_padding( T_FormatSpec * pspec,
                                size_t         length,
                                size_t *       ps1,
                                size_t *       ps2 )
{
    size_t left = 0, right = 0, width = 0;

    if ( length < pspec->width )
        width = pspec->width - length;

    if ( pspec->flags & FMINUS )
        right = width;
    else
        left = width;

    *ps1 = left;
    *ps2 = right;
}

/*****************************************************************************/
/**
    Process the %c conversion.

    @param pspec    Pointer to format specification.
    @param ap       Reference to optional format arguments list.
    @param cons     Pointer to consumer function.
    @param parg     Pointer to opaque pointer updated by cons.

    @return Number of emitted characters, or EXBADFORMAT if failure
**/
static int do_conv_c( T_FormatSpec * pspec,
                      va_list *      ap,
                      void *      (* cons)(void *, const char *, size_t),
                      void * *       parg )
{
    char cc;
    unsigned int rep;

    cc = (char)va_arg( *ap, int );

    /* apply default precision */
    rep = (unsigned int)MAX( 1, pspec->prec );

    if ( pad( cc, rep, cons, parg ) < 0 )
        return EXBADFORMAT;

    return rep;
}

/*****************************************************************************/
/**
    Process a %s conversion.

    @param pspec    Pointer to format specification.
    @param ap       Reference to optional format arguments list..
    @param cons     Pointer to consumer function.
    @param parg     Pointer to opaque pointer updated by cons.

    @return Number of emitted characters, or EXBADFORMAT if failure
**/
static int do_conv_s( T_FormatSpec * pspec,
                      va_list *      ap,
                      void *      (* cons)(void *, const char *, size_t),
                      void * *       parg )
{
    size_t length = 0;
    size_t ps1 = 0, ps2 = 0;

    const char *s = va_arg( *ap, const char * );

    if ( s == NULL )
        s = "(null)";

    length = STRLEN( s );
    if ( pspec->prec >= 0 )
        length = (size_t)MIN( pspec->prec, (int)length );

    calc_space_padding( pspec, length, &ps1, &ps2 );

    return gen_out( cons, parg, ps1, NULL, 0, 0, s, length, ps2 );
}

/*****************************************************************************/
/**
    Process the numeric conversions (%b, %d, %u, %x, %X).

    @param pspec    Pointer to format specification.
    @param ap       Reference to optional format arguments list.
    @param code     Conversion specifier code.
    @param cons     Pointer to consumer function.
    @param parg     Pointer to opaque pointer updated by cons.

    @return Number of emitted characters, or EXBADFORMAT if failure
**/
static int do_conv_numeric( T_FormatSpec * pspec,
                            va_list *      ap,
                            char           code,
                            void *      (* cons)(void *, const char *, size_t),
                            void * *       parg,
                            unsigned int base )
{
    size_t length = 0;
    size_t numWidth, digitWidth;
    char numBuffer[BUFLEN];
    size_t ps1 = 0, ps2 = 0, pz = 0, pfx_n = 0;
    const char * pfx_s = NULL;
    uint16_t uv;
    char prefix[1];
    static const char digits[] = "0123456789ABCDEF";

    /* Get the value.
     * Signed values need special handling for negative values and the
     *  extra options for sign output which don't apply to the unsigned
     *  values.
     */
    if ( pspec->flags & F_IS_SIGNED )
    {
        int16_t v;

        v = (int16_t)va_arg( *ap, int );

        /* Get absolute value */
        uv = (uint16_t)ABS(v);

        /* Based on original sign and flags work out any prefix */
        prefix[0] = '\0';
        if ( v < 0 )
            prefix[0]     = '-';
        else if ( pspec->flags & FPLUS )
            prefix[0]     = '+';
        else if ( pspec->flags & FSPACE )
            prefix[0]     = ' ';

        if ( prefix[0] != '\0' )
        {
            length++;
            pfx_s = prefix;
            pfx_n = 1;
        }
    }
    else
    {
        uv = (uint16_t)va_arg( *ap, unsigned int );
    }

    for( numWidth = 0; uv != 0; )
    {
        uint16_t div_uv  = uv / base;
        uint16_t div_rem = uv % base;
        char cc = digits[div_rem];

        /* convert to lower case? */
        if ( code == 'x' )
            cc |= 0x20;

        ++numWidth;
        numBuffer[sizeof(numBuffer) - numWidth] = cc;
        uv = div_uv;
    }

    digitWidth = numWidth;

    /* apply default precision */
    if ( pspec->prec < 0 )
        pspec->prec = 1;
    else
        pspec->flags &= ~FZERO; /* Ignore if precision specified */

    numWidth = MAX( numWidth, pspec->prec );
    length  += numWidth;

    calc_space_padding( pspec, length, &ps1, &ps2 );

    /* Convert space padding into zero padding if we have the ZERO flag */
    pz = numWidth - digitWidth;
    if ( pspec->flags & FZERO )
    {
        pz += ps1;
        ps1 = 0;
    }

    return gen_out( cons, parg,
                    ps1,
                    pfx_s, pfx_n,
                    pz,
                    &numBuffer[sizeof(numBuffer) - digitWidth], digitWidth,
                    ps2 );
}

/*****************************************************************************/
/**
    Handle a single format conversion for a given type.

    @param pspec    Pointer to format specification.
    @param ap       Reference to optional format arguments list.
    @param code     Conversion specifier code.
    @param cons     Pointer to consumer function.
    @param parg     Pointer to opaque pointer updated by cons.

    @return Number of emitted characters, or EXBADFORMAT if failure
**/
static int do_conv( T_FormatSpec * pspec,
                    va_list *      ap,
                    char           code,
                    void *      (* cons)(void *, const char *, size_t),
                    void * *       parg )
{
    unsigned int base = 0;

    if ( code == '%' )
        return pad( code, 1, cons, parg );

    if ( code == 'c' )
        return do_conv_c( pspec, ap, cons, parg );

    if ( code == 's' )
        return do_conv_s( pspec, ap, cons, parg );

    /* -------------------------------------------------------------------- */

    /* The '%p' conversion is a meta-conversion, which we convert to a
     *  pre-defined format.  In this case we convert it to "%N.NX"
     *  where N is double the machine-word size, as each byte converts into
     *  two characters.
     */
    if ( code == 'p' )
    {
        code          = 'X';
        pspec->width  = 4;
        pspec->prec   = 4;
    }

    /* -------------------------------------------------------------------- */

    /* The '%d' conversion is decimal (base 10).
     *  We set the F_IS_SIGNED internal flag to guide later
     *  processing.
     */
    if ( code == 'd' )
    {
        pspec->flags |= F_IS_SIGNED;
        base = 10;
    }

    if ( code == 'u' )
        base = 10;

    if ( code == 'x' || code == 'X' )
        base = 16;

    if ( code == 'b' )
        base = 2;

    if ( base > 1 )
        return do_conv_numeric( pspec, ap, code, cons, parg, base );

    return EXBADFORMAT;
}

/*****************************************************************************/
/**
    Interpret format specification passing formatted text to consumer function.

    Executes the printf-compatible format specification fmt, referring to
    optional arguments ap.  Any output text is passed to caller-provided
    consumer function cons, which also takes caller-provided opaque pointer
    arg.

    Note: floating point is not supported.

    @param cons     Pointer to caller-provided consumer function.
    @param arg      Opaque pointer passed through to cons.
    @param fmt      Printf-compatible format specifier.
    @param apx      List of optional format string arguments.

    @return Number of characters sent to @a cons, or EXBADFORMAT.
**/
int format( void *    (* cons) (void *, const char * , size_t),
            void *       arg,
            const char * fmt,
            va_list      apx )
{
    T_FormatSpec fspec;
    char           c;
    va_list        ap;
    
    /* Setup varargs -- must va_end( ap ) before exit !! */
    va_copy( ap, apx );

    if ( fmt == NULL )
        goto exit_badformat;

    fspec.nChars = 0;

    while ( ( c = *fmt ) )
    {
        /* scan for % or \0 */
        {
            size_t n = 0;
            const char *s = fmt;

            for ( ; *s && *s != '%'; s++ )
                n++;

            if ( emit( fmt, n, cons, &arg ) < 0 )
                goto exit_badformat;

            fspec.nChars += n;
            fmt = s;
        }

        if ( *fmt )
        {
            /* found conversion specifier */
            char *t;
            int nn;
            static const char fchar[] = {" +-0"};
            static const unsigned int fbit[] = {
                FSPACE, FPLUS, FMINUS, FZERO, 0};

            fmt++; /* skip the % sign */

            /* process conversion flags */
            for ( fspec.flags = 0;
                  ( c = *fmt ) && (t = STRCHR(fchar, c)) != NULL;
                  fmt++ )
            {
                fspec.flags |= fbit[t - fchar];
            }

            /* process width */
            for ( fspec.width = 0;
                  ( c = *fmt ) && ISDIGIT( c ) && fspec.width < MAXWIDTH;
                  fmt++ )
            {
                fspec.width = fspec.width * 10 + c - '0';
            }

            if ( fspec.width > MAXWIDTH )
                goto exit_badformat;

            /* process precision */
            if ( *fmt != '.' )
                fspec.prec = -1; /* precision is missing */
            else
            {
                fmt++;
                for ( fspec.prec = 0;
                      ( c = *fmt ) && ISDIGIT( c ) && fspec.prec < MAXPREC;
                      fmt++ )
                {
                    fspec.prec = fspec.prec * 10 + c - '0';
                }
                if ( fspec.prec > MAXPREC )
                    goto exit_badformat;
            }

            /* Continuation */
            c = *fmt;
            if ( c == '\0' )
            {
                fmt = va_arg( ap, const char * );
                continue;
            }

            /* now process the conversion type */
            nn = do_conv( &fspec, &ap, c, cons, &arg );
            if ( nn < 0 )
                goto exit_badformat;
            else
                fspec.nChars += (unsigned int)nn;

            fmt++;
        }
    }

    va_end( ap );
    return (int)fspec.nChars;

exit_badformat:
    va_end( ap );
    return EXBADFORMAT;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
