# format

> A lightweight, low-overhead printf-style formatting library for embedded systems.

[![License: BSD-3-Clause](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](LICENSE.txt)
![Standard: C99](https://img.shields.io/badge/standard-C99-brightgreen.svg)
![Target: Embedded](https://img.shields.io/badge/target-embedded-orange.svg)

A platform-independent implementation of printf-style formatting designed for
constrained environments where the standard C library is too heavy or unavailable.
Three profiles are provided to suit different targets:

| Profile | Description | Typical code size |
|:--------|:------------|:------------------|
| **format** | Full-featured, all conversions | ~4 kB (no FP) / ~8 kB (with FP) on ARM GCC 12.2 |
| **tinyformat** | Feature-reduced for 16-bit MCUs | ~1.3 kB on atmega8 with AVR-GCC |
| **microformat** | Minimal for the most constrained platforms | smallest possible footprint |

A set of standard printf-compatible wrapper functions (sprintf, snprintf, asprintf, etc.)
is provided in `lib/` for drop-in use — see [`lib/README.md`](lib/README.md).

---

## Releases

  * **1.3** (May 2026) — `tinyformat` and `microformat` profiles; `ll` qualifier; `a`/`A` hex float specifiers; six new `lib/` wrappers (`scprintf`, `asprintf`, `asnprintf` and `v`-variants); full lib test harness.
  * **1.2** (Mar 2015) — Floating point (`e`/`E`/`f`/`F`/`g`/`G`), fixed-point (`k`), engineering and SI notation, arbitrary numeric bases, denormalized float support, rounding bug fixes.
  * **1.1** (Dec 2010) — ROM string support, length modifiers (`hh`, `j`, `z`, `t`), character repetition, grouping modifiers.
  * **1.0** (Jun 2010) — Initial release.

---

## Features

  * Small code size (4 kB without floating point, 8 kB with, compiled for ARM with GCC 12.2)
  * Low system overheads — no large internal buffers
  * Fully re-entrant — no global state
  * Supports most of ANSI C99 printf() format specifications (see exceptions below)
  * Output via a caller-supplied consumer function — works with any device (UART, LCD, buffer, …)

### ANSI Exceptions

The following C99 printf() features are not currently supported:

  * Length modifier `L` (long double)

### Extensions

**format** adds several useful features beyond standard printf:

  * `b` — binary conversion for unsigned values (base-2)
  * `!` flag — with `#` on `b`/`x`/`X`: always emit the prefix even for zero; with `e`/`E`: engineering notation (exponent forced to multiple of 3); with `f`/`F`: SI prefix formatting (µ, m, k, M, …)
  * `^` flag — centre-justify output within the field width
  * `I` / `U` conversions with `:base` modifier — arbitrary numeric base (2–36) _(FULL profile only)_
  * `k` — fixed-point conversion specifier
  * `%"` continuation specifier — intersperse format strings and arguments
  * `c` precision — repetition count for the character
  * `C` — character taken directly from the format string (with optional repetition)
  * `#` with `s` and continuation — select ROM-based string pointers (AVR PROGMEM)
  * `[sym count]` grouping modifier — digit grouping for numeric output (e.g. `%[,3]d` → `1,234,567`)

For examples see `doc/Examples.md` and the test cases in `test/testharness.c`.

---

## Producing Output

**format** never writes to any device directly. Instead it calls a _consumer function_
supplied by the caller, passing it chunks of formatted text. A simple example sends
characters to a UART; a more complex one might track an (x, y) position on an LCD.

See `doc/ManPage.md` for the full API and `lib/README.md` for the ready-made
printf-compatible wrappers.
