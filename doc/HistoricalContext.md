# Introduction #

The **format** library is not the first (and likely not the last!) lightweight printf library for embedded systems.  Here is a list of similar projects that I have found on the net that do something similar.  Note that this is not meant to be the definitive collection, merely a list of some of what is out there.

Interestingly I only found out about these projects _after_ I had written the first version of **format**.  In fact, the original version was written way back around 1998 when I was writing the standard C library for an 80186 compiler I was working on at the time, and took inspiration from Plauger's _Standard C Library_.

## Dave Hylands' StrPrintf ##

http://www.davehylands.com/Robotics/MRM/Str/

Dates from 2003.

## Georges Menies ##

Georges built a nice little printf replacement for embedded systems.  It supports a limited subset of the ANSI printf specification, but usually enough for most typical purposes.

http://www.menie.org/georges/embedded/#printf

Floating point support has been added and maintained by Daniel D Miller (last updated August 2010).

http://home.comcast.net/~derelict/snippets.html

## Philip J. Erdelsky ##

http://www.efgh.com/software/gprintf.htm

This library from 1996 is close in behaviour to **format** - it calls a consumer function to handle any output.

## Rud Merriam's rprintf ##

This version of printf was originally written for Embedded Systems Programming (1991).

An original copy is in the Files section of the LPC2000 Yahoo group, or you can find it searching for the string "Public Domain version of printf" using your favourite search engine (Google shows 7 hits).

This same source has been extended with the addition of basic floating point support:

http://www.intellimetrix.us/downloads.htm - see **rprintf.zip**

## Really Old!!! ##

Here is a really old version of printf that dates back to the 80s.

http://opensource.apple.com/source/CPANInternal/CPANInternal-62/DBD-SQLite/printf.c?txt

Some interesting observations:
  1. It implements something similar to **format**'s `^` string centre flag (they use `=`).
  1. It implements the `%b` conversion specifier.
  1. Adds a couple of unique features:
    * `%c` treats the precision field as a repetition count
    * `%'` behaves the same as `%c` but takes the character from the next format string character - on its own this seems silly, but it works in combination with the `%c` repetition feature.  For example, to print 78 minus sign characters you would express this as `"%.78'-"`, which is significantly more compact than 78 minus sign characters.
    * Both of these features have now been added to **format**, using `%C` instead of `%'` to indicate the similar operation to `%c`.