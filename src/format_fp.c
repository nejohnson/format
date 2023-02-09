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

/** Floating point support **/

/*****************************************************************************/
/* System Includes                                                           */
/*****************************************************************************/

#include <float.h>

/*****************************************************************************/
/* Macros, constants                                                         */
/*****************************************************************************/

/* Smaller platforms only support single-precision for everything, and as
*  well as changing the floating point format it also affects the decimal 
*  mantissa type.
*/
#if ( DBL_DIG > 8 ) /* 64-bit doubles */
   #define DEC_MANT_REG_TYPE     unsigned long long
   #define DEC_1P0               ( 1000000000000000ULL )
   #define DEC_SIG_FIG           ( 16 )
   #define BIN_SIGN_WIDTH        ( 1 )
   #define BIN_EXP_WIDTH         ( 11 )
   #define BIN_EXP_BIAS          ( 1023 )
   #define BIN_MANT_WIDTH        ( 52 )
   #define BIN_MANT_SINGLE_BIT   ( 1ULL )
#else /* 32-bit doubles */
   #define DEC_MANT_REG_TYPE     unsigned long
   #define DEC_1P0               ( 100000000UL )
   #define DEC_SIG_FIG           ( 9 )
   #define BIN_SIGN_WIDTH        ( 1 )
   #define BIN_EXP_WIDTH         ( 8 )
   #define BIN_EXP_BIAS          ( 127 )
   #define BIN_MANT_WIDTH        ( 23 )
   #define BIN_MANT_SINGLE_BIT   ( 1UL )
#endif

/** Generated constants - depend on platform settings **/
#define BIN_SIGN_MASK         ( ( 1 << BIN_SIGN_WIDTH ) - 1 )
#define BIN_SIGN_SHIFT        ( ( sizeof(double) * CHAR_BIT ) - BIN_SIGN_WIDTH )
#define BIN_EXP_MASK          ( ( 1 << BIN_EXP_WIDTH ) - 1 )
#define BIN_EXP_SHIFT         ( ( sizeof(double) * CHAR_BIT ) - BIN_SIGN_WIDTH - BIN_EXP_WIDTH )
#define BIN_MANT_MASK         ( ( BIN_MANT_SINGLE_BIT << BIN_MANT_WIDTH ) - 1 )
#define BIN_MANT_REG_TOP_BIT  ( BIN_MANT_SINGLE_BIT << ( ( sizeof(double) * CHAR_BIT ) - 1 ) )
#define BIN_MANT_LEFT_ALIGN   ( BIN_SIGN_WIDTH + BIN_EXP_WIDTH )

/** Macros to unpack the binary float **/
#define BIN_UNPACK_MANT(b)    ( (b) & BIN_MANT_MASK )
#define BIN_UNPACK_EXPO(b)    ( ((b) >> BIN_EXP_SHIFT) & BIN_EXP_MASK )
#define BIN_UNPACK_SIGN(b)    ( ((b) >> BIN_SIGN_SHIFT) & BIN_SIGN_MASK )

/** Macros to pack integer bit buckets into a binary float **/
#define BIN_PACK_MANT(b,v)      ( (b) = ((b) & ~BIN_MANT_MASK) | ( ((DEC_MANT_REG_TYPE)v) & BIN_MANT_MASK ) )
#define BIN_PACK_EXPO(b,v)      ( (b) = ((b) & ~((DEC_MANT_REG_TYPE)BIN_EXP_MASK << BIN_EXP_SHIFT) ) | (((((DEC_MANT_REG_TYPE)v) & BIN_EXP_MASK) + BIN_EXP_BIAS) << BIN_EXP_SHIFT ) )
#define BIN_PACK_SIGN(b,v)      ( (b) = (((b) & ~((DEC_MANT_REG_TYPE)BIN_SIGN_MASK << BIN_SIGN_SHIFT)) | ((((DEC_MANT_REG_TYPE)v) & BIN_SIGN_MASK) << BIN_SIGN_SHIFT)) )

/** Check for NAN or INF **/
#define DEC_FP_IS_NAN(s,m,e)   ( (e) == INT_MAX && (m) != 0 )
#define DEC_FP_IS_INF(s,m,e)   ( (e) == INT_MAX && (m) == 0 )

