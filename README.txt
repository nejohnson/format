format

Copyright(c) 2010-2019 Neil Johnson


SUMMARY

A platform-independent implementation of the core of the printf family of
functions with significant extensions for formatting output.


INTRODUCTION

In most standard C libraries the printf family of functions can be rather heavy
to use - they pull in quite a lot of additional library code, and they often 
require considerable additional effort to support on small or even medium-sized 
embedded projects.

The "format" library answers this need.  It provides a small, efficient core 
function which implements the majority of the printf conversions, requires 
little in the way of system support, and can be easily ported to work with a 
wide range of output devices.


SYNOPSIS

#include "format.h"
int format( void * (*cons) (void *a, const char *s , size_t n),
             void * arg, const char *fmt, va_list ap );


DESCRIPTION

The "format" function sends strings of one or more characters to the consumer
function "cons" under the control of the string pointed to by "fmt" that
specifies how the subsequent arguments in "ap" are converted for output.  If
there are insufficient arguments for the format, the behaviour is undefined. 
The "format" function returns when the end of the format string is encountered.

The format string "fmt" is composed of zero or more directives: ordinary
characters (not "%"), which are sent unchanged to the consumer function; and
conversion specifications, each of which results in fetching zero or more
subsequent arguments, converting them, if applicable, according to the
corresponding conversion specifier, and then sending the result to the consumer
function.


THE CONSUMER FUNCTION

void * cons( void *a, const char *s, size_t n )

The consumer function "cons" takes an opaque pointer "a", a pointer to an array 
of characters "s" and the number "n" of characters to consume from "s".  It 
returns another opaque pointer which may be equal or different to "a" which will 
be passed to the next call to "cons".  The consumer function returns NULL to
indicate an error condition, which will cease any further format processing
and cause the format function to terminate with the EXBADFORMAT error code.

The first opaque pointer passed to the first call to "cons" is supplied as the
argument "arg" to the call to "format" (see above).


CONVERSION SPECIFIERS
 
Each conversion specification is introduced by the character %.  After the %, 
the following appear in sequence: 

= Zero or more flags (in any order) that modify the meaning of the conversion
  specification. 

= An optional minimum field width. If the converted value has fewer characters
  than the field width, it is padded with spaces (by default) on the left (or 
  right, if the left adjustment flag, described later, has been given) to the 
  field width.  The field width takes the form of an asterisk * (described 
  later) or a decimal integer. 

= An optional precision that gives the minimum number of digits to appear for 
  the b, d, i, I, o, u, U, x, and X conversions, the number of digits to appear
  after the decimal-point character for e, E, f, F and k conversions, the
  maximum number of significant digits for the g and G conversions, the
  maximum number of bytes to be written for s conversions, or the number of 
  repetitions of the character for C conversions. The precision takes the form 
  of a period (.) followed either by an asterisk * (described later) or by an 
  optional decimal integer; if only the period is specified, the precision is
  taken as zero.
  
= An optional number base modifier that specifies the numeric base to be used
  by the i, I, u and U conversions. The base takes the form of a colon (:)
  followed by either an asterisk * (described later) or by an optional decimal 
  integer; if only the colon is specified the base is taken as decimal.
  
= An optional grouping modifier that specifies how digits are to be grouped for
  the b, d, i, I, o, u, U, x, and X conversions.  It is ignored for all other 
  conversion specifiers.

  A grouping modifier starts with `[` and ends with `]`.  Within the parentheses
  are symbol-number group specifiers.  The number specifies the number of digits
  within that group; the symbol specifies the character that is inserted after 
  the group.  A value of zero disables that particular group.  Unless otherwise
  terminated the left-most grouping specifier is repeatedly applied to any
  remaining digits.  If a group specifier begins with `-` then this terminates 
  any further grouping.  Group specifications are processed right-to-left, 
  starting with the rightmost symbol-number group and processing leftwards.

  An asterisk * (described later) in place of the number takes the next int 
  argument after the value to be converted (note this is contra to the width and
  precision specifiers).  If the value is negative it is treated as a '-', 
  terminating any further grouping.

