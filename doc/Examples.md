# Introduction #

The ReadMe page describes in detail the functionality that the `format` library provides.  This page provides illustrations of the additional features provided by `format` that extend the base C99-compatible functionality.


# Binary Conversion #

The `b` conversion specification formats the unsigned integer argument into a binary base-2 number.  For example

```
n = format( cons, arg, "GPIO = %8.8b", 37 );
```

would send to the `cons` function the characters representing the output

```
GPIO = 00100101
```



# Arbitrary Integer Base Conversion #

The `i`, `I`, `u` and `U` conversions can take any number base specified by a base modifier, in exactly the same way as precision and width.

For example, formatting a number for a our three-fingered Martian friends can be done quite easily:

```
format( cons, arg, "%:3i bottles of Martian beer on the wall", 99 );
```

would produce
```
10200 bottles of Martian beer on the wall
```

For bases greater than 10 the letters `a`-`z` are used for the `i` and `u` conversions, and `A`-`Z` are used for the `I` and `U` conversions.  As a consequence of this the maximum base is 36.


# Continuation Conversion Specifier #

A recent addition to the `format` library is the continuation conversion specification.  Notionally it is written `%"`, in practice you place a `%` as the last character in a format string and `format` will continue parsing the format specification from the string pointed to by the next argument.

What does this mean in practice?  Here is an illustration of the problem this solves: to print out the contents of three elements of a structure:

```
struct {
   unsigned int acc;   /* Accumulator */
   unsigned int x;     /* X-register  */
   unsigned int y;     /* Y-register  */

   /* other stuff ... */

} cpu;

/* ... */

format( cons, arg, "A = %#!8.8X, X = %#!8.8X, Y = %#!8.8X", cpu.acc, cpu.x, cpu.y );
```

where `cons` and `arg` are placeholders for the corresponding arguments to `format`.

I think it looks ugly - there is no easy visual connection between the conversion specifiers and their corresponding arguments.  The situation only gets worse as fields are added, changed or removed - it is very easy to lose track of which conversion specifier relates to which argument!

With continuation this takes on a more natural look, allowing format specifications and arguments to be interspersed, much like in languages like Java and C++:

```
format( cons, arg, "A = %#!8.8X, %", cpu.acc, 
                   "X = %#!8.8X, %", cpu.x, 
                   "Y = %#!8.8X",    cpu.y );
```

# Caret Flag #

The new `^` flag centres the result of an `%s` conversion within the field if the field is wider than the argument string.

For example, printing `hello` in a delimited field 8 characters wide

```
format( cons, arg, "[%^8s]", "hello");
```

produces the output

```
[ hello  ]
```

How is this done?  During formating the `format` function calculates how many space padding characters are required to satisfy the given field width.  The default is to put the space padding on the left, resulting in right-justified text.  The `-` flag puts the space padding on the right, giving left-justified text.  The new `^` flag splits any space padding equally between left and right, centering the text.

Note that if the number of space padding characters is odd (such as in the above example) the odd space padding character is placed to the right of the text.  In the above example you can see that of the three space padding characters there is one to the left of the `hello`, and two to the right.  If the '-' flag is added then the odd space padding character is placed to the left of the text.

# Single Character Repetition #

An extension to the `%c` conversion is to use the precision field to specify how many times the character is written.  A further addition is the `%C` conversion with takes its character as the next character in the format string.

This repetition feature allows for long repetitious strings to be efficiently stored.  For example, to write out a screen-wide horizontal line without this feature would require a string of at least 80 characters (assuming an 80-character wide terminal).

```
format( cons, arg, "---...etc...---" );
```

With this new feature the long string is shortened to something like 6 characters:

```
format( cons, arg, "%.80C-" );
```

Also, if your application is able to query the output device for the width then this can be specified directly.  For example:

```
format( cons, arg, "%.*C-", terminal.width );
```

# Alternate String Pointers #

The `#` flag can be used on certain target platforms to indicate that the corresponding pointer argument is of an alternate form, typically a pointer to a string in FLASH ROM.  An example would be a string in the AVR microcontroller's code memory.  This provides for far more compact programs as string constants, including format strings, can be placed in the FLASH ROM rather than in the smaller RAM.  The following example illustrates both format string and string argument in FLASH ROM:

