---
id: c-signedness-comparison
languages: [c, cpp]
cwe: 195
severity-typical: high
seed-cves: []
---

# Signed/unsigned confusion in a length check

The pattern: an attacker-controlled length is held in a **signed** type (`int`, `short`, `ssize_t`) and bound-checked with a comparison like `if (len > sizeof(buf)) return;`. A negative `len` passes the check (a negative `int` is "less than" the buffer size), then gets passed to `memcpy`/`malloc`/a loop bound where the parameter is `size_t`. The implicit signed→unsigned conversion turns the negative value into a near-`SIZE_MAX` quantity, and the bounded-looking copy becomes enormous.

This is distinct from `c-length-subtraction-underflow`: there the value underflows during arithmetic; here a value that is *already* negative survives a check that only guards the upper bound. The two often coexist.

## Detection
```
# signed length declared, then upper-bound-only checks
grep -rnE '\b(int|short|ssize_t|long)\s+\w*len\b' <subsys>
grep -rnE 'if\s*\(\s*\w+\s*[<>]=?\s*sizeof' <subsys> | grep -vE 'unsigned|size_t|u32|u64|uint'

# signed value flowing into a size_t sink
grep -rnE '(memcpy|memmove|malloc|alloca|read|recv)\s*\([^;]*\b(int|short)\b'
```

Also compile with `-Wsign-compare -Wsign-conversion` and read the diagnostics — this class is exactly what those warnings flag.

## When the pattern is buggy
- The length variable is a signed type and is read from untrusted input (a packet/file field, a `read()`/`recv()` return value, a `strtol` result)
- The validation checks only the **upper** bound (`len > MAX`) and not `len < 0`
- The value then reaches a `size_t` parameter (`memcpy`, allocation size, loop bound) where it converts to a huge unsigned number
- Bonus footgun: a narrow signed field (`char`/`short`) sign-extends when widened (e.g., `0xFF` byte read into `int` becomes `-1`)

## Common false positives
- The variable is unsigned end-to-end (`size_t`/`u32`) — no negative value is representable, so this class can't apply (a *different* class might)
- A `len < 0` (or `len <= 0`) check exists before the upper-bound check
- The value comes from `sizeof`/a compile-time constant and can't be negative
- The signed value is only ever compared, never used as a size or index

## Verify (Phase 4)
Port-and-isolate: extract the function with its real type declarations (keep the signedness identical — this bug *only* reproduces if the types match upstream). Drive the length to a negative value (e.g., `-1`, or a byte field of `0xFF` that sign-extends). The `< MAX` guard passes; the subsequent `memcpy`/`malloc` receives `~SIZE_MAX`; ASan reports the overflow. Confirm with both `-fsanitize=address` and `-fsanitize=undefined` (UBSan flags the implicit conversion).

## Fix shape
Use an unsigned type for lengths, or add the lower-bound check explicitly:
```c
if (len < 0 || (size_t)len > sizeof(buf))
    return -EINVAL;
memcpy(buf, src, len);
```
Prefer making the length `size_t` at the point it is read, so the type system rules out negatives entirely.