/** Other fudge factors **/
/* Compressed notation (engineering and scientific) only work within the
   ranges specified by the International Bureau of Weights and Measures.
   Ref: Resolution 4 of the 19th meeting of the CGPM (1991)
        http://www.bipm.org/en/CGPM/db/19/4/
*/
#define COMP_EXP_LIMIT		( 24 )

/*****************************************************************************/
/* Private function prototypes.  Declare as static.                          */
/*****************************************************************************/

static void radix_convert( double, unsigned int *, DEC_MANT_REG_TYPE *, int * );

static int mant_to_char( char *, DEC_MANT_REG_TYPE, int, int );

static int do_conv_fp( T_FormatSpec *, va_list *, char,
                       void * (*)(void *, const char *, size_t), void * * );

static int do_conv_infnan( T_FormatSpec *, char,
                           void *          (*)(void *, const char *, size_t),
                           void * *,
                           unsigned int, DEC_MANT_REG_TYPE, int );

static void round_mantissa( DEC_MANT_REG_TYPE *, int *, int, int, int );

/*****************************************************************************/
/* Private functions.  Declare as static.                                    */
/*****************************************************************************/

/*****************************************************************************/
/**
   Convert a binary IEEE 754 double-precision number from radix-2 to radix-10.

   (Note: on smaller platforms double is the same size as float, so read the
          following with that in mind if you're looking at a small system)

   Implement an exponent conversion from base-2 to base-10.  This is most
   useful in printing a floating point number without using any floating
   point math routines.

   The result is DEC_SIG_FIG digits.  For 64-bit double precision that
   equates to 16 digits in total.

   The usual exceptions are supported and are compatible with IEEE 754:
      -inf   sign = 1, mantissa  = 0, exponent = MAX
      +inf   sign = 0, mantissa  = 0, exponent = MAX
      NaN    (ignored) mantissa != 0, exponent = MAX

   The result is a decimal number in the mantissa of the form

         D[.]ddddddddddddddd

   where D is a single decimal digit
         dd...d is the (DEC_SIG_FIG - 1) digit fractional part
         the decimal point [.] is assumed

   The sign is the same as for the radix-2 number.
   The exponent is in powers of 10.

   @param v             Input value to be converted
   @param d_sign        Output sign (0 = +ve, 1 = -ve)
   @param d_mantissa    Output mantissa
   @param d_exponent    Output exponent
**/
static void radix_convert( double              v,
                           unsigned int       *d_sign,
                           DEC_MANT_REG_TYPE  *d_mantissa,
                           int                *d_exponent )
{
    union {
        double            fv;
        DEC_MANT_REG_TYPE bits;
    } u;
    struct {
        DEC_MANT_REG_TYPE  mantissa;
        int                exponent;
        unsigned int       sign;
    } bin, dec;

    /* Get the double value into a bit-addressable format with a union and then
    *  extract the mantissa, exponent and sign fields.
    */
    u.fv = v;
    bin.mantissa = BIN_UNPACK_MANT( u.bits );
    bin.exponent = (int)BIN_UNPACK_EXPO( u.bits );
    bin.sign     = (unsigned int)BIN_UNPACK_SIGN( u.bits );

#if ( DBL_DIG > 8 )
    DEBUG_LOG( "FP: bin.m = 0x%llX\n", bin.mantissa );
#else
    DEBUG_LOG( "FP: bin.m = 0x%lX\n", bin.mantissa );
#endif
    DEBUG_LOG( "    bin.e = %d\n", bin.exponent );
    DEBUG_LOG( "    bin.s = %d\n", bin.sign );

    /* Check for +/-inf and NaN.  The indication is the radix-2 exponent being
    *  all-1s.  We mark this in the radix-10 domain by setting the exponent to
    *  INT_MAX.
    */
    if ( bin.exponent == BIN_EXP_MASK )
    {
        dec.exponent = INT_MAX;
        dec.sign     = bin.sign;
        dec.mantissa = bin.mantissa;
    }
    /* Zero (both + and -) is a special case */
    else if ( bin.mantissa == 0 && bin.exponent == 0 )
    {
        dec.exponent = 0;
        dec.mantissa = 0;
        dec.sign     = bin.sign;
    }
    else
    {
        DEC_MANT_REG_TYPE inc;

        dec.sign     = bin.sign;

        /* Load initial conditions for conversion */
        if ( bin.exponent == 0 ) /* then this is a denormal */
        {
            bin.exponent = 1;
            dec.mantissa = 0;
            dec.exponent = 0;
            
            while ( 0 == ( bin.mantissa & ( BIN_MANT_SINGLE_BIT << ( BIN_MANT_WIDTH - 1 ) ) ) )
            {
                bin.mantissa <<= 1;
                bin.exponent--;
            }
        }
        else /* this is a normal number with a assumed leading "1." */
        {
            dec.mantissa = DEC_1P0;
            dec.exponent = 0;
        }

        /* STEP 1: compute decimal mantissa */
        inc = ( DEC_1P0 + 1 ) / 2;
        bin.mantissa <<= BIN_MANT_LEFT_ALIGN;
        while ( bin.mantissa )
        {
            if ( bin.mantissa & BIN_MANT_REG_TOP_BIT )
                dec.mantissa += inc;

            bin.mantissa <<= 1;
            inc = ( inc + 1 ) / 2;
        }

        /* STEP 2: convert base-2 exponent to base-10 exponent,
         *          adjusting mantissa accordingly.
         *         treat positive and negative exponents separately to keep as
         *          much information in the mantissa as possible.
         */
        bin.exponent -= BIN_EXP_BIAS;  /* Subtract exponent bias */
        for ( ; bin.exponent > 0; bin.exponent-- )
        {
            dec.mantissa *= 2;
            if ( dec.mantissa >= ( DEC_1P0 * 10 ) )
            {
                dec.mantissa = ( dec.mantissa + 5 ) / 10;
                dec.exponent++;
            }
        }
        for ( ; bin.exponent < 0; bin.exponent++ )
        {
            if ( dec.mantissa < ( ( DEC_1P0 * 2 ) ) )
            {
                dec.mantissa *= 10;
                dec.exponent--;
            }
            dec.mantissa = ( dec.mantissa + 1 ) / 2;
        }
    }

    if ( d_sign     ) *d_sign     = dec.sign;
    if ( d_mantissa ) *d_mantissa = dec.mantissa;
    if ( d_exponent ) *d_exponent = dec.exponent;
}

