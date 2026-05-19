# Printf-compatibility library

This directory contains printf-compatible wrapper functions built on top of the
`format()` engine.  Each source file is self-contained and can be added directly
to a project; there is no separate build step required.

All files depend on `format.h` and must be linked with `format.c` (or one of the
profile-specific variants).

---

## Functions

Functions are grouped by their output destination.  Each group also provides a
`v`-prefixed variant that accepts a `va_list` instead of variadic arguments,
following the standard C convention.

### Output via `putchar()` — `printf.c`

```c
int printf( const char *fmt, ... );
int vprintf( const char *fmt, va_list ap );
```

Produce formatted output by calling the system-supplied `putchar()` function for
each character.  The system must provide an `int putchar(int)` implementation.

Returns the number of characters written, or -1 on error.

---

### Write to an unbounded buffer — `sprintf.c`

```c
int sprintf( char *buf, const char *fmt, ... );
int vsprintf( char *buf, const char *fmt, va_list ap );
```

Write formatted output into `buf`, appending a null terminator.  The caller is
responsible for ensuring `buf` is large enough to hold the result; no bounds
checking is performed.  Use `snprintf` if the maximum output length is not known
in advance.

Returns the number of characters written (excluding the null terminator), or -1
on error.

---

### Write to a bounded buffer — `snprintf.c`

```c
int snprintf( char *buf, size_t n, const char *fmt, ... );
int vsnprintf( char *buf, size_t n, const char *fmt, va_list ap );
```

Write formatted output into `buf`, writing at most `n` bytes including the null
terminator.  Characters beyond the limit are silently discarded; the buffer is
always null-terminated provided `n > 0`.

Returns the number of characters that would have been written if `buf` were large
enough (excluding the null terminator), or -1 on error.  A return value greater
than or equal to `n` indicates that the output was truncated.

---

### Count characters without producing output — `scprintf.c`

```c
int scprintf( const char *fmt, ... );
int vscprintf( const char *fmt, va_list ap );
```

Determine the number of characters that would be produced by the format string,
without writing any output.  Useful for sizing a buffer before calling `sprintf`
or `malloc`.

Returns the number of characters that would be written (excluding any null
terminator), or -1 on error.

**No external dependencies beyond `format()`.**

---

### Allocate a buffer and write into it — `asprintf.c`

```c
int asprintf( char **strp, const char *fmt, ... );
int vasprintf( char **strp, const char *fmt, va_list ap );
```

Format output into a newly allocated buffer.  On success `*strp` is set to point
to a null-terminated string; the caller is responsible for freeing it with
`free()`.  On failure `*strp` is not modified.

Returns the number of characters written (excluding the null terminator), or -1
on error (including allocation failure).

**Requires `malloc()`.** These functions are intended for hosted environments.
Do not use them on targets without a working heap.

---

### Write into a caller-supplied buffer, allocating only if necessary — `asnprintf.c`

```c
char *asnprintf( char *resultbuf, size_t *lengthp, const char *fmt, ... );
char *vasnprintf( char *resultbuf, size_t *lengthp, const char *fmt, va_list ap );
```

A hybrid of `snprintf` and `asprintf`.  If the formatted result (including the
null terminator) fits within `*lengthp` bytes of `resultbuf`, `resultbuf` is used
directly and no allocation occurs.  Otherwise a new buffer is allocated with
`malloc()`.

On entry `*lengthp` is the capacity of `resultbuf`.  On success `*lengthp` is
updated to the number of characters written (excluding the null terminator), and
the function returns a pointer to the result — either `resultbuf` or the newly
allocated buffer.  If a new buffer was allocated the caller is responsible for
freeing it with `free()`; a simple way to check is to test whether the return
value differs from `resultbuf`.

Returns a pointer to the formatted result, or `NULL` on error (including
allocation failure).

**Requires `malloc()` when the result does not fit in `resultbuf`.**  Passing
`NULL` as `resultbuf` (with `*lengthp` set to 0) always allocates, and is
equivalent to `asprintf`.

---

## Choosing the right function

| Situation | Recommended function |
|:---|:---|
| Output to terminal / UART via `putchar` | `printf` / `vprintf` |
| Fixed-size stack buffer, truncation acceptable | `snprintf` / `vsnprintf` |
| Fixed-size stack buffer, size known correct | `sprintf` / `vsprintf` |
| Need to know the length before allocating | `scprintf` / `vscprintf` |
| Heap available, buffer size unknown | `asprintf` / `vasprintf` |
| Prefer stack but heap available as fallback | `asnprintf` / `vasnprintf` |

---

## Notes

- Functions that require `malloc` are in their own source files so that projects
  without a heap can still use the rest of the library without pulling in any
  allocator dependency.

- There is currently no dedicated test harness for these wrapper functions.
  The underlying `format()` engine is thoroughly tested by the harnesses in
  `test/`.