= An optional fixed-point modifier that specifies the widths of the integer and
  fractional parts of the argument to the k conversion.  It is ignored for all
  other conversion specifiers.  A fixed-point modifier starts with `{` and ends 
  with `}`.  Within the parentheses are an optional integer width specifier, 
  a period (.), and an optional fractional width specifier.  Both specifiers are 
  non-negative decimal integers, or asterisks * (described later) where negative 
  values are interpreted as zero.

= An optional length modifier that specifies the size of the argument.

= A conversion specifier character that specifies the type of conversion to be 
  applied. 

As noted above, a field width, precision, base, or any combination, may be 
indicated by an asterisk. In this case, an int argument supplies the field width
or precision.  The arguments specifying field width, or precision, or both, 
shall appear (in that order) before the argument (if any) to be converted. A 
negative field width argument is taken as a "-" flag followed by a positive 
field width. A negative precision argument is taken as if the precision were 
omitted. 

The flag characters and their meanings are: 

-          The result of the conversion is left-justified within the field.  It
           is right-justified if this flag is not specified.

^          The result of the conversion is centre-justified within the field.
           It is right-justified if this flag is not specified. When there is
           an odd number of padding spaces the result of the conversion is
           biased to the right. It is biased to the left if the - flag is also
           specified.

+          The result of a signed conversion always begins with a plus or minus 
           sign. It begins with a sign only when a negative value is converted
           if this flag is not specified.

space      If the first character of a signed conversion is not a sign, or if a 
           signed conversion results in no characters, a space is prefixed to
           the result. If the space and + flags both appear, the space flag is 
           ignored.

#          The result is converted to an alternative form. For o conversion, 
           it increases the precision, if and only if necessary, to force the 
           first digit of the result to be a zero (if the value and precision
           are both 0, a single 0 is printed). For x (or X or b) conversion, a  
           nonzero result has "0x" (or "0X" or "0b") prefixed to it.  For 
           continuation and s conversions, it indicates that the pointer 
           argument is of an alternate form.  For e, E, f, F, g and G 
           conversions, the result of converting a floating point number always 
           contains a decimal point character, even if no digits follow it 
           (normally, a decimal point character appears in the result of these 
           conversions only if a digit follows it.)  For g and G conversions 
           trailing zeros and not removed from the result.  For other 
           conversions, the flag is ignored.
           
!          For b, x and X conversions with the # flag the result is always 
           prefixed, even when zero.  For x and X conversions the prefix is 
           always "0x".  For e and E conversions the exponent is forced to a 
           multiple of three with one to three digits appearing before the 
           decimal point.  For f and F conversions the result of the conversion
           is formatted to use the SI multiplier suffixes, with one to three 
           digits appearing before the decimal point; where the exponent of the 
           conversion is less than -24 or greater than 27 the result will not 
           conform to this rule, although it will be correct. For other 
           conversions, the flag is ignored.

0          For b, d, i, I, o, u, U, x, and X conversions, leading zeros 
           (following any indication of sign or base) are used to pad to the 
           field width rather than performing space padding. If the 0 and - 
           flags both appear, the 0 flag is ignored.  For b, d, i, I, o, u, U, 
           x, and X conversions, if a precision is specified, the 0 flag is 
           ignored. For other conversions, the flag is ignored. 

The length modifiers and their meanings are: 

h          Specifies that a following b, d, i, I, o, u, U, x, or X conversion 
           specifier applies to a short int or unsigned short int argument (the 
           argument will have been promoted according to the integer promotions, 
           but its value shall be converted to short int or unsigned short int 
           before consuming); or that a following n conversion specifier applies
           to a pointer to a short int argument. 

l(ell)     Specifies that a following b, d, i, I, o, u, U, x, or X conversion 
           specifier applies to a long int or unsigned long int argument; or 
           that a following n conversion specifier applies to a pointer to a 
           long int argument.
           
j          Specifies that a following b, d, i, o, u, x, or X conversion 
           specifier applies to an intmax_t or uintmax_t argument; or that a 
           following n conversion specifier applies to a pointer to an intmax_t
           argument.

z          Specifies that a following b, d, i, o, u, x, or X conversion 
           specifier applies to a size_t or the corresponding signed integer 
           type argument; or that a following n conversion specifier applies to
           a pointer to a signed integer type corresponding to size_t argument.

