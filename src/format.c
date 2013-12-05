/* ****************************************************************************
 * Format - lightweight string formatting library.
 * Copyright (C) 2010-2013, Neil Johnson
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
#define FSPACE          ( 0x01 )
#define FPLUS           ( 0x02 )
#define FMINUS          ( 0x04 )
#define FHASH           ( 0x08 )
#define FZERO           ( 0x10 )
#define FBANG           ( 0x20 )
#define FCARET          ( 0x40 )
#define F_IS_SIGNED     ( 0x80 )

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
#define MAXWIDTH        ( 500 )
#define MAXPREC         ( 500 )
#define MAXBASE         ( 36 )
#define BUFLEN          ( 130 )  /* Must be long enough for 64-bit pointers
                                     in binary with maximum grouping chars and
                                     prefix. */

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

/** Set up platform-dependent access to variable arguments **/
#if defined(CONFIG_VA_LIST_AS_ARRAY_TYPE)
  #define VARGS(x)        (x)
  #define VALPARM(y)      va_list y
  #define VALST(z)        (z)
#else
  #define VARGS(x)        (&x)
  #define VALPARM(y)      const va_list *y
  #define VALST(z)        (*z)
#endif

/*****************************************************************************/
/**
    Some devices have separate memory spaces for normal data and read-only
    (or "ROM") data.  We classify these as NORMAL and ALT memory pointers.
**/
enum ptr_mode            { NORMAL_PTR, ALT_PTR };

/** A generic macro to read a character from memory **/
#if defined(CONFIG_HAVE_ALT_PTR)
  #define READ_CHAR(m,p)   (((m)==NORMAL_PTR) ? *(const char *)(p) : ROM_CHAR(p))
  #else
  #define READ_CHAR(m,p)   (*(const char *)(p))
#endif

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

#define STRLEN_ALT(s)       (xx_strlen_alt(s))

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
    int             width;  /**< width                              **/
    int             prec;   /**< precision                          **/
    int             base;   /**< numeric base                       **/
    char            qual;   /**< length qualifier                   **/
    char            repchar;/**< Repetition character               **/
    struct {
#if defined(CONFIG_HAVE_ALT_PTR)
        enum ptr_mode mode; /**< grouping spec pointer type         **/
#endif
        const void *  ptr;  /**< ptr to grouping specification      **/
        size_t        len;  /**< length of grouping spec            **/
    } grouping;
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

