/* ****************************************************************************
 * Format - lightweight string formatting library.
 * Copyright (C) 2010-2026, Neil Johnson
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
#define FHASH           ( 0x08U )
#define FZERO           ( 0x10U )
#define FBANG           ( 0x20U )
#define FCARET          ( 0x40U )
#define F_IS_SIGNED     ( 0x80U )

/**
    Some length qualifiers are doubled-up (e.g., "hh").

    This little hack works on the basis that all the valid length qualifiers
    (h,l,j,z,t,L) ASCII values are all even, so we use the LSB to tag double
    qualifiers.  I'm not sure if this was the intent of the spec writers but
    it is certainly convenient!  If this ever changes then we need to review
    this hack and come up with something else.
**/
#define DOUBLE_QUAL(q)  ( (q) | 1 )

/**
    Set limits.
**/
#define MAXWIDTH        CONFIG_MAXWIDTH
#define MAXPREC         CONFIG_MAXPREC
#define MAXBASE         ( 36 )
#define BUFLEN          CONFIG_BUFLEN

/**
    Return the maximum/minimum of two scalar values.
**/
#define MAX(a,b)        ( (a) > (b) ? (a) : (b) )
#define MIN(a,b)        ( (a) < (b) ? (a) : (b) )

/**
    Return the number of elements in a static array.
**/
#define NELEMS(a)       ( sizeof(a) / sizeof(*(a)) )

/**
    Return the absolute value of a signed scalar value.
**/
#define ABS(a)          ( (a) < 0 ? -(a) : (a) )

/** A generic macro to read a character from memory **/
#define READ_CHAR(p)   (*(const char *)(p))

/** It is nonsensical to increment a void pointer directly, so we kind-of-cheat
    by casting it to a char pointer and then incrementing. **/
#define INC_VOID_PTR(v)     ( (v) = ((const char *)(v))+1 )
#define DEC_VOID_PTR(v)     ( (v) = ((const char *)(v))-1 )
#define MOVE_VOID_PTR(v,n)  ( (v) = ((const char *)(v))+(n) )

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
/**
    Debugging aids.  Only intended for debugging "format" itself, using
    standard out for dumping internal state.  Not for use in target systems.
**/
/* #define FORMAT_DEBUG */
#ifdef FORMAT_DEBUG
#include <stdio.h>
#define DEBUG_LOG(fmt,val)  printf((fmt),(val))
#else
#define DEBUG_LOG(fmt,val)  ((void)0)
#endif

/*****************************************************************************/
/* Data types                                                                */
/*****************************************************************************/

/**
    Describe a format specification.
**/
typedef struct {
    unsigned int    nChars; /**< number of chars emitted so far     **/
    unsigned int    flags;  /**< flags                              **/
    unsigned int    width;  /**< width                              **/
    int             prec;   /**< precision, -1 == default precision **/
    unsigned int    base;   /**< numeric base                       **/
    char            qual;   /**< length qualifier                   **/
    char            repchar;/**< Repetition character               **/
#if defined(CONFIG_WITH_GROUPING_SUPPORT)
    struct {
        const void *  ptr;  /**< ptr to grouping specification      **/
        size_t        len;  /**< length of grouping spec            **/
    } grouping;
#endif
#if defined(CONFIG_WITH_FP_SUPPORT)
    struct {
        int w_int;          /**< fixed-point integer field width    **/
        int w_frac;         /**< fixed-point fractional field width **/
    } xp;
#endif
} T_FormatSpec;

/*****************************************************************************/
/* Private Data.  Declare as static.                                         */
/*****************************************************************************/

/**
    Padding strings, used by gen_out().
**/
#define PAD_STRING_LEN      ( 16 )
static const char spaces[] = "                ";
static const char zeroes[] = "0000000000000000";

/*****************************************************************************/
/* Private function prototypes.  Declare as static.                          */
/*****************************************************************************/

static int do_conv( T_FormatSpec *, va_list *, char,
                    void *(*)(void *, const char *, size_t), void * * );

static int emit( const char *, size_t,
                 void * (*)(void *, const char *, size_t ), void * * );

