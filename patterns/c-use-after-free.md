---
id: c-use-after-free
languages: [c, cpp]
cwe: 416
severity-typical: high
seed-cves: [CVE-2022-2588]
---

# Use-after-free and double-free

The pattern: memory is `free()`d (or `kfree`/`vfree`/`delete`d) and then read, written, or freed again. Two common shapes:

- **Use-after-free (CWE-416):** a pointer is freed on one path, then dereferenced later — often because an error/cleanup path frees an object the caller still holds, or because a freed pointer isn't NULLed and a subsequent code path reuses it.
- **Double-free (CWE-415):** the same allocation is freed twice — typically two cleanup paths (a local `goto err` plus a caller's free), or a freed-but-not-NULLed pointer reaching a second cleanup.

These are the highest-impact memory-safety class after OOB write: the freed chunk can be reallocated and attacker-controlled, turning the stale dereference into a write/read primitive.

## Detection
Hard to grep precisely — flow-sensitive. Use static analysis as the primary detector and grep to triage:
```
# the textbook double-free / UAF shape: free without a following NULL
grep -rnE '\b(free|kfree|vfree|g_free)\s*\(\s*(\w+)\s*\)' <subsys>
# ...then check whether the same pointer is used or freed again before reassignment

# error-path frees that may also be freed by the caller
grep -rnB2 'goto\s+(err|out|fail|cleanup)' <subsys> | grep -E 'free\('
```
The real work is the static analyzer: `clang --analyze` and `cppcheck --enable=warning` both model alloc/free lifetimes. Run them and read the `Use of memory after it is freed` / `Memory pointed to ... is freed twice` diagnostics — these have a lower false-positive rate than the OOB checkers.

## When the pattern is buggy
- A pointer is `free`d on an error/early-return path **and** the same pointer is freed or used by the caller (ownership is ambiguous — who frees?)
- A freed pointer is not set to `NULL`, and a later path (retry loop, second cleanup, reused struct field) touches it
- An object is freed while still referenced by a list/cache/global that isn't updated
- A `realloc` failure path frees the original and the caller's stale pointer is still used (classic `p = realloc(p, n)` antipattern)
- In C++: a raw pointer is `delete`d while another owner still holds it; or a container is mutated while an iterator/reference into it is live

## Common false positives
- The pointer is set to `NULL` immediately after `free` and every later use is `NULL`-guarded
- Clear single-ownership: the freeing function documents that it takes ownership, and no caller frees again
- The "second free" is on a mutually exclusive branch that can't run after the first
- `free(NULL)` is a no-op — a double-free path that only ever sees `NULL` the second time is safe

## Verify (Phase 4)
ASan is purpose-built for this — `heap-use-after-free` and `double-free` are first-class ASan diagnostics with allocation/free/use backtraces.

Port-and-isolate: extract the allocation, the free path(s), and the use/second-free path. Drive the object through the lifecycle that triggers the bug (e.g., force the error path that frees, then invoke the caller path that uses or frees again). Build with `-fsanitize=address` and run; ASan prints the three relevant stacks (allocated here, freed here, used/freed-again here) — paste those into the report, they are the most credible evidence this class produces.

For UAF that depends on reallocation landing attacker data in the freed chunk, **do not** build the grooming primitive (hard limit). Demonstrating the stale access under ASan is sufficient; the reallocation/exploitation step is out of scope.

## Fix shape
NULL after free, and make ownership unambiguous:
```c
free(obj->buf);
obj->buf = NULL;          /* prevents later use and double-free */
```
For error paths, adopt single-exit cleanup with a clear owner:
```c
err:
    free(tmp);
    tmp = NULL;
    return ret;           /* caller does NOT free tmp on error */
```
For `realloc`, never overwrite the only pointer until success:
```c
char *new = realloc(p, n);
if (!new) { free(p); return -ENOMEM; }
p = new;
```