/******************************************************************************/
/**
    Convert part of mantissa into an array of decimal digit characters.

    When removing unwanted digits we might be required to round.  If rounding
    is required then we only round with the last digit to be removed.

    @param buf                  Output buffer
    @param m                    Input mantissa
    @param digits_total         Total number of input digits
    @param digits_to_convert    Number of digits required in output

    @return number of digits put into @a buf.
**/
static int mant_to_char( char * buf,
                         DEC_MANT_REG_TYPE m,
                         int digits_total,
                         int digits_to_convert )
{
    int i;

    for ( i = digits_total - digits_to_convert; i > 0; i-- )
        m /= 10;

    for ( i = digits_to_convert; i > 0; i-- )
    {
        unsigned int d = (unsigned int)(m % 10);

        buf[i-1] = '0' + d;
        m /= 10;
    }

    return digits_to_convert;
}

/*****************************************************************************/
/**
    Process the floating point infinity and NaN values.

    "A double argument representing an infinity is converted in one of the
    styles [-]inf or [i]infinity - which style is implementation-defined.  A
    double argument representing a NaN is converted in one of the styles [-]nan
    or [-]nan(n-char-seq) - which style, and the meaning of any n-char-seq, is
    implementation-defined.  The F conversion specifier produces INF, INFINITY,
    or NAN instead of inf, infinity or nan, respectively."

    @param pspec        Pointer to format specification.
    @param ap           Reference to optional format arguments list.
    @param code         Conversion specifier code.
    @param cons         Pointer to consumer function.
    @param parg         Pointer to opaque pointer updated by cons.
    @param sign         0 = +ve, 1 = -ve
    @param mantissa     Decimal-coded matissa, of form d[.]ddddddd
    @param exponent     Radix-10 exponent.

    @return Number of emitted characters, or EXBADFORMAT if failure
**/
static int do_conv_infnan( T_FormatSpec *     pspec,
                           char               code,
                           void *          (* cons)(void *, const char *, size_t),
                           void * *           parg,
                           unsigned int       sign,
                           DEC_MANT_REG_TYPE  mantissa,
                           int                exponent )
{
    const char * pfx_s;
    size_t pfx_n;
    const char * e_s;
    size_t e_n;
    size_t ps1 = 0, ps2 = 0;

    if ( DEC_FP_IS_NAN( sign, mantissa, exponent ) )
        if ( code == 'f' || code == 'e' || code == 'g' )
            e_s = "nan";
        else
            e_s = "NAN";
    else
        if ( code == 'f' || code == 'e' || code == 'g' )
            e_s = "inf";
        else
            e_s = "INF";

    e_n = STRLEN(e_s);

    if ( sign )
        pfx_s = "-";
    else if ( !sign && pspec->flags & FPLUS )
        pfx_s = "+";
    else if ( !sign && pspec->flags & FSPACE )
        pfx_s = " ";
    else
        pfx_s = "";
    pfx_n = STRLEN( pfx_s );

    calc_space_padding( pspec, e_n + pfx_n, &ps1, &ps2 );

    return gen_out( cons, parg, ps1, pfx_s, pfx_n, 0, e_s, e_n, ps2 );
}