static int pad( const char *, size_t,
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
static int do_conv_n( T_FormatSpec *, va_list * );

static int do_conv_c( T_FormatSpec *, va_list *, char,
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
    if ( ( *parg = ( *cons )( *parg, s, n ) ) == NULL )
        return EXBADFORMAT;
    else
        return 0;
}

/*****************************************************************************/
/**
    Emit @p n padding characters from padding string @p s.

    @param s        Name of padding string.
    @param n        Number of padding characters to emit.
    @param cons     Pointer to consumer function
    @param parg     Pointer to opaque pointer arg for @p cons

    @return 0 if successful, or EXBADFORMAT if failed.
**/
static int pad( const char *s, size_t n,
                void * (* cons)(void *, const char *, size_t), void * * parg )
{
    while ( n > 0 )
    {
        size_t j = (size_t)MIN( PAD_STRING_LEN, n );
        if ( emit( s, j, cons, parg ) < 0 )
            return EXBADFORMAT;
        n -= j;
    }
    return 0;
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
    size_t n = 0;

    if ( ps1 && pad( spaces, ps1, cons, parg ) < 0 )
        return EXBADFORMAT;
    n += ps1;

    if ( pfx_s && pfx_n )
    {
        if ( emit( pfx_s, pfx_n, cons, parg ) < 0 )
            return EXBADFORMAT;
        n += pfx_n;
    }

    if ( pz && pad( zeroes, pz, cons, parg ) < 0 )
        return EXBADFORMAT;
    n += pz;

    if ( e_n && emit( e_s, e_n, cons, parg ) < 0 )
        return EXBADFORMAT;
    n += e_n;

    if ( ps2 && pad( spaces, ps2, cons, parg ) < 0 )
        return EXBADFORMAT;
    n += ps2;

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

    if ( pspec->flags & FCARET )
    {
        size_t tot = left + right;
        left       = ( tot + !( pspec->flags & FMINUS ) ) / 2;
        right      = tot - left;
    }

    if ( ps1 ) *ps1 = left;
    if ( ps2 ) *ps2 = right;
}

/*****************************************************************************/
/**
    Floating Point code is in a separate source file for clarity.
    Pull it in if required.
**/
#if defined(CONFIG_WITH_FP_SUPPORT)
#include "format_fp.c"
#endif

/*****************************************************************************/
/**
    Process a %n conversion.

    @param pspec    Pointer to format specification.
    @param ap       Reference to optional format arguments list.

    @return 0 as no characters are emitted.
**/
static int do_conv_n( T_FormatSpec * pspec,
                      va_list *      ap )
{
    void *vp = va_arg( *ap, void * );

    if ( vp )
    {
        if ( pspec->qual == 'h' )
            *(short *)vp = (short)pspec->nChars;
        else if ( pspec->qual == DOUBLE_QUAL( 'h' ) )
            *(signed char *)vp = (signed char)pspec->nChars;
        else if ( pspec->qual == 'l' )
            *(long *)vp = (long)pspec->nChars;
#if defined(CONFIG_WITH_LONG_LONG_SUPPORT)
        else if ( pspec->qual == DOUBLE_QUAL( 'l' ) )
            *(long long *)vp = (long long)pspec->nChars;
#endif
        else if ( pspec->qual == 'j' )
            *(intmax_t *)vp = (intmax_t)pspec->nChars;
        else if ( pspec->qual == 'z' )
            *(size_t *)vp = (size_t)pspec->nChars;
        else if ( pspec->qual == 't' )
            *(ptrdiff_t *)vp = (ptrdiff_t)pspec->nChars;
        else
            *(int *)vp = (int)pspec->nChars;
    }
    return 0;
}

/*****************************************************************************/
/**
    Process the %c and %C conversions.

    @param pspec    Pointer to format specification.
    @param ap       Reference to optional format arguments list.
    @param code     Conversion specifier code.
    @param cons     Pointer to consumer function.
    @param parg     Pointer to opaque pointer updated by cons.

    @return Number of emitted characters, or EXBADFORMAT if failure
**/
static int do_conv_c( T_FormatSpec * pspec,
                      va_list *      ap,
                      char           code,
                      void *      (* cons)(void *, const char *, size_t),
                      void * *       parg )
{
    char cc;
    int n = 0;
    unsigned int rep;

    if ( code == 'c' )
        cc = (char)va_arg( *ap, int );
    else
        cc = pspec->repchar;

    /* apply default precision */
    if ( pspec->prec < 0 )
        pspec->prec = 1;

    rep = (unsigned int)MAX( 1, pspec->prec );

    for ( ; rep > 0; rep-- )
    {
        int r = gen_out( cons, parg, 0, NULL, 0, 0, &cc, 1, 0 );
        if ( r == EXBADFORMAT )
            return EXBADFORMAT;
        n += r;
    }

    return n;
}

/*****************************************************************************/
/**
    Process a %s conversion.

    @param pspec    Pointer to format specification.
    @param ap       Reference to optional format arguments list.
    @param code     Conversion specifier code.
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
    Process the numeric conversions (%b, %d, %i, %I, %o, %u, %U, %x, %X).

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
#if defined(CONFIG_WITH_LONG_LONG_SUPPORT)
#define T long long
#else
#define T long
#endif
    unsigned T uv;
    char prefix[2];
    size_t pfxWidth = 0;
    size_t grp_insertions = 0;
    static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    /* Get the value.
     * Signed values need special handling for negative values and the
     *  extra options for sign output which don't apply to the unsigned
     *  values.
     */
    if ( pspec->flags & F_IS_SIGNED )
    {
        T v;

#if defined(CONFIG_WITH_LONG_LONG_SUPPORT)
        if ( pspec->qual == DOUBLE_QUAL( 'l' ) )
            v = (T)va_arg( *ap, T );
        else
#endif
        if ( pspec->qual == 'l' )
            v = (T)va_arg( *ap, long );
        else if ( pspec->qual == 'j' )
            v = (T)va_arg( *ap, intmax_t );
        else if ( pspec->qual == 'z' )
            v = (T)va_arg( *ap, size_t );
        else if ( pspec->qual == 't' )
            v = (T)va_arg( *ap, ptrdiff_t );
        else
            v = (T)va_arg( *ap, int );

        if ( pspec->qual == 'h' )
            v = (short)v;
        if ( pspec->qual == DOUBLE_QUAL( 'h' ) )
            v = (signed char)v;

        /* Get absolute value */
        uv = (unsigned T)(v < 0 ? -v : v);

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
            pfxWidth      = 1;
            pspec->flags |= FHASH;
        }
    }
    else
    {
#if defined(CONFIG_WITH_LONG_LONG_SUPPORT)
        if ( pspec->qual == DOUBLE_QUAL( 'l' ) )
            uv = (unsigned T)va_arg( *ap, unsigned T );
        else
#endif
        if ( pspec->qual == 'l' )
            uv = (unsigned T)va_arg( *ap, unsigned long );
        else if ( pspec->qual == 'j' )
            uv = (unsigned T)va_arg( *ap, uintmax_t );
        else if ( pspec->qual == 'z' )
            uv = (unsigned T)va_arg( *ap, size_t );
        else if ( pspec->qual == 't' )
            uv = (unsigned T)va_arg( *ap, ptrdiff_t );
        else
            uv = (unsigned T)va_arg( *ap, unsigned int );

        if ( pspec->qual == 'h' )
            uv = (unsigned short)uv;
        if ( pspec->qual == DOUBLE_QUAL( 'h' ) )
            uv = (unsigned char)uv;

        prefix[0] = '0';
    }

    if ( code == 'o' && uv )
        pfxWidth = 1;

    if ( code == 'x' || code == 'X' || code == 'b' )
    {
        /* if non-zero or bang flag, add prefix for hex and binary */
        if ( ( pspec->flags & FBANG ) || uv )
        {
            prefix[1] = code;
            pfxWidth  = 2;
        }

        /* Bang flag forces lower-case */
        if ( pspec->flags & FBANG )
            prefix[1] |= 0x20;
    }

    if ( pspec->flags & FHASH )
    {
        length += pfxWidth;
        pfx_s = prefix;
        pfx_n = pfxWidth;
    }

    /* work out how many digits in uv */
    for ( numWidth = 0; uv > 0; uv /= base )
    {
        char cc = digits[uv % base];

        /* convert to lower case? */
        if ( code == 'x' || code == 'i' || code == 'u' )
            cc |= 0x20;

        ++numWidth;
        numBuffer[sizeof(numBuffer) - numWidth] = cc;
    }