static int do_conv( T_FormatSpec *, VALPARM(), char,
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
static int do_conv_n( T_FormatSpec *, VALPARM() );

static int do_conv_c( T_FormatSpec *, VALPARM(), char,
                      void * (*)(void *, const char *, size_t), void * * );

static int do_conv_s( T_FormatSpec *, VALPARM(), char,
                      void * (*)(void *, const char *, size_t), void * * );

static int do_conv_numeric( T_FormatSpec *, VALPARM(), char,
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

/** Additional support function that computes the length of a string held in
     ROM memory.  Because this is a non-standard function we always use our
     own version, irrespective of what machine-specific support libraries may
     offer.
**/
#if defined(CONFIG_HAVE_ALT_PTR)
static size_t xx_strlen_alt( ROM_PTR_T s )
{
    ROM_PTR_T p;
    for ( p = s; ROM_CHAR(p) != '\0'; p++ )
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
        size_t j = MIN( PAD_STRING_LEN, n );
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
                                size_t length,
                                size_t *ps1,
                                size_t *ps2 )
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
                      VALPARM(ap) )
{
    void *vp = va_arg(VALST(ap), void *);

    if ( vp )
    {
        if ( pspec->qual == 'h' )
            *(short *)vp = (short)pspec->nChars;
        else if ( pspec->qual == DOUBLE_QUAL( 'h' ) )
            *(signed char *)vp = (signed char)pspec->nChars;
        else if ( pspec->qual == 'l' )
            *(long *)vp = (long)pspec->nChars;
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
                      VALPARM(ap),
                      char           code,
                      void *      (* cons)(void *, const char *, size_t),
                      void * *       parg )
{
    char cc;
    int n = 0;
    unsigned int rep;

    if ( code == 'c' )
        cc = (char)va_arg(VALST(ap), int);
    else
        cc = pspec->repchar;

    /* apply default precision */
    if ( pspec->prec < 0 )
        pspec->prec = 1;

    rep = MAX( 1, pspec->prec );

    for ( ; rep; rep-- )
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
                      VALPARM(ap),
                      char           code,
                      void *      (* cons)(void *, const char *, size_t),
                      void * *       parg )
{
    size_t length = 0;
    size_t ps1 = 0, ps2 = 0;

    const char *s = va_arg(VALST(ap), const char *);

    if ( s == NULL )
        s = "(null)";

    length = STRLEN( s );
    if ( pspec->prec >= 0 )
        length = MIN( pspec->prec, length );

    calc_space_padding( pspec, length, &ps1, &ps2 );

    return gen_out( cons, parg, ps1, NULL, 0, 0, s, length, ps2 );
}

/*****************************************************************************/
/**
    Process a %s conversion that uses the alternate pointer type.

    We split out the alternate functionality into a separate function so that
    platforms that do not have this behaviour are not unduly loaded with
    additional code, and it helps keep the main code cleaner.

    @param pspec    Pointer to format specification.
    @param ap       Reference to optional format arguments list.
    @param code     Conversion specifier code.
    @param cons     Pointer to consumer function.
    @param parg     Pointer to opaque pointer updated by cons.

    @return Number of emitted characters, or EXBADFORMAT if failure
**/

#if defined(CONFIG_HAVE_ALT_PTR)
static int do_conv_s_alt( T_FormatSpec * pspec,
                          VALPARM(ap),
                          char           code,
                          void *      (* cons)(void *, const char *, size_t),
                          void * *       parg )
{
    size_t length = 0;
    size_t ps1 = 0, ps2 = 0;
    size_t n = 0;
    static ROM_DECL(char null_string[]) = "(null)";

    const void *vp = (const void*)va_arg(VALST(ap), ROM_PTR_T);

    if ( vp == NULL )
        vp = null_string;

    length = STRLEN_ALT( (ROM_PTR_T)vp);
    if ( pspec->prec >= 0 )
        length = MIN( pspec->prec, length );

    calc_space_padding( pspec, length, &ps1, &ps2 );

    /* Inline the relevant parts of gen_out() here as we need to handle
     *  alternate memory pointers.
     */
    if ( ps1 && pad( spaces, ps1, cons, parg ) < 0 )
        return EXBADFORMAT;
    n += ps1;

    while ( length-- )
    {
        char c = ROM_CHAR(vp++);
        if ( emit( &c, 1, cons, parg ) < 0 )
            return EXBADFORMAT;
        n++;
    }

    if ( ps2 && pad( spaces, ps2, cons, parg ) < 0 )
        return EXBADFORMAT;
    n += ps2;

    return (int)n;
}
#endif

/*****************************************************************************/
/**
    Process the numeric conversions (%b, %d, %i, %o, %u, %x, %X).

    @param pspec    Pointer to format specification.
    @param ap       Reference to optional format arguments list.
    @param code     Conversion specifier code.
    @param cons     Pointer to consumer function.
    @param parg     Pointer to opaque pointer updated by cons.

    @return Number of emitted characters, or EXBADFORMAT if failure
**/
static int do_conv_numeric( T_FormatSpec * pspec,
                            VALPARM(ap),
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
    unsigned long uv;
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
        long v;

        if ( pspec->qual == 'l' )
            v = (long)va_arg( VALST(ap), long );
        else if ( pspec->qual == 'j' )
            v = (long)va_arg( VALST(ap), intmax_t );
        else if ( pspec->qual == 'z' )
            v = (long)va_arg( VALST(ap), size_t );
        else if ( pspec->qual == 't' )
            v = (long)va_arg( VALST(ap), ptrdiff_t );
        else
            v = (long)va_arg( VALST(ap), int );

        if ( pspec->qual == 'h' )
            v = (short)v;
        if ( pspec->qual == DOUBLE_QUAL( 'h' ) )
            v = (signed char)v;

        /* Get absolute value */
        uv = v < 0 ? -v : v;

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
        if ( pspec->qual == 'l' )
            uv = (unsigned long)va_arg( VALST(ap), unsigned long );
        else if ( pspec->qual == 'j' )
            uv = (unsigned long)va_arg( VALST(ap), uintmax_t );
        else if ( pspec->qual == 'z' )
            uv = (unsigned long)va_arg( VALST(ap), size_t );
        else if ( pspec->qual == 't' )
            uv = (unsigned long)va_arg( VALST(ap), ptrdiff_t );
        else
            uv = (unsigned long)va_arg( VALST(ap), unsigned int );

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
    /* Note: splitting it out like this affords us the opportunity to
     *  avoid calling out to libgcc.
     *  In the case of decimal, on large word machines we can produce tight
     *  code for dividing by a known constant by applying "Division by
     *  Invariant Integers Using Multiplication" algorithm from the 1994 paper
     *  by Torbjorn Granlund and Peter L. Montgomery.
     * For the magic number see:
     *            http://www.hackersdelight.org/magic.htm
     * We compute the remainder in the obvious way.
     *
     * For the other cases we can implement the necessary math through
     *  bit ops - masking and shifting.
     */
    if ( base == 10 )
    {
        for( numWidth = 0; uv; )
        {
#if defined(CONFIG_USE_INLINE_DIV10)
            unsigned long long div_a = uv * 0xCCCCCCCDULL;
            unsigned long div_uv = (div_a >> 32) >> 3;
            unsigned long div_5uv = div_uv + (div_uv << 2);
            unsigned long div_rem = uv - (div_5uv << 1);
#else
            unsigned long div_uv  = uv / 10;
            unsigned long div_rem = uv % 10;
#endif

            ++numWidth;
            numBuffer[sizeof(numBuffer) - numWidth] = '0' + div_rem;
            uv = div_uv;
        }
    }
     /* Special-case for bases 2, 8 or 16 for the dedicated conversion
      * specifiers %b, %o and %x/%X.
      */
    else if ( base == 2 || base == 8 || base == 16 )
    {
        unsigned int mask  = base - 1;
        unsigned int shift = base == 16 ? 4
                                        : base == 8 ? 3 : 1;

        for( numWidth = 0; uv; uv >>= shift )
        {
            char cc = digits[uv & mask];

            /* convert to lower case? */
            if ( code == 'x' || code == 'i' || code == 'u' )
                cc |= 0x20;

            ++numWidth;
            numBuffer[sizeof(numBuffer) - numWidth] = cc;
        }
    }
    else /* all other bases */
    {
       for ( numWidth = 0; uv; uv /= base )
       {
          char cc = digits[uv % base];

          /* convert to lower case? */
          if ( code == 'i' || code == 'u' )
             cc |= 0x20;

          ++numWidth;
          numBuffer[sizeof(numBuffer) - numWidth] = cc;
       }
    }

    if ( pspec->grouping.len )
    {
#if defined(CONFIG_HAVE_ALT_PTR)
        enum ptr_mode mode  = pspec->grouping.mode;
#endif
        const void *  ptr   = pspec->grouping.ptr;
        size_t        glen  = pspec->grouping.len;
        char          grp;
        int           wid   = 0;
        unsigned int  decade;
        size_t        d_rem = numWidth;
        size_t        idx   = sizeof(numBuffer) - numWidth;
        size_t        s, n;

        MOVE_VOID_PTR( ptr, glen - 1 );

        while ( d_rem )
        {
            if ( glen )
            {
                grp = READ_CHAR( mode, ptr );

                if ( grp == '-' )
                    break;

                if ( grp == '*' )
                {
                    wid = (int)va_arg( VALST(ap), int );
                    if ( wid < 0 )
                        break;

                    DEC_VOID_PTR(ptr);
                    --glen;
                }
                else
                {
                    for ( wid = 0, decade = 1;
                          glen != 0
                             && ( grp = READ_CHAR( mode, ptr ) ) != '\0'
                             && ISDIGIT( grp );
                          DEC_VOID_PTR(ptr), --glen )
                    {
                        wid += decade * ( grp - '0' );
                        decade *= 10;
                    }
                }

                if ( !glen )
                    break;

                grp = READ_CHAR( mode, ptr );
                DEC_VOID_PTR(ptr);
                --glen;
            }

            if ( wid )
            {
                if ( d_rem <= wid )
                    break;

                for ( s = idx, n = d_rem - wid; n; n--, s++ )
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
                    VALPARM(ap),
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
#if defined(CONFIG_HAVE_ALT_PTR)
        if ( pspec->flags & FHASH )
            return do_conv_s_alt( pspec, ap, code, cons, parg );
        else
#endif
            return do_conv_s( pspec, ap, code, cons, parg );
    }

#if defined(CONFIG_WITH_FP_SUPPORT)
    if ( code == 'e' || code == 'E'
      || code == 'f' || code == 'F'
      || code == 'g' || code == 'G' )
        return do_conv_fp( pspec, ap, code, cons, parg );
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
        pspec->qual   = ( sizeof(int *) > sizeof( int ) ) ? 'l' : 0;
        pspec->flags  = FHASH | FBANG;
        pspec->width  = sizeof( int * ) * 2;
        pspec->prec   = sizeof( int * ) * 2;
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
    @param ap       List of optional format string arguments.

    @return Number of characters sent to @a cons, or EXBADFORMAT.
**/
int format( void *    (* cons) (void *, const char * , size_t),
            void *       arg,
            const char * fmt,
            va_list      ap )
{
    T_FormatSpec fspec;
#if defined(CONFIG_HAVE_ALT_PTR)
    enum ptr_mode  mode = NORMAL_PTR;
#endif
    char           c;
    const void   * ptr = (const void *)fmt;

    if ( fmt == NULL )
        return EXBADFORMAT;

    fspec.nChars = 0;

    while ( ( c = READ_CHAR( mode, ptr ) ) )
    {
        /* scan for % or \0 */
#if defined(CONFIG_HAVE_ALT_PTR)
        if ( mode == NORMAL_PTR )
#endif
        {
            unsigned int n;
            const char *s = (const char *)ptr;

            /* For normal RAM-based strings we scan over as many input chars
             *  as we can to minimise calls to emit().
             */
             for ( ; *s && *s != '%'; s++ )
             ;

             n = s - (const char *)ptr;
            if ( n > 0 )
            {
                if ( emit( (const char *)ptr, n, cons, &arg ) < 0 )
                    return EXBADFORMAT;
               fspec.nChars += n;
            }
            ptr = (const void *)s;
        }
#if defined(CONFIG_HAVE_ALT_PTR)
        else
        {
            /* Alternate pointers are treated one character at a time to keep
             *  the interface to emit() common across all string pointer
             *  types.
             */
            while ( ( c = ROM_CHAR(ptr) ) && c != '%' )
            {
                if ( emit( &c, 1, cons, &arg ) < 0 )
                    return EXBADFORMAT;
                fspec.nChars++;
                ptr++;
            }
        }
#endif

        if ( READ_CHAR( mode, ptr ) )
        {
            /* found conversion specifier */
            char convspec;
            char *t;
            int nn;
            static const char fchar[] = {" +-#0!^"};
            static const unsigned int fbit[] = {
                FSPACE, FPLUS, FMINUS, FHASH, FZERO, FBANG, FCARET, 0};

            INC_VOID_PTR(ptr);    /* skip the % sign */

            /* process conversion flags */
            for ( fspec.flags = 0;
                  (c = READ_CHAR( mode, ptr )) && (t = STRCHR(fchar, c)) != NULL;
                  INC_VOID_PTR(ptr) )
            {
                fspec.flags |= fbit[t - fchar];
            }

            /* process width */
            if ( READ_CHAR( mode, ptr ) == '*' )
            {
                fspec.width = va_arg( ap, int );
                if ( fspec.width < 0 )
                {
                    fspec.width = -fspec.width;
                    fspec.flags |= FMINUS;
                }
                INC_VOID_PTR(ptr);
            }
            else
            {
                for ( fspec.width = 0;
                      ( c = READ_CHAR( mode, ptr ) ) && ISDIGIT( c ) && fspec.width < MAXWIDTH;
                      INC_VOID_PTR(ptr) )
                {
                    fspec.width = fspec.width * 10 + c - '0';
                }
            }

            if ( fspec.width > MAXWIDTH )
                return EXBADFORMAT;

            /* process precision */
            if ( READ_CHAR( mode, ptr ) != '.' )
                fspec.prec = -1; /* precision is missing */
            else if ( READ_CHAR( mode, INC_VOID_PTR(ptr) ) == '*' )
            {
                fspec.prec = va_arg( ap, int );

                if ( fspec.prec > MAXPREC )
                    return EXBADFORMAT;

                INC_VOID_PTR(ptr);
            }
            else
            {
                for ( fspec.prec = 0;
                      ( c = READ_CHAR( mode, ptr ) ) && ISDIGIT( c ) && fspec.prec < MAXPREC;
                      INC_VOID_PTR(ptr) )
                {
                    fspec.prec = fspec.prec * 10 + c - '0';
                }
                if ( fspec.prec > MAXPREC )
                    return EXBADFORMAT;
            }

            /* process base */
            if ( READ_CHAR( mode, ptr ) != ':' )
                fspec.base = 0;
            else if ( READ_CHAR( mode, INC_VOID_PTR(ptr) ) == '*' )
            {
                int v = va_arg( ap, int );

                if ( v < 0 )
                    fspec.base = 0;
                else if ( v > MAXBASE )
                    return EXBADFORMAT;
                else
                    fspec.base = (unsigned int)v;

                INC_VOID_PTR(ptr);
            }
            else
            {
                for ( fspec.base = 0;
                      ( c = READ_CHAR( mode, ptr ) ) && ISDIGIT( c ) && fspec.base < MAXBASE;
                      INC_VOID_PTR(ptr) )
                {
                    fspec.base = fspec.base * 10 + c - '0';
                }
                if ( fspec.base > MAXBASE )
                    return EXBADFORMAT;
            }

            /* test for grouping qualifier */
            fspec.grouping.len = 0;
            fspec.grouping.ptr = NULL;
#if defined(CONFIG_HAVE_ALT_PTR)
            fspedc.grouping.mode = NORMAL_PTR;
#endif
            c = READ_CHAR( mode, ptr );
            if ( c == '[' )
            {
                size_t gplen = 0;

                /* skip over opening brace */
                INC_VOID_PTR(ptr);

                /* set the pointer mode */
#if defined(CONFIG_HAVE_ALT_PTR)
                fspec.grouping.mode = mode;
#endif
                fspec.grouping.ptr  = ptr;

                /* scan to end of grouping string */
                while ( ( c = READ_CHAR( mode, ptr ) ) && c != ']' )
                {
                    INC_VOID_PTR(ptr);
                    ++gplen;
                }
                if ( c == '\0' )
                    return EXBADFORMAT;

                /* skip over closing brace */
                INC_VOID_PTR(ptr);

                /* record the grouping spec length */
                fspec.grouping.len = gplen;
            }

            /* test for length qualifier */
            c = READ_CHAR( mode, ptr );
            fspec.qual = ( c && STRCHR( "hljztL", c ) ) ? (INC_VOID_PTR(ptr), c) : '\0';

            /* catch double qualifiers */
            if ( fspec.qual && (c = READ_CHAR( mode, ptr )) && c == fspec.qual )
            {
                fspec.qual = DOUBLE_QUAL( fspec.qual );
                INC_VOID_PTR(ptr);
            }

            /* Continuation */
            c = READ_CHAR( mode, ptr );
            if ( c == '\0' )
            {
#if defined(CONFIG_HAVE_ALT_PTR)
                if ( fspec.flags & FHASH )
                {
                    mode = ALT_PTR;
                    ptr = va_arg( ap, ROM_PTR_T );
                }
                else
                {
                    mode = NORMAL_PTR;
#endif
                    ptr = va_arg( ap, const char * );
#if defined(CONFIG_HAVE_ALT_PTR)
                }
#endif
                continue;
            }

            convspec = c;

            if ( convspec == 'C' )
            {
                c = READ_CHAR( mode, INC_VOID_PTR(ptr) );
                if ( c == '\0' )
                    return EXBADFORMAT;
                fspec.repchar = c;
            }
            else
            {
                fspec.repchar = '\0';
            }

            /* now process the conversion type */
            nn = do_conv( &fspec, VARGS(ap), convspec, cons, &arg );
            if ( nn < 0 )
                return EXBADFORMAT;
            else
                fspec.nChars += (unsigned int)nn;

            INC_VOID_PTR(ptr);
        }
    }

    return fspec.nChars;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