```
char fmtstring[] PROGMEM = "hello %#s";
char hellostr[] PROGMEM = "world";

format( cons, arg, "%#", fmtstring, hellostr );
```

Note: this example uses the AVR extensions for GCC.  Other compilers may use slightly different ways of expressing this information, but the basic principle is the same.

On target platforms that do not support nor need this alternate pointer form the `#` flag is ignored by the continuation and `s` conversions.

# Grouping #

The new grouping feature offers a convenient way to format numeric fields in a natural way, somewhat similar to the locale functionality offered in larger C libraries.

One point to note is that the ordering of the grouping specifiers is right-to-left, in exactly the same as applied to the number itself.

A good first example is printing out disk sizes:

```
format( cons, arg, "Disk size: %[,3]d bytes", disk_size );
```

If disk\_size is 131,238 bytes, then the result will be

```
Disk size: 131,238 bytes
```

Because of the self-repeating feature, the same format specification works for larger disks too: if disk\_size is 12,345,678 then the result will be:

```
Disk size: 12,345,678 bytes
```

Currency provides another good use:

```
format( cons, arg, "You have $%[,3.2]d left to spend", money_cents );
```

If your bank account contains $1,234.56, or 123,456 cents, then the above will produce:

```
You have $1,234.56 left to spend
```

The first (rightmost) grouping handles the cents, then the second (leftmost) grouping splits the dollars into groups of thousands, millions (if you're lucky!), and so on.

Formatting of binary or hexadecimal fields can be easily achieved:

```
format( cons, arg, "Register = %[-_2_2]b", 0xF3 );
```

will produce

```
Register = 1111_00_11
```

In this case the number is split into two groups of two digits separated by underscore, and then all remaining digits without any further grouping.

Finally, group widths can also be given at runtime using the asterisk:

```
format( cons, arg, "%[,*]d", number, group_width );
```

will format `number` split into groups of `group_width` digits.  If `group_width` is zero (no group) or negative (terminate grouping) then no grouping will be applied in this case.  Also note that unlike the width and precision cases the asterisk arguments _follow_ the numeric argument.

## Note ##

The current version of the grouping feature does **not** apply to any zero padding nor to floating point conversions.


# Floating Point Support #

The **format** library provides the facility for formatting floating point values.  Floating point conversions have a long and twisty history, so **format** takes a pragmatic approach in considering only the first sixteen decimal digits for 64-bit doubles (9 digits for 32-bit doubles) as significant.  While it is possible to specify very large values or much greater precisions than this, the result will comprise the maximum number of significant digits followed by zeros.

An example of this would be something like:

```
"%f", 1.0e20/3.0
```
which would produce
```
33333333333333480000.000000
```
and not the expected all-3s.

While this is clearly not _exact_ enough for scientific users it should be sufficient for smaller embedded systems for which **format** is designed for.

A compile-time facility supports 32-bit doubles as used on smaller embedded systems, typically 8-bit microcontrollers.  On these platforms **format** considers only the first nine digits as significant.

## Engineering / SI Multipliers ##

The `!` flag when applied to floating point conversions `e`, `E`, `f` and `F` formats the result of the conversion using engineering or SI prefixes.

The `e`/`E` conversion formats the result with an exponent forced to a multiple of three, with one to three digits in front of the decimal point.  For example

```
"%!.2e", 12345.0
```
would produce
```
12.35e+03
```

The `f`/`F` conversion formats the result using the internationally-recognised SI prefixes ([NIST](http://physics.nist.gov/cuu/Units/prefixes.html)), which cover the range 10<sup>-24</sup> to 10<sup>24</sup>.  For example

```
"%!.3fV", 0.00123
```
would produce
```
1.230 mV
```

If the result of conversion is outside of the range of the published SI prefixes the output will be correct, but will not look right.  For example, suppose you were a large first-world country with a huge national deficit of $4.567x10<sup>27</sup> and you tried to print it out you might see something like

```
$4567Y
```
since Y (yotta) is the largest prefix, representing 10<sup>24</sup>.  If SI ever extend the range then `format` can be readily extended to support the new prefixes.