#if defined(CONFIG_WITH_GROUPING_SUPPORT)
    if ( pspec->grouping.len )
    {
        const void *  ptr   = pspec->grouping.ptr;
        size_t        glen  = pspec->grouping.len;
        char          grp   = 0;
        size_t        wid   = 0;
        unsigned int  decade;
        size_t        d_rem = numWidth;
        size_t        idx   = sizeof(numBuffer) - numWidth;
        size_t        s, n;

        MOVE_VOID_PTR( ptr, glen - 1 );

        while ( d_rem )
        {
            if ( glen )
            {
                grp = READ_CHAR( ptr );

                if ( grp == '-' )
                    break;

                if ( grp == '*' )
                {
                    int w = (int)va_arg( *ap, int );
                    if ( w < 0 )
                        break;

                    wid = (size_t)w;
                    DEC_VOID_PTR(ptr);
                    --glen;
                }
                else
                {
                    for ( wid = 0, decade = 1;
                          glen != 0
                             && ( grp = READ_CHAR( ptr ) ) != '\0'
                             && ISDIGIT( grp );
                          DEC_VOID_PTR(ptr), --glen )
                    {
                        wid += decade * ( grp - '0' );
                        decade *= 10;
                    }
                }

                if ( !glen )
                    break;

                grp = READ_CHAR( ptr );
                DEC_VOID_PTR(ptr);
                --glen;
            }

            if ( wid )
            {
                if ( d_rem <= wid )
                    break;

                for ( s = idx, n = d_rem - wid; n > 0; n--, s++ )
                    numBuffer[s-1] = numBuffer[s];

                idx--;
                numBuffer[idx + d_rem - wid] = grp;
                numWidth++;
                grp_insertions++;

                d_rem -= wid;
            }
            else if ( !glen )
                break;
        }
    }