/*****************************************************************************/
/**
    Round the mantissa according to the conversion type and precision

    @param mantissa 		Pointer to mantissa to round
    @param exponent		Pointer to exponent
    @param prec			Required precision
    @param is_f                 True for f/F conversion
    @param compressed           True if using compressed engineering or
                                   scientific formatting
**/
static void round_mantissa( DEC_MANT_REG_TYPE *mantissa, int *exponent,
                            int prec, int is_f, int compressed )
{
   /* The precision tells us the number of digits that will be sent to the
      output.  The exponent tells us how many of the digits in the
      mantissa are after the decimal point.

    */

   DEC_MANT_REG_TYPE addend = DEC_1P0 * 5;
   int shift = 0;
   int e = *exponent;

   if ( compressed )
   {
      e %= 3;

      if ( e < 0 )
         e += 3;

      if ( is_f )
      {
         /* Scientific notation has limited suffixes */
         int absexp = ABS(*exponent);
         if ( absexp > COMP_EXP_LIMIT ) e += ( absexp - COMP_EXP_LIMIT );
      }

      DEBUG_LOG( "round_mantissa(): compressed exponent = %d\n", e );
   }

   if ( !is_f )
   {
      /* e/E always has one digit to the left of DP */

      if ( e < 0 ) e++;
      e = ABS(e);
   }
   shift = e + prec + 1;
   shift = MAX( shift, 0 );

   DEBUG_LOG( "round_mantissa(): shift = %d\n", shift );


   while ( shift-- )
      addend /= 10;

   *mantissa += addend;

   /* Catch integer portion overflow */
   if ( *mantissa >= ( DEC_1P0 * 10 ) )
   {
      *mantissa = ( *mantissa + 5 ) / 10;
      *exponent += 1;
      DEBUG_LOG( "round_mantissa(): integer overflow (new exponent: %d)\n", *exponent );
   }
}

