/* ****************************************************************************
 * Format - lightweight string formatting library.
 * Copyright (C) 2010, Neil Johnson
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

/** Query the environment about what capabilities are available **/
#if __STDC_HOSTED__
    #define CONFIG_HAVE_LIBC
#endif

#if defined(CONFIG_HAVE_LIBC)
    #include <string.h>
    #include <ctype.h>
#else
    /** Define some things we need **/
    #define NULL        ((void*)0)
#endif

/*****************************************************************************/
/* Project Includes                                                          */
/*****************************************************************************/

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

/**
    Set limits.
**/
#define MAXWIDTH        ( 999 )
#define MAXPREC         ( 999 )
#define BUFLEN          ( 40 )  /* must be long enough for 32-bit pointers */

/**
    Return the maximum/minimum of two scalar values.
**/
#define MAX(a,b)        ( (a) > (b) ? (a) : (b) )
#define MIN(a,b)        ( (a) < (b) ? (a) : (b) )

/* The following is inspired from code from the kannel project.
   See http://www.kannel.org
 */
/**
    Some platforms va_list is an array type, on others it is a pointer (such as
    pointer to char).  These macros hide this important difference.
**/
#if (defined(__linux__) && (defined(__powerpc__)    \
                           || defined(__s390__)     \
                           || defined(__x86_64)))   \
    || (defined(__FreeBSD__) && defined(__amd64__)) \
    || (defined(DARWIN) && defined(__x86_64__))
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
    Emit @p n characters from string @p s.
    
    @param s        Pointer to source string
    @param n        Number of characters to emit
     
    This macro is specific to do_conv().
**/
#define EMIT(s, n)  do { if ((n)>0 && (*parg = (*cons)(*parg, (s), (n))) == NULL) \
                            return EXBADFORMAT;                               \
                    } while(0);

/*****************************************************************************/
/**
    Emit @p n padding characters from padding string @p s.
    
    @param s        Name of padding string.
    @param n        Number of padding characters to emit.
    
    This macro is specific to do_conv().
**/
#define PAD(s, n)   do { if ((n) > 0) {                         \
                        size_t i, j = (size_t)(n);              \
                        for ( ; 0 < j; j -= i) {                \
                            i = MIN( (sizeof(s) - 1), j );      \
                            EMIT((s),i);                        \
                        } }                                     \
                    } while(0);

/*****************************************************************************/
/**
    Wrapper macro around isdigit().
**/
#if defined(CONFIG_HAVE_LIBC)
    #define ISDIGIT(c)      (isdigit(c))
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
    int             nChars; /**< number of chars emitted so far     **/
    unsigned int    flags;  /**< flags                              **/
    int             width;  /**< width                              **/
    int             prec;   /**< precision                          **/
    char            qual;   /**< qualifiers                         **/
} T_FormatSpec;

/*****************************************************************************/
/* Private Data.  Declare as static.                                         */
/*****************************************************************************/

/**
    Padding strings, used by PAD() macro.
**/
static const char spaces[] = "                ";
static const char zeroes[] = "0000000000000000";

/*****************************************************************************/
/* Private function prototypes.  Declare as static.                          */
/*****************************************************************************/

static int do_conv( T_FormatSpec *, VALPARM(), char, 
                    void *(*)(void *, const char *, size_t), void * * );

/* Only declare these prototypes in a freestanding environment */
#if !defined( CONFIG_HAVE_LIBC )                    
static size_t xx_strlen( const char * );
static char * xx_strchr( const char *, int );
static void * memcpy( void *, const void *, size_t );
#endif

/*****************************************************************************/
/* Private functions.  Declare as static.                                    */
/*****************************************************************************/

/*****************************************************************************/
/**
    Local implementation of strlen().
    
    @param s        Pointer to string.
    
    @returns Length of string s excluding the terminating null character.
**/
#if !defined( CONFIG_HAVE_LIBC )
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
    
    @returns Address of first matching character, or NULL if not found.
**/
#if !defined( CONFIG_HAVE_LIBC )
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
    Freestanding implementation of memcpy() needed by GCC.
    
    @param dst        Pointer to destination.
    @param src        Pointer to source.
    @param len        Number of bytes to copy.
    
    @returns Original value of dst.
**/
#if !defined( CONFIG_HAVE_LIBC ) && defined( __GNUC__ )
static void * memcpy( void *dst, const void *src, size_t len )
{
    char *d = dst;
    const char *s = src;
    
    while(len--)
        *d++ = *s++;
        
    return dst;
}
#endif

