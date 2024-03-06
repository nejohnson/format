# microformat #

Copyright (c) 2010-2023 Neil Johnson

# SUMMARY #

A very feature-reduced platform-independent implementation of the core of the printf
family of functions, ideally suited to system logging purposes.

# INTRODUCTION #

In most standard C libraries the `printf` family of functions can be rather
heavy to use - they pull in quite a lot of additional library code, and they
often require considerable additional effort to support on small or even
 medium-sized embedded projects.

The `microformat` library answers this need.  It provides an extremely small, efficient core
function which implements a useful subset of the `printf` conversions, requires
little in the way of system support, and can be easily ported to work with a
wide range of output devices.  Based on the `format` library it reduces the feature
set down to a very minimal core, described below.  Target machine support is limited
to CPUs with 16-bit pointer and integer sizes.


# SYNOPSIS #

```
#include "microformat.h"
int microformat( const char *fmt, va_list ap );
```


# DESCRIPTION #

The `microformat` function sends strings of one or more characters to the system-supplied
function `format_putchar` under the control of the string pointed to by `fmt` that specifies
how the subsequent arguments in `ap` are converted for output.  If there are
insufficient arguments for the format, the behaviour is undefined.  The `microformat`
function returns when the end of the format string is encountered.

The format string `fmt` is composed of zero or more directives: ordinary characters
(not `%`), which are sent unchanged to `format_putchar`; and conversion
specifications, each of which results in fetching zero or more subsequent arguments,
converting them, if applicable, according to the corresponding conversion specifier,
and then sending the result to `format_putchar`.


## The Output Function ##

```
int format_putchar( int c )
```

The output function `format_putchar` writes the character `c`, cast to an unsigned char,
to the output and returns the character written as an unsigned char cast to an int, 
or -1 on error.


## Conversion Specifiers ##

Each conversion specification is introduced by the character `%`.  After the
`%`, the following appear in sequence:

  * Zero or more flags (in any order) that modify the meaning of the conversion
    specification.

  * An optional minimum field width. If the converted value has fewer characters
    than the field width, it is padded with spaces (by default) on the left (or
    right, if the left adjustment flag, described later, has been given) to the
    field width.

  * An optional precision that gives the minimum number of digits to appear for
    the `b`, `d`, `u`, `x`, and `X` conversions, the maximum number of bytes to
    be written for `s` conversions, or the number of repetitions for `c` 
    conversions. The precision takes the form of a period (.) followed by an
    optional decimal integer; if only the period is specified, the precision is
    taken as zero. If a precision appears with any other conversion specifier,
    it is ignored.

  * A conversion specifier character that specifies the type of conversion to be
    applied.


### Flags ###

The flag characters and their meanings are:

| Flag | Description |
|:---|:---|
|`-`|   The result of the conversion is left-justified within the field.  It is right-justified if this flag is not specified.|
|`+`|   The result of a signed conversion always begins with a plus or minus sign. It begins with a sign only when a negative value is converted if this flag is not specified.|
|space| If the first character of a signed conversion is not a sign, or if a signed conversion results in no characters, a space is prefixed to the result. If the space and `+` flags both appear, the space flag is ignored.|
|`0`|   For `b`, `d`, `u`, `x` and `X` conversions, leading zeros (following any indication of sign) are used to pad to the field width rather than performing space padding. If the `0` and `-` flags both appear, the `0` flag is ignored.  For `b`, `d`, `u`, `x`, and `X` conversions, if a precision is specified, the `0` flag is ignored. For other conversions, the flag is ignored.|


### Conversion Specifiers ###

The conversion specifiers and their meanings are:

| Specifier | Description |
|:---|:---|
|`d`| The `int` argument is converted to signed decimal (`d`) in the style `[`-`]`dddd. The precision specifies the minimum number of digits to appear; if the value being converted can be represented in fewer digits, it is expanded with leading zeros. The default precision is 1. The result of converting a zero value with a precision of zero is no characters. |
|`b`,`u`,`x`,`X`| The `unsigned int` argument is converted to unsigned binary (`b`), unsigned decimal (`u`), or unsigned hexadecimal notation (`x` or `X`) in the style `dddd`. The letters "A" to "F" are used for `X` conversions, and "a" to "f" for `x` conversions. The precision specifies the minimum number of digits to appear; if the value being converted can be represented in fewer digits, it is expanded with leading zeros.  The default precision is 1. The result of converting a zero value with a precision of zero is no characters.|
|`c`|         The `int` argument is converted to an `unsigned char`, and the resulting character is written.  The precision specifies how many times the character is written.  The default and minimum precision is 1.|
|`s`|         The argument is a pointer to the initial element of an array of character type. Characters from the array are written up to (but not including) the terminating null character. If the precision is specified, no more than that many bytes are written. If the precision is not specified or is greater than the size of the array, the array must contain a null character.  A NULL argument is treated as pointer to the string "(null)".|
|`p`|         The argument is a pointer to `void`. The value of the pointer is converted to a sequence of printing characters using the conversion specification `%4.4X`.|
|`%`|         A `%` character is written. No argument is converted. The complete conversion specification is `%%`.|

If a conversion specification is invalid, `microformat` returns an error code. If
any argument is not the correct type for the corresponding conversion
specification, bad mojo happens.

In no case does a nonexistent or small field width cause truncation of a field;
if the result of a conversion is wider than the field width, the field is
expanded to contain the conversion result.


## Return Value ##

The `microformat` function returns the number of characters sent to the output
function, or the negative value `EXBADFORMAT` if an output or encoding error
occurred.


## Limits ##

The maximum width and precision are 80.  It is an error if values larger
than this are specified.


# EXAMPLES #

The first example implements the same behaviour as the standard C library
function `printf`.  The output function is written for a mythical machine with two
hardware registers, one is true if the UART is ready, the other is the
transmit register into which the next character to send is written:

```
int format_putchar( int c )
{
    while( !UART_TX_READY ) /* wait */ ;
    UART_TX_REG = c;
}
```

The `printf` function then wraps around `microformat` to marshall the arguments
into the correct form:
```
int printf ( const char *fmt, ... )
{
    va_list arg;
    int done;

    va_start ( arg, fmt );
    done = microformat( fmt, arg );
    va_end ( arg );

    return done;
}
```

