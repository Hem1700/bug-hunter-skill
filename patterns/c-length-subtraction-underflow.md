---
id: c-length-subtraction-underflow
languages: [c, cpp]
cwe: 191
severity-typical: high
seed-cves: []
---

# Length subtraction underflow

The pattern: `remaining = len - header_size;` or `payload_len = total - offset;` where `header_size` or `offset` may exceed `len`/`total`. Underflow produces a huge value that's then used as a copy size or loop bound.

## Detection
```
grep -rnE '(len|size|remain|left|payload)\s*=\s*\w+\s*-\s*(sizeof|\w+_HDR|\w+_HEADER)' <subsys>
grep -rnE '\w+\s*-=\s*(sizeof|\w+_HDR|\w+_HEADER|\w+_LEN)' <subsys>
```

## When the pattern is buggy
- The minuend can be less than the subtrahend on some input path (typically: a truncated packet, a malformed header that claims a size larger than the actual buffer)
- The result is unsigned, so underflow yields a near-`SIZE_MAX` value
- The result is used as a length in `memcpy`/`memmove`/loop bound/allocation size

## Common false positives
- The subtrahend is bounded immediately before the subtraction (`if (offset > len) return;`)
- The result is signed and the code checks `< 0`
- The result is used only for a bounded comparison and not as a size

## Verify (Phase 4)
Feed a packet where the declared header size exceeds the actual packet length. ASan catches the resulting huge-`memcpy` as a heap or stack overflow.

## Fix shape
```c
if (offset > len)
    return -EINVAL;
remaining = len - offset;
```
Or use a signed type for the intermediate calculation, then check `< 0`.