#endif /* CONFIG_WITH_GROUPING_SUPPORT */

    digitWidth = numWidth;

    /* apply default precision */
    if ( pspec->prec < 0 )
        pspec->prec = 1;
    else
        pspec->flags &= ~FZERO; /* Ignore if precision specified */

    numWidth = MAX( numWidth, pspec->prec + grp_insertions );
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

    if ( code == 'n' )
        return do_conv_n( pspec, ap );

    if ( code == '%' )
        return gen_out( cons, parg, 0, NULL, 0, 0, &code, 1, 0 );

    if ( code == 'c' || code == 'C' )
        return do_conv_c( pspec, ap, code, cons, parg );

    if ( code == 's' )
    {
        return do_conv_s( pspec, ap, cons, parg );
    }

#if defined(CONFIG_WITH_FP_SUPPORT)
    if ( code == 'a' || code == 'A'
      || code == 'e' || code == 'E'
      || code == 'f' || code == 'F'
      || code == 'g' || code == 'G' )
        return do_conv_fp( pspec, ap, code, cons, parg );

    if ( code == 'k' )
        return do_conv_k( pspec, ap, cons, parg );
#endif

    /* -------------------------------------------------------------------- */

    /* The '%p' conversion is a meta-conversion, which we convert to a
     *  pre-defined format.  In this case we convert it to "%!#N.NX"
     *  where N is double the machine-word size, as each byte converts into
     *  two characters.
     */
    if ( code == 'p' )
    {
        code          = 'X';
        pspec->qual   = ( sizeof( void * ) > sizeof( int ) ) ? DOUBLE_QUAL('l') : 0;
        pspec->width  = (unsigned int)(sizeof( void * ) * 2);
        pspec->prec   = (int)(sizeof( void * ) * 2);
    }

    /* -------------------------------------------------------------------- */

    /* The '%d' and '%i' conversions are both decimal (base 10) and the '#'
     *  flag is ignored.  We set the F_IS_SIGNED internal flag to guide later
     *  processing.
     */
    if ( code == 'd' || code == 'i' || code == 'I' )
    {
        pspec->flags |= F_IS_SIGNED;
        base = 10;
        pspec->flags &= ~FHASH;

        if ( ( code == 'i' || code == 'I' ) && pspec->base )
           base = pspec->base;
    }

    if ( code == 'x' || code == 'X' )
        base = 16;

    if ( code == 'u' || code == 'U' )
       base = pspec->base ? pspec->base : 10;

    if ( code == 'o' )
        base = 8;

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
    const void   * ptr = (const void *)fmt;
    va_list        ap;
    int            parsing_conversion = 0;  /* Track if inside conversion spec */
    
    /* Setup varargs -- must va_end( ap ) before exit !! */
    va_copy( ap, apx );

    if ( fmt == NULL )
        goto exit_badformat;

    fspec.nChars = 0;

    while ( ( c = READ_CHAR( ptr ) ) )
    {
        /* scan for % or \0 */
        {
            size_t n = 0;
            const char *s = (const char *)ptr;

            /* For normal RAM-based strings we scan over as many input chars
             *  as we can to minimise calls to emit().
             */
            for ( ; *s && *s != '%'; s++ )
                n++;

            if ( n > 0 )
            {
                if ( emit( (const char *)ptr, n, cons, &arg ) < 0 )
                    goto exit_badformat;

                fspec.nChars += n;
            }
            ptr = (const void *)s;
        }

        if ( READ_CHAR( ptr ) )
        {
            /* found conversion specifier */
            char convspec;
            char *t;
            int nn;
            static const char fchar[] = {" +-#0!^"};
            static const unsigned int fbit[] = {
                FSPACE, FPLUS, FMINUS, FHASH, FZERO, FBANG, FCARET, 0};

            INC_VOID_PTR(ptr);    /* skip the % sign */

            /* If next char is not '\0', we're parsing a conversion specification.
             * If it IS '\0', it's bare '%' continuation (don't set flag). */
            if ( READ_CHAR( ptr ) != '\0' )
                parsing_conversion = 1;

            /* process conversion flags */
            for ( fspec.flags = 0;
                  (c = READ_CHAR( ptr )) && (t = STRCHR(fchar, c)) != NULL;
                  INC_VOID_PTR(ptr) )
            {
                fspec.flags |= fbit[t - fchar];
            }

            /* process width */
            if ( READ_CHAR( ptr ) == '*' )
            {
                int w = va_arg( ap, int );
                if ( w < 0 )
                {
                    w = -w;
                    fspec.flags |= FMINUS;
                }
                fspec.width = (unsigned int)w;
                INC_VOID_PTR(ptr);
            }
            else
            {
                for ( fspec.width = 0;
                      ( c = READ_CHAR( ptr ) ) && ISDIGIT( c ) && fspec.width < MAXWIDTH;
                      INC_VOID_PTR(ptr) )
                {
                    fspec.width = fspec.width * 10 + c - '0';
                }
            }

            if ( fspec.width > MAXWIDTH )
                goto exit_badformat;

            /* process precision */
            if ( READ_CHAR( ptr ) != '.' )
                fspec.prec = -1; /* precision is missing */
            else if ( READ_CHAR( INC_VOID_PTR(ptr) ) == '*' )
            {
                fspec.prec = va_arg( ap, int );

                if ( fspec.prec > MAXPREC )
                    goto exit_badformat;

                INC_VOID_PTR(ptr);
            }
            else
            {
                for ( fspec.prec = 0;
                      ( c = READ_CHAR( ptr ) ) && ISDIGIT( c ) && fspec.prec < MAXPREC;
                      INC_VOID_PTR(ptr) )
                {
                    fspec.prec = fspec.prec * 10 + c - '0';
                }
                if ( fspec.prec > MAXPREC )
                    goto exit_badformat;
            }

            /* process base */
            if ( READ_CHAR( ptr ) != ':' )
                fspec.base = 0;
            else if ( READ_CHAR( INC_VOID_PTR(ptr) ) == '*' )
            {
                int v = va_arg( ap, int );

                if ( v < 0 )
                    fspec.base = 0;
                else if ( v > MAXBASE )
                    goto exit_badformat;
                else
                    fspec.base = (unsigned int)v;

                INC_VOID_PTR(ptr);
            }
            else
            {
                for ( fspec.base = 0;
                      ( c = READ_CHAR( ptr ) ) && ISDIGIT( c ) && fspec.base < MAXBASE;
                      INC_VOID_PTR(ptr) )
                {
                    fspec.base = fspec.base * 10 + c - '0';
                }
                if ( fspec.base > MAXBASE )
                    goto exit_badformat; 
            }

#if defined(CONFIG_WITH_FP_SUPPORT)
            fspec.xp.w_int  = -1;
            fspec.xp.w_frac = -1;
#endif

#if defined(CONFIG_WITH_GROUPING_SUPPORT)
            /* test for grouping qualifier */
            fspec.grouping.len = 0;
            fspec.grouping.ptr = NULL;
#endif

            switch( READ_CHAR( ptr ) )
            {
#if defined(CONFIG_WITH_GROUPING_SUPPORT)
            case '[': /* grouping specifier */
            {
                size_t gplen = 0;

                /* skip over opening brace */
                INC_VOID_PTR(ptr);

                /* set the pointer mode */
                fspec.grouping.ptr  = ptr;

                /* scan to end of grouping string */
                while ( ( c = READ_CHAR( ptr ) ) && c != ']' )
                {
                    INC_VOID_PTR(ptr);
                    ++gplen;
                }
                if ( c == '\0' )
                    goto exit_badformat;

                /* skip over closing brace */
                INC_VOID_PTR(ptr);

                /* record the grouping spec length */
                fspec.grouping.len = gplen;
            }
	    break;
#endif
#if defined(CONFIG_WITH_FP_SUPPORT)
	    case '{': /* fixed-point specifier */
            {
                int p, q;

                /* skip over opening brace */
                INC_VOID_PTR( ptr );

                /* get integer width */
                if ( READ_CHAR( ptr ) == '*' )
                {
                    p = va_arg( ap, int );
                    p = MAX( MIN_XP_INT, p );  /* clamp negative values to minimum */

                    INC_VOID_PTR( ptr );
                }
                else
                {
                    for ( p = 0;
                         ( c = READ_CHAR( ptr ) ) && ISDIGIT( c );
                         INC_VOID_PTR( ptr ) )
                    {
                        p = p * 10 + c - '0';
                        if ( p > MAX_XP_INT )
                            goto exit_badformat;
                    }

                    /* explicit values must meet minimum */
                    if ( p < MIN_XP_INT )
                        goto exit_badformat;
                }

                /* get fractional width */
                if ( READ_CHAR( ptr ) != '.' )
                    goto exit_badformat; /* fractional width is missing */
                else if ( READ_CHAR( INC_VOID_PTR(ptr) ) == '*' )
                {
                    q = va_arg( ap, int );
                    q = MAX( MIN_XP_FRAC, q );  /* clamp negative values to minimum */

                    INC_VOID_PTR( ptr );
                }
                else
                {
                    for ( q = 0;
                         ( c = READ_CHAR( ptr ) ) && ISDIGIT( c );
                         INC_VOID_PTR( ptr ) )
                    {
                        q = q * 10 + c - '0';
                        if ( q > MAX_XP_FRAC )
                            goto exit_badformat;
                    }

                    /* explicit values must meet minimum */
                    if ( q < MIN_XP_FRAC )
                        goto exit_badformat;
                }

                if ( p + q >= MAX_XP_WIDTH )
                    goto exit_badformat;

                if ( c == '\0' )
                    goto exit_badformat;

                /* skip over closing brace */
                INC_VOID_PTR( ptr );

                fspec.xp.w_int  = p;
                fspec.xp.w_frac = q;
            }
	    break;
#endif
            } /* switch(..) */

            /* test for length qualifier */
            c = READ_CHAR( ptr );
            fspec.qual = ( c && STRCHR( "hljztL", c ) ) ? (INC_VOID_PTR(ptr), c) : '\0';

            /* catch double qualifiers */
            if ( fspec.qual && (c = READ_CHAR( ptr )) && c == fspec.qual )
            {
                fspec.qual = DOUBLE_QUAL( fspec.qual );
                INC_VOID_PTR(ptr);
            }

            /* Continuation */
            c = READ_CHAR( ptr );
            if ( c == '\0' )
            {
                /* If we're in middle of parsing a conversion, it's incomplete */
                if ( parsing_conversion )
                    goto exit_badformat;

                /* Otherwise, bare '%' continuation is valid */
                ptr = va_arg( ap, const char * );
                continue;
            }

            convspec = c;

            if ( convspec == 'C' )
            {
                c = READ_CHAR( INC_VOID_PTR(ptr) );
                if ( c == '\0' )
                    goto exit_badformat;
                fspec.repchar = c;
            }
            else
            {
                fspec.repchar = '\0';
            }

            /* now process the conversion type */
            nn = do_conv( &fspec, &ap, convspec, cons, &arg );
            if ( nn < 0 )
                goto exit_badformat;
            else
            {
                fspec.nChars += (unsigned int)nn;
                parsing_conversion = 0;  /* Conversion completed successfully */
            }

            INC_VOID_PTR(ptr);
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
