---
id: c-unbounded-copy
languages: [c, cpp]
cwe: 120
severity-typical: high
seed-cves: [CVE-2022-30790, CVE-2022-30552]
---

# Unbounded memcpy / strcpy with attacker-controlled length

The pattern: `memcpy(dst, src, len)` where `len` comes from untrusted input and isn't validated against `sizeof(dst)`. Stack-allocated `dst` makes this a stack buffer overflow.

## Detection
```
grep -rnE '(memcpy|memmove|strcpy|strncpy|strcat|sprintf)\s*\(' <subsys> \
  | grep -v 'sizeof\|ETH_ALEN'
```

Then focus on calls where the third argument (or the implicit length in `strcpy`) is a variable rather than a constant.

## When the pattern is buggy
- `len` (or the source length for `strcpy`) is read from a packet, file, or other untrusted source
- There is no upstream check that `len <= sizeof(dst)` or equivalent
- `dst` is either a fixed-size buffer (stack array, global, struct field) or a heap allocation whose size doesn't match `len`

## Common false positives
- An upstream wrapper validates `len` against the destination's size (look one or two stack frames up)
- A "truncate" helper caps `len` first (search the calling function for `min`, `truncate_sz`, or similar)
- The destination is sized explicitly to match the source on every code path

## Verify (Phase 4)
Port-and-isolate: extract the function, feed an oversized `len`, ASan reports stack/heap-buffer-overflow.

## Fix shape
```c
if (len > sizeof(dst))
    return -EINVAL;
memcpy(dst, src, len);
```
Or use a size-aware helper: `strlcpy`, `strscpy`, `memcpy_s`. Avoid `strcpy`/`strcat` entirely on untrusted input.
