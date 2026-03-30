# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a lightweight, low-overhead printf-style formatting library designed for embedded systems. It provides three variants optimized for different constraints:

- **format**: Full-featured version with extensive printf support (~4kB without FP, ~8kB with FP on ARM GCC 12.2)
- **tinyformat**: Feature-reduced version for very small systems (~1.3kB on atmega8 with AVR-GCC)
- **microformat**: Extremely minimal version for the most constrained platforms

## Build and Test Commands

### Building and Running Tests
```bash
cd test
make              # Build all test harnesses and run them
make clean        # Remove all build artifacts
```

### Individual Test Targets
```bash
make testharness      # Build and run full format tests
make tinytestharness  # Build tinyformat tests
make microtestharness # Build microformat tests
make perftest         # Build floating point performance tests
make libtest          # Build and test printf-compatible library wrappers
```

### Running Individual Tests
```bash
cd test
make testharness && ./testharness
make tinytestharness && ./tinytestharness
make microtestharness && ./microtestharness
```

### Compiler Flags
The test Makefile uses strict compilation flags:
- C99 standard (`-std=c99`)
- Warnings: `-Wall -pedantic -Wunused -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wmaybe-uninitialized -Wtype-limits`

## Code Architecture

### Core Design Pattern: Consumer Functions

The library uses a **consumer function pattern** instead of directly outputting to stdout or a buffer. This abstraction allows the library to work with any output device (UART, LCD, buffer, etc.) without modification.

The consumer function signature:
```c
void * cons(void *arg, const char *s, size_t n)
```

- Takes an opaque pointer `arg` (e.g., buffer pointer, device context)
- Receives `n` characters from string `s` to output
- Returns updated opaque pointer for next call (or NULL on error)

### Source Structure

#### `src/` - Core Formatting Implementations

- **format.c** / **format.h**: Full-featured formatting engine with all printf conversions
  - Main entry point: `format(cons, arg, fmt, va_list)`
  - Implements standard printf specifiers plus extensions (binary, arbitrary bases, grouping, etc.)
  - Returns character count or `EXBADFORMAT` (-1) on error

- **format_fp.c**: Floating point conversion support
  - Separated for code size control (can be excluded at build time)
  - Implements `e`, `E`, `f`, `F`, `g`, `G`, `a`, `A` conversions
  - Supports engineering notation and SI unit formatting with `!` flag
  - Handles denormalized floats, NaN, infinity
  - Platform-aware: detects 32-bit vs 64-bit double precision

- **format_config.h**: Configuration flags for feature control
  - `CONFIG_WITH_FP_SUPPORT`: Enable/disable floating point (removes ~4kB)
  - `CONFIG_WITH_LONG_LONG_SUPPORT`: Enable `ll` qualifier support
  - `CONFIG_WITH_GROUPING_SUPPORT`: Enable grouping modifiers
  - `CONFIG_HAVE_ALT_PTR`: Support for ROM-based strings (AVR PROGMEM)
  - Platform detection (e.g., AVR with `__GNUC__ && __AVR__`)

- **tinyformat.c**: Standalone reduced-feature implementation
  - Self-contained version with smaller feature set
  - Same consumer function pattern as format.c
  - Optimized for minimal code size on 16-bit CPUs

- **microformat.c** / **microformat.h**: Minimal implementation
  - Hardcoded to use external `format_putchar()` function
  - No consumer function abstraction for maximum size reduction
  - User must provide: `int format_putchar(int c)`

#### `lib/` - Printf-Compatible Wrappers

These files wrap the core `format()` engine to provide standard printf-family functions:

- **printf.c** / **printf.h**: `printf()` and `vprintf()` using `putchar()`
- **sprintf.c**: `sprintf()` and `vsprintf()` for unbounded buffer output
- **snprintf.c**: `snprintf()` and `vsnprintf()` with size limits

Note from lib/README.txt: These have no dedicated test harness and are provided as-is.

#### `test/` - Test Infrastructure

- **testharness.c**: Comprehensive test suite for format.c
  - Tests all conversion specifiers, flags, modifiers
  - Uses `TEST(expected, return_value, format, args...)` macro
  - Validates both output string and return value

- **tinytestharness.c**: Test suite for tinyformat
- **microtestharness.c**: Test suite for microformat
- **performance.c**: Floating point performance benchmarks
- **libtest.c**: Tests for the printf-compatible library wrappers

#### `example/` - Usage Examples

- **lcd.c**: Example consumer function for LCD output

#### `doc/` - Documentation

- **ManPage.md**: Full format specification with all conversion specifiers
- **TinyManPage.md**: Documentation for tinyformat variant
- **MicroManPage.md**: Documentation for microformat variant
- **Examples.md**: Usage examples
- **HistoricalContext.md**: Design decisions and history
- **TestedPlatforms.md**: List of tested platforms

### Key Design Principles

1. **No Direct Output**: Library never outputs directly; always through consumer function
2. **Re-entrant**: No global state; safe for multi-threaded/multi-context use
3. **Configurable**: Feature flags allow trading functionality for code size
4. **Platform-Aware**: Detects and adapts to platform capabilities (AVR PROGMEM, float precision)
5. **Standards-Based**: Implements most of ANSI C99 printf with useful extensions

### Feature Extensions Beyond Standard Printf

- `b` conversion: Binary output (base-2)
- `!` flag: Modify `#` behavior for bases, engineering/SI notation for floats
- `^` flag: Center-justify output
- `I`/`U` conversions: Arbitrary base conversions (2-36) with `:base` modifier
- `k` conversion: Fixed-point numbers
- `C` conversion: Character from format string itself
- Grouping modifiers: `[symbol-count]` for digit grouping (e.g., `%[,3]d` for 1,234,567)
- Continuation specifier: `%"` to intersperse format and arguments
- `#` with `s`: Select ROM-based string pointers

### Important Constants and Limits

From format.c:
- `MAXWIDTH`: 500 (maximum field width)
- `MAXPREC`: 500 (maximum precision)
- `MAXBASE`: 36 (maximum numeric base for arbitrary base conversions)
- `BUFLEN`: 130 (internal buffer, sized for 64-bit binary with grouping and prefix)

### Floating Point Implementation Details

From format_fp.c:
- Automatically detects 32-bit vs 64-bit double support via `DBL_DIG`
- 64-bit: Uses `unsigned long long` mantissa, 16 significant figures
- 32-bit: Uses `unsigned long` mantissa, 9 significant figures
- Binary representation unpacking with bit field macros
- Engineering/SI notation limited to ±10^24 per BIPM standards

## Common Development Patterns

### Adding New Format Specifiers

1. Add flag/state definitions in format.c macros section
2. Implement parsing in main format() function
3. Add conversion logic in switch statement
4. Update format.h if API changes needed
5. Add comprehensive tests in testharness.c using TEST() macro

### Porting to New Platform

1. Check format_config.h for platform detection
2. Add platform-specific macros if needed (ROM access, etc.)
3. Test with existing test harnesses
4. Add platform to doc/TestedPlatforms.md

### Optimizing Code Size

To reduce code size, edit format_config.h:
- Undefine `CONFIG_WITH_FP_SUPPORT` (saves ~4kB)
- Undefine `CONFIG_WITH_LONG_LONG_SUPPORT` (saves library dependencies)
- Undefine `CONFIG_WITH_GROUPING_SUPPORT` (saves grouping logic)
- Or use tinyformat.c or microformat.c variants

### Testing Changes

After modifying any src/ files:
```bash
cd test
make clean
make  # Builds and automatically runs all test harnesses
```

All test harnesses must pass before committing changes.
