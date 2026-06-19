---
id: c-alloc-arith-wraparound
languages: [c, cpp]
cwe: 190
severity-typical: high
seed-cves: [CVE-2024-57255, CVE-2024-57256, CVE-2024-57257]
---

# Allocation arithmetic wraparound

The pattern: `malloc(untrusted_value + constant)` or `malloc(untrusted_a * untrusted_b)` where the arithmetic can wrap to a small value, causing a tiny allocation followed by a large copy.

## Detection
```
grep -rnE '(malloc|zalloc|calloc|realloc|kmalloc|kzalloc)\s*\([^)]*[\+\*][^)]*\)' <subsys>
```

Filter the results: ignore lines where the arithmetic uses `sizeof(...)` or other compile-time constants.

## When the pattern is buggy
- One operand is read from untrusted input (a length field in a packet/file/inode)
- The operand type allows the arithmetic to wrap to a small value (most commonly: `u32` + small constant where the `u32` came from `le32_to_cpu` or `ntohl`)
- The allocated buffer is then filled from the same source with the *original* length, not the wrapped one

## Common false positives
- The operand is masked or bound-checked just before the alloc (e.g., `if (size > MAX) return -EINVAL;`)
- The operand is a small fixed-width field (e.g., a `u8` or 24-bit field whose `+1` cannot reach a large allocation)
- The allocation is used only as a hint and overflow is checked downstream

## Verify (Phase 4)
A minimal PoC: craft input where the length field is `0xFFFFFFFF`; observe `malloc(0)` and a subsequent write past the (zero-byte or tiny) allocation. ASan catches it.

## Fix shape
```c
if (size > MAX_REASONABLE_SIZE)
    return -EINVAL;
ptr = malloc(size + 1);
```
Or use `kmalloc_array` / similar overflow-safe helpers when available.
