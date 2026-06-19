---
id: c-array-shift-count
languages: [c, cpp]
cwe: 193
severity-typical: high
seed-cves: []
---

# Off-by-one in array shift / compaction counts

The pattern: a `memmove` (or hand-rolled shift loop) is used to compact or insert into a fixed-size array, but the count argument is computed wrong — typically using the wrong variable or off by one. The U-Boot `tcp_hole` SACK finding from the reference session is the canonical example.

## Detection
```
grep -rnE 'memmove\s*\(&?\w+\[[^]]+\]\s*,\s*&?\w+\[[^]]+\]' <subsys>
```

Then read each hit and ask: when this `memmove` runs, can the source range extend past the end of the array? Can the destination + count?

## When the pattern is buggy
- The array has a fixed compile-time size (e.g., `hill[TCP_SACK_HILLS]`)
- The shift count is computed from variables that can equal the array size in some path
- The count is *not* re-derived from the actual remaining-survivor count after the operation that changes the array's effective length
- A guard like `if (cnt > i + N)` exists but uses a different variable than the `memmove` count argument — they happen to coincide in some cases and diverge in others

## How to confirm by reading
1. Identify the array bound `N`.
2. Identify the maximum value of every variable that goes into the `memmove`'s third argument and into the source/dest index arithmetic.
3. Construct an input that drives the variables to maxima.
4. Check: does `source_offset + count > N`? If yes → OOB read. Does `dest_offset + count > N`? If yes → OOB write.

## Common false positives
- The count is `sizeof(array) - i*sizeof(elem)` (correctly derived from remaining space) — safe
- The function has an early return for the boundary case (`if (i == N - 1) return;`)
- The array is dynamically sized and the bound used here matches the dynamic size

## Verify (Phase 4)
Port-and-isolate is ideal — the U-Boot `tcp_hole` PoC shape works. Drive the function into the corner case where the count argument exceeds the survivor count; ASan reports OOB.

## Fix shape
Use the real survivor count explicitly:
```c
int n_move = total - (i + cnt_move + 1);  /* survivors after the merged region */
if (n_move > 0)
    memmove(&arr[i + 1], &arr[i + cnt_move + 1], n_move * sizeof(*arr));
```