/*****************************************************************************/
/**
    Process the floating point %e, %E, %f and %F conversions and the pseudo
    %g and %G conversions.

    Generating e/E output
    ---------------------

    "A double argument representing a floating point number is converted in the
    style [-]d.ddde[+/-]dd, where there is one digit (which is non-zero if the
    argument is nonzero) before the decimal point character and the number of
    digits after it is equal to the precision; if the precision is missing,
    it is taken as 6; if the precision is zero and the # flag is not specified,
    no decimal point character appears.  The value is rounded to the appropriate
    number of digits.  The E conversion specifier produces a number with 'E'
    instead of 'e' introducing the exponent.  The exponent always contains at
    least two digits, and only as many more digits as necessary to represent
    the exponent.  If the value is zero, the exponent is zero."


    Generating f/F output
    ---------------------

    "A double argument representing a floating point number is converted to
    decimal notation in the style [-]ddd.ddd, where the number of digits after
    the decimal point character is equal to the precision specification.  If the
    precision is missing, it is taken as 6; if the precision is zero and the
    # flag is not specified, no decimal point character appears.  If a decimal
    point character appears, at least one digit appears before it.  The value
    is rounded to the appropriate number of digits."

   
    A unified output model
    ----------------------

    Aligning the two formats above each other identifies the similarities:

    E:  [space+][sign?][zero+][digit]        [.]       [digit+][zero+][eE][sign][digit+][space+]
           PS1           PZ1                            n_right  PZ4              n_exp    PS2

    F:  [space+][sign?][zero+][digit+][zero+][.][zero+][digit+][zero+]                  [space+]
           PS1           PZ1   n_left   PZ2       PZ3   n_right  PZ4       n_exp = 0       PS2


    In the e/E case PZ2 and PZ3 are 0 and n_left is 1, while for the f/F case 
    n_exp is zero.
    Many of these fields are optional, even the DP if no following digits.


    Function Parameters
    -------------------

    @param pspec        Pointer to format specification.
    @param ap           Reference to optional format arguments list.
    @param code         Conversion specifier code.
    @param cons         Pointer to consumer function.
    @param parg         Pointer to opaque pointer updated by cons.
    @param sign         0 = +ve, 1 = -ve
    @param mantissa     Decimal-coded matissa, of form d[.]ddddddd
    @param exponent     Radix-10 exponent.

    @return Number of emitted characters, or EXBADFORMAT if failure
**/
static int do_conv_efg( T_FormatSpec *     pspec,
                        char               code,
                        void *          (* cons)(void *, const char *, size_t),
                        void * *           parg,
                        unsigned int       sign,
                        DEC_MANT_REG_TYPE  mantissa,
                        int                exponent )
{
    int sigfig = 0;
    const char * pfx_s;
    size_t pfx_n;
    char   e_s[DEC_SIG_FIG];
    size_t e_n = 0;
    size_t length = 0;
    size_t ps1 = 0, ps2 = 0;
    size_t pz1 = 0, pz2 = 0, pz3 = 0, pz4 = 0;
    int count = 0;
    int n;
    int want_dp = 0;
    int n_left, n_right;
    int i;
    size_t n_exp = 0;
    int really_g = 0;
    int is_f = 0;
    char si = '\0';

    /************************************************************************/
    DEBUG_LOG( ">>>> do_conv_efg with %%%c: ", code );
    DEBUG_LOG( "width: %d, ", pspec->width );
    DEBUG_LOG( "prec: %d, ", pspec->prec );
    DEBUG_LOG( "flags: %X, ", pspec->flags );
    DEBUG_LOG( "sign: %d, ", sign );
    DEBUG_LOG( "mantissa: %lld, ", mantissa );
    DEBUG_LOG( "exponent: %d\n", exponent );
    /************************************************************************/

    /* "g,G   ... style e (or E) is used only if the exponent resulting from
     *        such a conversion is less than -4 or greater than or equal to the
     *        precision."
     */
    if ( code == 'g' || code == 'G' )
    {
        /* Make a note that we were called with 'g' or 'G'. */
        really_g = 1;

        /* Turn off the '!' flag - just too messy with g/G */
        pspec->flags &= ~FBANG;

        /* If the precision is zero, it is taken as 1. */
        if ( pspec->prec == 0 )
            pspec->prec = 1;

        /* Then convert the g/G into e/E or f/F as appropriate. */
        if ( exponent < -4 || exponent >= pspec->prec )
            code = (code == 'g') ? 'e' : 'E';
        else
            code = (code == 'g') ? 'f' : 'F';
    }

    if ( code == 'f' || code == 'F' )
        is_f = 1;   

    /* Apply default precision */
    if ( pspec->prec < 0 )
        pspec->prec = 6;

    /* Generate the prefix, if any */
    if ( sign )
        pfx_s = "-";
    else if ( !sign && pspec->flags & FPLUS )
        pfx_s = "+";
    else if ( !sign && pspec->flags & FSPACE )
        pfx_s = " ";
    else
        pfx_s = "";
    pfx_n = STRLEN( pfx_s );

    /* Perform any rounding on the mantissa prior to formatting */
    round_mantissa( &mantissa, &exponent, pspec->prec, is_f, (int)(pspec->flags & FBANG) );

    /* Trim trailing zeros from mantissa and compute no. of sig.figures */
    if ( mantissa )
        for ( sigfig = DEC_SIG_FIG; sigfig > 0; sigfig--, mantissa /= 10 )
            if ( mantissa % 10 )
                break;

    DEBUG_LOG( "sigfig: %d\n", sigfig );

    /* Work out how many digits on each side of the DP */
    if ( is_f )
    {
        if ( pspec->flags & FBANG )
        {
            static char sitab[] = { 'y', 'z', 'a', 'f', 'p', 'n', 'u', 'm',
                                    '\0', 
                                    'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
            int idx = (int)NELEMS(sitab) / 2;

            while ( idx > 0 && idx < ((int)NELEMS(sitab) - 1) )
            {
                if ( exponent >= 3 ) { idx++; exponent -= 3; continue; }
                if ( exponent <  0 ) { idx--; exponent += 3; continue; }
                break;
            }
            si = sitab[idx];
        }

        n_left = exponent > -1 ? 1 + exponent : 0;
    }
    else /* must be 'e' */
    {
        n_left = 1;

        /* Engineering format forces exponent to multiple of 3 */
        if ( pspec->flags & FBANG )
        {
           int m = exponent % 3;

           if ( m < 0 )
              m += 3;
           n_left   += m;
           exponent -= m;
        }
    }

    n_right = MIN( MAX( sigfig - n_left, 0 ), pspec->prec );

    /* The g-as-f conversion strips out additional digits */
    if ( is_f && really_g )
    {
        DEC_MANT_REG_TYPE  m = mantissa;

        /* strip extraneous digits */
        for ( i = sigfig; i > n_left + n_right; i--, m /= 10 );

        /* strip trailing zeros */
        for ( ; n_right > 0 && m % 10 == 0; m /= 10, n_right-- );
    }

    DEBUG_LOG( "n_left: %d ", n_left );
    DEBUG_LOG( "n_right: %d\n", n_right );

    /* Compute length of the actual generated text */
    length = pfx_n + n_left + n_right;

    if ( is_f )
    {
        /* If nothing on the left make sure we have a '0' */
        if ( n_left == 0 )
        {
            pz1 = 1;
            length++;
        }

        /* Add any zero padding after figures but before DP */
        if ( n_left > sigfig )
            pz2 = (size_t)(n_left - sigfig);

        /* Add any zero padding between DP and figures on right */
        if ( exponent < -1 && pspec->prec > 0 )
        {
            int x = -1 - exponent;
            pz3 = (size_t)MIN( x, pspec->prec );
            length += pz3;
        }

        /* Include any SI multiplier suffix */
        if ( si )
            length += 2;
    }
    else /* is 'e' */
    {
        /* Add length of exponent suffix, remembering that the length of the
         *  exponent field is minimum of 2.
         */
        for ( i = ABS(exponent), n_exp = 0; i > 0; n_exp++, i /= 10 )
            ;

        n_exp = MAX( n_exp, 2 );

        /* Total length = 'e'/'E' + sign + 'dd..d' */
        length += 2 + n_exp;
    }

    /* Compute trailing zeros */
    if ( (int)(pz3 + n_right) < pspec->prec
         /* g,G     ... Trailing zeros are removed from the fractional portion
          *         of the result unless the # flag is specified; ...
          */
        && !( really_g && !(pspec->flags & FHASH) )
       )
    {
        pz4 = pspec->prec - pz3 - n_right;
        length += pz4;
    }
    else if ( is_f && (int)(pz3 + n_right) > pspec->prec )
    {
        int x = (int)pz3 + n_right - pspec->prec;
        length  -= x;
        n_right -= x;
    }

    /* Add DP if required */
    if ( ( pz3 || pz4 ) || n_right > 0 || pspec->flags & FHASH )
    {
        want_dp = 1;
        length++;
    }

    calc_space_padding( pspec, length, &ps1, &ps2 );

    /* Convert space padding into zero padding if we have the ZERO flag */
    if ( pspec->flags & FZERO )
    {
        pz1 += ps1;
        ps1 = 0;
    }

    /************************************************************************/
    DEBUG_LOG( "pz1: %zd, ", pz1 );
    DEBUG_LOG( "pz2: %zd, ", pz2 );
    DEBUG_LOG( "pz3: %zd, ", pz3 );
    DEBUG_LOG( "pz4: %zd, ", pz4 );
    DEBUG_LOG( "ps1: %zd, ", ps1 );
    DEBUG_LOG( "ps2: %zd, ", ps2 );
    DEBUG_LOG( "want_dp: %d\n", want_dp );
    DEBUG_LOG( "n_left: %d, ", n_left );
    DEBUG_LOG( "n_right: %d, ", n_right );
    DEBUG_LOG( "n_exp: %zd, ", n_exp );
    DEBUG_LOG( "length: %zd\n", length );
    /************************************************************************/

    /* Generate the output sections */

    /* LEFT, including leading space and prefix */
    e_n = n_left ? (size_t)mant_to_char( e_s, mantissa, sigfig, (int)(n_left - pz2) )
                 : 0;

    sigfig -= e_n;

    n = gen_out( cons, parg, ps1, pfx_s, pfx_n, pz1, e_s, e_n, 0 );
    if ( n == EXBADFORMAT )
        return n;
    count += n;

    /* Add any zeros before DP */
    n = gen_out( cons, parg, 0, NULL, 0, pz2, NULL, 0, 0 );
    if ( n == EXBADFORMAT )
        return n;
    count += n;

    /* RIGHT */
    e_n = n_right ? (size_t)mant_to_char( e_s, mantissa, sigfig, n_right )
                  : 0;

    n = gen_out( cons, parg, 0, ".", (size_t)(want_dp ? 1 : 0), pz3, e_s, e_n, 0 );
    if ( n == EXBADFORMAT )
        return n;
    count += n;

    /* Trailing zeros, if any */
    n = gen_out( cons, parg, 0, NULL, 0, pz4, NULL, 0, 0 );
    if ( n == EXBADFORMAT )
        return n;
    count += n;

    /* EXPONENT */
    if ( n_exp )
    {
        unsigned int absexp = (unsigned int)ABS(exponent);
        int nx;
        char epfx_s[2];

        /* Exponent prefix comprises the letter 'e' or 'E' and a +/- sign */
        epfx_s[0] = code;
        epfx_s[1] = ( exponent < 0 ) ? '-' : '+';

        for ( nx = (int)n_exp, e_n = 0; nx > 0; nx--, e_n++, absexp /= 10 )
            e_s[nx-1] = (char)( absexp % 10 ) + '0';

        n = gen_out( cons, parg, 0, epfx_s, sizeof(epfx_s), 0, e_s, e_n, 0 );
        if ( n == EXBADFORMAT )
            return n;
        count += n;
    }

    /* SI multiplier and trailing space */
    n = gen_out( cons, parg, !!si, NULL, 0, 0, &si, !!si, ps2 );
    if ( n == EXBADFORMAT )
        return n;
    count += n;

    return count;
}

/*****************************************************************************/
/**
    Process the floating point conversions (%e, %E, %f, %F, %g, %G).

    We simplify things here by using the radix converter above and then using
    integer output functions with suitable format specs to emit the output.
    We also detect infinity and NaN at this early stage and treat them directly.

    That leaves g,G which is handled by the e,E and f,F conversions:

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

    @param pspec    Pointer to format specification.
    @param ap       Reference to optional format arguments list.
    @param code     Conversion specifier code.
    @param cons     Pointer to consumer function.
    @param parg     Pointer to opaque pointer updated by cons.

    @return Number of emitted characters, or EXBADFORMAT if failure
**/
static int do_conv_fp( T_FormatSpec * pspec,
                       va_list *      ap,
                       char           code,
                       void *      (* cons)(void *, const char *, size_t),
                       void * *       parg )
{
    double dv;
    unsigned int sign;
    DEC_MANT_REG_TYPE mantissa;
    int exponent;

    /* Do not support long doubles */
    if ( pspec->qual == 'L' )
        return EXBADFORMAT;

    dv = va_arg( *ap, double );
    radix_convert( dv, &sign, &mantissa, &exponent );

    /* Infs and NaNs are treated in the same style */
    if ( DEC_FP_IS_NAN( sign, mantissa, exponent )
      || DEC_FP_IS_INF( sign, mantissa, exponent ) )
    {
        return do_conv_infnan( pspec, code, cons, parg, sign, mantissa, exponent );
    }
    else
    {
        return do_conv_efg( pspec, code, cons, parg, sign, mantissa, exponent );
    }    
}

/****************************************************************************/
/**
    Process fixed-point conversion (%k).

    @param pspec    Pointer to format specification.
    @param ap       Reference to optional format arguments list.
    @param code     Conversion specifier code.
    @param cons     Pointer to consumer function.
    @param parg     Pointer to opaque pointer updated by cons.

    @return Number of emitted characters, or EXBADFORMAT if failure
**/
static int do_conv_k( T_FormatSpec * pspec,
                      va_list *      ap,
                      void *      (* cons)(void *, const char *, size_t),
                      void * *       parg )
{
    unsigned int total_bits = pspec->xp.w_int + pspec->xp.w_frac;
    size_t total_bytes = ( total_bits + 7 ) / 8;
    long v;
    unsigned int sign;
    DEC_MANT_REG_TYPE mantissa;
    int exponent;
    union {
        double d;
        DEC_MANT_REG_TYPE b;
    } u;
	u.d = 0.0;

    if ( total_bytes == 0 )
        return EXBADFORMAT;
    
    if ( total_bytes <= sizeof( int ) )
        v = (long)va_arg( *ap, int );
    else
        v = (long)va_arg( *ap, long );

    DEBUG_LOG( "k: val = 0x%8.8lX ", v );
    DEBUG_LOG( "w_int = %zd ", pspec->xp.w_int );
    DEBUG_LOG( "w_frac = %zd ", pspec->xp.w_frac );

    if ( v == 0 ) /* handle zero as special case */
    {
        sign     = 0;
        mantissa = 0;
        exponent = 0;
    }
    else
    {
        int i;

        /* Extract sign bit */
        sign = ( v >> ( total_bits - 1 ) ) & 0x01;

        /* If the sign bit is set (number is negative) then apply 2's complement
         * sign inversion.  We can use the built-in type sign inversion as long
         * as we then mask out any bits not in the fixed-point type.
         */
        if ( sign )
            v = -v;

        /* Mask out any dross from the input value and save for later */
        v &= ( 1 << ( total_bits - 1) ) - 1;
        mantissa = (DEC_MANT_REG_TYPE)v;

        /* Work out where highest bit is */
        for ( i = -1; v != 0; i++ )
            v >>= 1;

        /* i gives index of highest '1' bit, which then gives us the exponent */
        DEBUG_LOG( "i = %d ", i );
        exponent = i - pspec->xp.w_frac;
        
        /* Shift up the mantissa until the top-most bit pops out of the top
         * of the mantissa, which will then get masked out, which is exactly
         * what we want - in floating point the '1' is implied.
         */
        while ( (mantissa & ~BIN_MANT_MASK) == 0 )
            mantissa <<= 1;

        DEBUG_LOG("mantissa: %llu\n", mantissa);

        /* Now pack everthing into the binary FP */
        BIN_PACK_MANT( u.b, mantissa );
        BIN_PACK_EXPO( u.b, exponent );
        BIN_PACK_SIGN( u.b, sign     );

        /* Convert to decimal components */
        radix_convert( u.d, &sign, &mantissa, &exponent );
    }
    
    return do_conv_efg( pspec, 'f', cons, parg, sign, mantissa, exponent );
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
