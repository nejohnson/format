# tinyformat #

Copyright (c) 2010-2023 Neil Johnson

# SUMMARY #

A feature-reduced platform-independent implementation of the core of the printf
family of functions with significant extensions for formatting output.

# INTRODUCTION #

In most standard C libraries the `printf` family of functions can be rather
heavy to use - they pull in quite a lot of additional library code, and they
often require considerable additional effort to support on small or even
 medium-sized embedded projects.

The `tinyformat` library answers this need.  It provides a very small, efficient core
function which implements the majority of the `printf` conversions, requires
little in the way of system support, and can be easily ported to work with a
wide range of output devices.  Based on the `format` library it reduces the feature
set down to a minimal core, described below, while retaining the
same API as the original `format` library.  Target machine support is limited
to CPUs with 16-bit pointer and integer sizes.


# SYNOPSIS #

```
#include "format.h"
int format( void * (*cons) (void *a, const char *s , size_t n),
             void * arg, const char *fmt, va_list ap );
```


# DESCRIPTION #

The `format` function sends strings of one or more characters to the consumer
function `cons` under the control of the string pointed to by `fmt` that specifies
how the subsequent arguments in `ap` are converted for output.  If there are
insufficient arguments for the format, the behaviour is undefined.  The `format`
function returns when the end of the format string is encountered.

The format string `fmt` is composed of zero or more directives: ordinary characters
(not `%`), which are sent unchanged to the consumer function; and conversion
specifications, each of which results in fetching zero or more subsequent arguments,
converting them, if applicable, according to the corresponding conversion specifier,
and then sending the result to the consumer function.


## The Consumer Function ##

```
void * cons( void *a, const char *s, size_t n )
```

The consumer function `cons` takes an opaque pointer `a`, a pointer to an array
of characters `s` and the number `n` of characters to consume from `s`.  It
returns another opaque pointer which may be equal or different to `a` which
will be passed to the next call to `cons`.  The consumer function returns NULL
to indicate an error condition, which will cease any further format processing
and cause the `format` function to terminate with the `EXBADFORMAT` error code.

The first opaque pointer passed to the first call to `cons` is supplied as
the argument `arg` to the call to `format` (see above).


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
|`0`|   For `b`, `d`, `u`, `x` and `X` conversions, leading zeros (following any indication of sign or base) are used to pad to the field width rather than performing space padding. If the `0` and `-` flags both appear, the `0` flag is ignored.  For `b`, `d`, `u`, `x`, and `X` conversions, if a precision is specified, the `0` flag is ignored. For other conversions, the flag is ignored.|

### Conversion Specifiers ###

The conversion specifiers and their meanings are:

| Specifier | Description |
|:---|:---|
|`d`| The `int` argument is converted to signed decimal (`d`) in the style `[`-`]`dddd. The precision specifies the minimum number of digits to appear; if the value being converted can be represented in fewer digits, it is expanded with leading zeros. The default precision is 1. The result of converting a zero value with a precision of zero is no characters. |
|`b`,`u`,`x`,`X`| The `unsigned int` argument is converted to unsigned binary (`b`),  unsigned decimal (`u`), or unsigned hexadecimal notation (`x` or `X`) in the style `dddd`. For bases greater than decimal the letters "A" to "F" are used for `X` conversions, and "a" to "f" for `x` conversions. The precision specifies the minimum number of digits to appear; if the value being converted can be represented in fewer digits, it is expanded with leading zeros.  The default precision is 1. The result of converting a zero value with a precision of zero is no characters.|
|`c`|         The `int` argument is converted to an `unsigned char`, and the resulting character is written.  The precision specifies how many times the character is written.  The default and minimum precision is 1.|
|`s`|         The argument is a pointer to the initial element of an array of character type. Characters from the array are written up to (but not including) the terminating null character. If the precision is specified, no more than that many bytes are written. If the precision is not specified or is greater than the size of the array, the array must contain a null character.  A NULL argument is treated as pointer to the string "(null)".|
|`p`|         The argument is a pointer to `void`. The value of the pointer is converted to a sequence of printing characters using the conversion specification `%4.4X`.|
|`%`|         A `%` character is written. No argument is converted. The complete conversion specification is `%%`.|
|`"`|         The argument is a pointer to a string which is treated as a continuation of the format specification. Any flags, width, precision or length will be ignored.|

If a conversion specification is invalid, `format` returns an error code. If
any argument is not the correct type for the corresponding conversion
specification, bad mojo happens.

In no case does a nonexistent or small field width cause truncation of a field;
if the result of a conversion is wider than the field width, the field is
expanded to contain the conversion result.


## Return Value ##

The `format` function returns the number of characters sent to the consumer
function, or the negative value `EXBADFORMAT` if an output or encoding error
occurred.


## Limits ##

The maximum width and precision are 500.  It is an error if values larger
than this are specified.


# EXAMPLES #

The first example implements the same behaviour as the standard C library
function `printf`.  First, the consumer function:

```
void * outfunc( void * op, const char * buf, size_t n )
{
    while ( n-- )
        putchar( *buf++ );

    return (void *)( !NULL );
}
```

In this case the opaque pointer is not used, and a non-NULL value is returned.
Second, the implementation of the `printf` function:

```
int printf ( const char *fmt, ... )
{
    va_list arg;
    int done;

    va_start ( arg, fmt );
    done = format( outfunc, NULL, fmt, arg );
    va_end ( arg );

    return done;
}
```

Because the opaque pointer is not used, and the consumer function ignores it,
a NULL is passed as the second argument to `format`.

The second example illustrates how the opaque pointer is used to implement the
standard C library function `sprintf`.  In this example the consumer function
returns the address of the next location to receive any following characters:

```
void * bufwrite( void * memptr, const char * buf, size_t n )
{
    return ( memcpy( memptr, buf, n ) + n );
}
```

The implementation of `sprintf` is shown below, with an additional step to
append a null character to the end of the string written into `buf`:

```
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
```

The final example illustrates the use of the opaque pointer to support an LCD
display with (x,y) positioning.  In this example a `struct` data type describes
where the consumer function is to place its output:

```
struct coord {
	short x, y;
};
```

This example assumes the LCD is 80 characters wide, and follows the usual convention of
top-left being at (0,0).

The consumer function uses the `coord` to set the position for each character
sent to the LCD calling a driver function `lcd_putc`:

```
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
```

The implementation of the `lcd_printf` function itself:

```
int lcd_printf( struct coord loc, const char *fmt, ... )
{
    va_list arg;
    int done;

    va_start ( arg, fmt );
    done = format( lcd_putat, &loc, fmt, arg );
    va_end ( arg );

    return done;
}
```

And an example call to this function might look like this:

```
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
```