t          Specifies that a following b, d, i, o, u, x, or X conversion 
           specifier applies to a ptrdiff_t or the corresponding unsigned 
           integer type argument; or that a following n conversion specifier 
           applies to a pointer to a ptrdiff_t argument.

L          Specifies that a following e, E, f, F, g, or G conversion specifier 
           applies to a long double argument.  Until further notice this is an
           unsupported feature and will return an error.

If a length modifier appears with any conversion specifier other than as 
specified above, the length modifier is ignored. 

The conversion specifiers and their meanings are: 

d,i,I        The int argument is converted to signed decimal (d) or signed 
             number of the specified base (i or I) in the style [-]dddd. The 
             base specifies the number base. For bases greater than decimal the
             letters 'A' to 'Z' are used for I conversions, and 'a' to 'z' for 
             i conversions. The precision specifies the minimum number of digits
             to appear; if the value being converted can be represented in fewer
             digits, it is expanded with leading zeros. The default precision is
             1. The result of converting a zero value with a precision of zero
             is no characters. 
             
b,o,u,U,x,X  The unsigned int argument is converted to unsigned binary (b), 
             unsigned octal (o), unsigned number of specified base (u or U), or 
             unsigned hexadecimal notation (x or X) in the style dddd. For 
             bases greater than decimal the letters 'A' to 'Z' are used for X 
             and U conversions, and 'a' to 'z' for x and u conversions. The 
             precision specifies the minimum number of digits to appear; if the 
             value being converted can be represented in fewer digits, it is 
             expanded with leading zeros.  The default precision is 1. The 
             result of converting a zero value with a precision of zero is no
             characters.
             
e, E         A double argument representing a floating point number is converted
             in the style [-]d.ddde[+/-]dd, where there is one digit (which is 
             non-zero if the argument is nonzero) before the decimal point
             character and the number of digits after it is equal to the 
             precision; if the precision is missing, it is taken as 6; if the 
             precision is zero and the # flag is not specified, no decimal 
             point character appears.  The value is rounded to the appropriate 
             number of digits.  The E conversion specifier produces a number 
             with `E` instead of `e` introducing the exponent.  The exponent 
             always contains at least two digits, and only as many more digits 
             as necessary to represent the exponent.  If the value is zero, the 
             exponent is zero.
             
             A double argument representing an infinity or NaN is converted in 
             the style of an f or F conversion specifier.

f,F          A double argument representing a floating point number is converted
             to decimal notation in the style [-]ddd.ddd, where the number of 
             digits after the decimal point character is equal to the precision
             specification.  If the precision is missing, it is taken as 6; if 
             the precision is zero and the # flag is not specified, no decimal 
             point character appears.  If a decimal point character appears, at 
             least one digit appears before it.  The value is rounded to the 
             appropriate number of digits.
             
             A double argument representing an infinity is converted to the 
             style [-]inf.  A double argument representing a NaN is converted
             in the style [-]nan.  The F conversion specifier produces INF or
             NAN instead of inf or nan respectively.

g,G          A double argument representing a floating point number is converted
             in the style f or e (on in style F or E in the case of a G 
             conversion specifier), with the precision specifying the number of 
             significant digits.  If the precision is zero, it is taken as 1.
             The style used depends on the value converted; style e (or E) is 
             used only if the exponent resulting from such a conversion is less 
             than -4 or greater than or equal to the precision.  Trailing zeros 
             are removed from the fractional portion of the result unless the 
             # flag is specified; a decimal point character appears only if it 
             is followed by a digit.
             
             A double argument representing an infinity or NaN is converted in 
             the style of an f or F conversion specifier.

k            An integer argument representing a signed fixed-point argument is 
             converted to decimal notation in the style of f.  The parameters of
             the fixed-point format are specified by the fixed-point modifier 
             described above.  The default fixed-point format is 16p16.

c            The int argument is converted to an unsigned char, and the
             resulting character is written.

C            The int argument is converted to an unsigned char, and the
             resulting character is written.  The precision specifies how many 
             times the character is written.  The default and minimum precision
             is 1, equivalent to the c conversion specifier.

