A lightweight low-overhead library for processing printf-style format descriptions and arguments designed for the constrained environments of embedded systems.

# News #
  * 10-Mar-2015: Release 1.2 now available.
  * 05-Feb-2015: Add support for fixed-point numbers with the `k` conversion specifier.
  * 06-Nov-2014: Add support for denormalized floating point numbers, for people who like _really small_ numbers.
  * 17-Mar-2014: **Rounding bug fixed** ([Issue 7](https://code.google.com/p/format/issues/detail?id=7)).  Remaining work to get %g and %G conversion specifiers working properly.
  * 20-Jan-2014: Bug in floating-point rounding code caused incorrect output for some values.
  * 19-Aug-2013: Enhanced the `e`, `E`, `f` and `F` floating point conversions with the use of the `!` flag to implement formatting to Engineering (x10<sup>3</sup> etc) or SI (mega, micro, etc) units.
  * 08-Aug-2013: Sincere apologies to those who have raised issues over the last couple of years.  For some reason google did not notify me and I have only just found them.  Hopefully now fixed.
  * 19-Mar-2013: Added support for `e`, `E`, `f`, `F`, `g` and `G` floating point conversions.
  * 19-mar-2013: After much consideration I have decided to change the way grouping is specified into a more natural form.  This unfortunately breaks existing code. _Sorry_
  * 16-Aug-2012: Added arbitrary numeric base conversions.
  * 22-May-2012: Centre flag '^' uses '-' flag to bias left in %s conversions.  It now also defaults to right bias in the same way that non-centred %s conversions are right-aligned by default.
  * 07-Feb-2011: Added grouping specifiers to numeric conversions.
  * 09-Dec-2010: Added support for ROM-located strings.
  * 08-Dec-2010: Release 1.1 now available.
  * 23-Nov-2010: Extended %c and added %C.
  * 12-Nov-2010: Initial port to 8-bit AVR shows good size of about 2.5-2.8kB
  * 11-Nov-2010: Added TestedPlatforms page
  * 28-Sept-2010: Added support for length modifiers `hh`, `j`, `z`, `t`
  * 23-June-2010: Release 1.0 now available.

# Features #

  * Small code size (3kB code size without floating point formats, 6kB code with float support, compiled for ARM with GCC 4.4)
  * Low system overheads (no large buffers)
  * Fully re-entrant
  * Supports most of ANSI C99 printf() format specifications (see below for exceptions) with many useful features

## ANSI Exceptions ##

The following features from the ANSI C99 printf() format specifications are not supported in the current version of **format**

  * Hexadecimal floating point conversions (`a`,`A`)
  * Length modifiers `ll` (long long int), `L` (long double)

New features added include

  * `b` binary conversion for formatting unsigned values in base-2
  * `!` flag modifies the behaviour of the `#` flag in binary, octal and hexadecimal conversions to always add the prefix (the default is to drop the prefix for zero results)
  * `!` flag modifies the behaviour of the `e`, `E`, `f` and `F` floating point conversions to use engineering (for `e`/`E`) or SI (`f`/`F`) formatting
  * interspersing format specification and arguments using a new continuation specifier
  * `^` flag centre-justifies conversion results if the field is wide enough to require padding
  * `c` conversion treats precision as a repetition count.
  * `C` conversion is same as `c` but gets character from format string itself.
  * `#` flag with continuation and `s` conversions to select alternate ROM-based string pointers
  * `I` and `U` conversions, together with a numeric base modifier, for arbitrary numeric base conversions

# Producing Output #

**format** itself does not send any output characters to any device.  Instead, it calls a _consumer function_, supplied by the caller, to process any output.  A simple example would be a function to send the characters to a UART.  A more advanced use might be sending characters to an LCD.


---

(Logo courtesy of http://www.clker.com)