/*****************************************************************************/
/**
    Handle a single format conversion for a given type.
    
    @param pspec    Pointer to format specification.
    @param ap       Reference to optional format arguments list.
    @param code     Conversion specifier code.
    @param cons     Pointer to consumer function.
    @param parg     Pointer to opaque pointer updated by cons.
    
    @returns Number of emitted characters, or EXBADFORMAT if failure
**/
static int do_conv( T_FormatSpec * pspec,
                    VALPARM(ap), 
                    char           code,
                    void *      (* cons)(void *, const char *, size_t),
                    void * *       parg )
{
    int length = 0;
    int width, numWidth, digitWidth, pfxWidth = 0;
    unsigned int base = 0;
    char numBuffer[BUFLEN];
    
    if ( code == 'n' )
    {
        if ( pspec->qual == 'h' )
        {
            short *sp = va_arg(VALST(ap), short *);
            if ( sp )
                *sp = (short) pspec->nChars;
        }
        else if ( pspec->qual == 'l' )
        {
            long *lp = va_arg(VALST(ap), long *);
            if ( lp )
                *lp = (long) pspec->nChars;
        }
        else
        {
            int *ip = va_arg(VALST(ap), int *);
            if ( ip )
                *ip = (int) pspec->nChars;
        }
        
        return 0;
    }
    
    if ( code == 'c' || code == '%' )
    {
        char cc = (char)code;
            
        if ( code == 'c' )
            cc = (char)va_arg(VALST(ap), int);
        
        EMIT( &cc, 1 );
        
        return 1;
    }
    
    if ( code == 's' )
    {
        char *s = va_arg(VALST(ap), char *);
        
        if ( s == NULL )
            s = "(null)";
                
        length = (int)STRLEN( s );
            
        if ( 0 <= pspec->prec && pspec->prec < length )
            length = pspec->prec;
        
        width = MAX( pspec->width - length, 0 );
        
        if ( !(pspec->flags & FMINUS) )
            PAD( spaces, width );
            
        EMIT( s, length );
        
        if ( pspec->flags & FMINUS )
            PAD( spaces, width );
            
        return ( length + width );
    }
    
    if ( code == 'd' || code == 'i' )
    {
        long v;
        unsigned long uv;
            
        /* get the value */
        v = pspec->qual == 'l' ? va_arg(VALST(ap), long)
                               : va_arg(VALST(ap), int);
        if ( pspec->qual == 'h' )
            v = (short)v;
        
        if ( v < 0 )
        {
            uv = -v;
            ++length;   /* allow for '-' sign */
        }
        else
        {
            uv = v;
            if ( pspec->flags & (FPLUS | FSPACE) )
                ++length;
        }
        
        /* work out how many digits in v */
        for( numWidth = 0; uv; uv /= 10 )
        {
            ++numWidth;
            numBuffer[sizeof(numBuffer) - numWidth] = '0' + (uv % 10);
        }
        
        digitWidth = numWidth;
        
        /* apply default precision */
        if ( pspec->prec < 0 )
            pspec->prec = 1;
        else
            pspec->flags &= ~FZERO;
        
        numWidth = MAX( numWidth, pspec->prec );
        
        length += numWidth;
        width   = MAX( pspec->width - length, 0 );
        
        /* got necessary info, now emit the number */
        
        if ( pspec->flags & FMINUS )
            ;
        else if ( pspec->flags & FZERO )
            numWidth = pspec->width;
        else
            PAD( spaces, width );
        
        if ( v < 0 )                        { EMIT( "-", 1 ); }
        else if ( pspec->flags & FPLUS )    { EMIT( "+", 1 ); }
        else if ( pspec->flags & FSPACE )   { EMIT( " ", 1 ); }
        
        PAD( zeroes, (numWidth - digitWidth) );
        EMIT( &numBuffer[sizeof(numBuffer) - digitWidth], digitWidth );
        
        if ( pspec->flags & FMINUS )
            PAD( spaces, width );
        
        return ( length + width );
    }
    
    if ( code == 'p' )
    {
        code          = 'X';
        pspec->qual   = ( sizeof(int *) > sizeof( int ) ) ? 'l' : 0;
        pspec->flags  = FHASH | FBANG;
        pspec->width  = sizeof( int * ) * 2;
        pspec->prec   = sizeof( int * ) * 2;
    }
    
    if ( code == 'x' || code == 'X' )
        base = 16;
    else if ( code == 'u' )
        base = 10;
    else if ( code == 'o' )
        base = 8;
    else if ( code == 'b' )
        base = 2;
        
    if ( base > 0 )
    {
        unsigned long uv;
        char prefix[2] = { '0', '0' };
        
        /* get the value */
        uv = pspec->qual == 'l' ? va_arg(VALST(ap), unsigned long) 
                                : va_arg(VALST(ap), unsigned int );
        if ( pspec->qual == 'h' )
            uv = (unsigned short)uv;
            
        if ( code == 'o' && uv )
            pfxWidth = 1;
        
        if ( code == 'x' || code == 'X' || code == 'b' ) 
        {
            /* if non-zero or bang flag, add prefix for hex and binary */
            if ( ( pspec->flags & FBANG ) || uv )   
                pfxWidth = 2;
                
            prefix[1] = code;
            
            /* Bang flag forces lower-case */
            if ( pspec->flags & FBANG )
                prefix[1] |= 0x20;
        }
        
        /* work out how many digits in uv */
        /* Note: splitting it out like this avoids calling out to libgcc.
         *  In the case of decimal, gcc can produce tight code for dividing
         *  by a known constant (it applies "Division by Invariant Integers 
         *  Using Multiplication" algorithm from the 1994 paper by Torbjorn 
         *  Granlund and Peter L. Montgomery).
         *  For the other cases we can implement the necessary math through
         *  bit ops - masking and shifting.
         */
        if ( base == 10 )
        {
            for( numWidth = 0; uv; uv /= 10 )
            {
                ++numWidth;
                numBuffer[sizeof(numBuffer) - numWidth] = '0' + (uv % 10);
            }
        }
        else /* base is 2, 8 or 16 */
        {
            unsigned int mask  = base - 1;
            unsigned int shift = base == 16 ? 4
                                            : base == 8 ? 3 : 1;
            static const char digits[] = "0123456789ABCDEF";
            
            for( numWidth = 0; uv; uv >>= shift )
            {
                char cc = digits[uv & mask];
                
                /* convert to lower case? */
                if ( code == 'x' ) 
                    cc |= 0x20;
                
                ++numWidth;
                numBuffer[sizeof(numBuffer) - numWidth] = cc;
            }
        }
               
        digitWidth = numWidth;
        
        /* apply default precision */
        if ( pspec->prec < 0 )
            pspec->prec = 1;
        else
            pspec->flags &= ~FZERO;
        
        numWidth = MAX( numWidth, pspec->prec );
        
        length += numWidth;
        if ( pspec->flags & FHASH )
            length += pfxWidth;
        width = MAX( pspec->width - length, 0 );
        
        /* got necessary info, now emit the number */
        
        if ( !( pspec->flags & FMINUS ) && !( pspec->flags & FZERO ) )
            PAD( spaces, width );
            
        if ( pspec->flags & FHASH )
            EMIT( prefix, pfxWidth );
            
        if ( !( pspec->flags & FMINUS ) &&  ( pspec->flags & FZERO ) )
                PAD( zeroes, width );
        
        PAD( zeroes, (numWidth - digitWidth) );
        EMIT( &numBuffer[sizeof(numBuffer) - digitWidth], digitWidth );
        
        if ( pspec->flags & FMINUS )
            PAD( spaces, width );

        return ( length + width );
    }

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
    
    @returns Number of characters sent to @a cons, or EXBADFORMAT.
**/
int format( void *    (* cons) (void *, const char * , size_t),
            void *       arg,
            const char * fmt,
            va_list      ap )
{
    T_FormatSpec fspec;
    int n;
    
    if ( fmt == NULL )
        return EXBADFORMAT;

    fspec.nChars = 0;
    while ( *fmt )
    {
        const char *s = fmt;

        n = 0;

        /* scan for % or \0 */
        while ( *s && *s != '%' )
        {
            s++;
            n++;
        }

        if ( n > 0 )
        {
            if ( ( arg = (*cons)( arg, fmt, n ) ) != NULL )
                fspec.nChars += n;
            else
                return EXBADFORMAT;
        }

        fmt = s;

        if ( *s )
        {
            /* found conversion specifier */
            char *t;
            static const char fchar[] = {" +-#0!"};
            static const unsigned int fbit[] = {
                FSPACE, FPLUS, FMINUS, FHASH, FZERO, FBANG, 0};

            ++s;    /* skip the % sign */
            
            /* process conversion flags */
            for ( fspec.flags = 0; *s != '\0' && ( t = STRCHR(fchar, *s) ) != NULL; ++s )
                fspec.flags |= fbit[t - fchar];

            /* process width */
            if ( *s == '*' )
            {
                fspec.width = va_arg( ap, int );
                if ( fspec.width < 0 )
                {
                    fspec.width = -fspec.width;
                    fspec.flags |= FMINUS;
                }
                ++s;
            }
            else
                for ( fspec.width = 0; ISDIGIT( *s ); ++s )
                    if ( fspec.width < MAXWIDTH )
                        fspec.width = fspec.width * 10 + *s - '0';

            /* process precision */
            if ( *s != '.' )
                fspec.prec = -1;
            else if ( *++s == '*' )
            {
                fspec.prec = va_arg( ap, int );
                ++s;
            }
            else
                for ( fspec.prec = 0; ISDIGIT( *s ); ++s )
                    if ( fspec.prec < MAXPREC )
                        fspec.prec = fspec.prec * 10 + *s - '0';

            /* test for qualifier */
            fspec.qual = ( *s != '\0' && STRCHR( "hl", *s ) ) ? *s++: '\0';

            /* Continuation feature */
            if ( *s == '\0' )
            {
                fmt = va_arg( ap, const char * );
                continue;
            }

            /* now process the conversion type */
            n = do_conv( &fspec, VARGS(ap), *s, cons, &arg );
            if ( n < 0 )
                return EXBADFORMAT;
            else
                fspec.nChars += n;

            fmt = s + 1;
        }
    }
    
    return fspec.nChars;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