s            The argument is a pointer to the initial element of an array of 
             character type. Characters from the array are written up to (but 
             not including) the terminating null character. If the precision is 
             specified, no more than that many bytes are written. If the 
             precision is not specified or is greater than the size of the 
             array, the array must contain a null character.  A NULL argument is
             treated as pointer to the string "(null)".

p            The argument is a pointer to void. The value of the pointer is 
             converted to a sequence of printing characters using the conversion
             specification %#!N.NX, where N is determined by the size of pointer 
             to int on the target machine. 

n            The argument is a pointer to signed integer into which is written
             the number of characters passed to the consumer function so far by 
             this call to "format".  No argument is converted, but one is 
             consumed. Any flags, a field width, or a precision will be ignored.
             A NULL argument is silently ignored.

%            A "%" character is written. No argument is converted. The complete 
             conversion specification is %%. 
           
"            The argument is treated as a continuation of the format
             specification.  Any flags, width, precision or length will be 
             ignored.

If a conversion specification is invalid, "format" returns an error code. If any 
argument is not the correct type for the corresponding conversion specification, 
bad mojo happens. 

In no case does a nonexistent or small field width cause truncation of a field;
if the result of a conversion is wider than the field width, the field is 
expanded to contain the conversion result. 


RETURN VALUE

The "format" function returns the number of characters sent to the consumer
function, or the negative value EXBADFORMAT if an output or encoding error
occurred. 


LIMITS

The maximum width and precision are 500.  It is an error if values larger than
this are specified.

The largest number base is 36.  The smallest is 2.  A base of 0 (the default) is
treated as decimal (base 10).  It is an error to specify a base of 1 or greater
than 36.


EXAMPLES

The first example implements the same behaviour as the standard C library
function printf.  First, the consumer function:

void * outfunc( void * op, const char * buf, size_t n )
{
    while ( n-- )
        putchar( *buf++ );

    return op;
}

In this case the opaque pointer is simply returned unchanged.
Second, the implementation of the printf function:

int printf ( const char *fmt, ... )
{
    va_list arg;
    int done;
    
    va_start ( arg, fmt );
    done = format( outfunc, (void *)!NULL, fmt, arg );
    va_end ( arg );
    
    return done;
}

Because the opaque pointer is not used, and the consumer function just returns
it, a non-NULL is passed to "format".

The second example illustrates how the opaque pointer is used to implement the
standard C library function sprintf.  In this example the consumer function
returns the address of the next location to receive any following characters:

void * bufwrite( void * memptr, const char * buf, size_t n )
{
    return ( memcpy( memptr, buf, n ) + n );
}

The implementation of sprintf is shown below, with an additional step to
append a null character to the end of the string written into buf:

int sprintf( char *buf, const char *fmt, ... )
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

The final example illustrates the use of the opaque pointer to support an
LCD display with (x,y) positioning.  In this example a struct data type
describes where the consumer function is to place its output:

struct coord {
	short x, y;
};

Also assume the LCD is 80 characters wide, and follows the usual convention of
top-left being (0,0).

The consumer function uses the coord to set the position for each character
sent to the LCD calling a driver function lcd_putc:

void * lcd_putat( void * ap, const char *s, size_t n )
{
	struct coord *pc = (struct coord *)ap;
	
	while ( n-- )
	{
		lcd_putc( pc->x++, pc->y, *s++ );
		if ( pc->x >= 80 )
		{
			pc->x = 0;
			pc->y++;
		}
	}
	
	return (void *)pc;
}

The implementation of the lcd_printf function itself:

int lcd_printf( struct coord loc, const char *fmt, ... )
{
    va_list arg;
    int done;
    
    va_start ( arg, fmt );
    done = format( lcd_putat, &loc, fmt, arg );
    va_end ( arg );
    
    return done;
}

And an example call to this function might look like this:

    struct coord loc;
    int temperature;
    int status;
    
    temperature = 32;
    loc.x = 5;
    loc.y = 2;
    status = lcd_printf( loc, "Boiler temp = %+d Celsius", temperature );
    if ( status < 0 )
    {
    	/* error handler */
    }

--




	
