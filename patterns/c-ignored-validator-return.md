---
id: c-ignored-validator-return
languages: [c, cpp, rust]
cwe: 252
severity-typical: medium
seed-cves: []
---

# Ignored return of a validating function

The pattern: a function that *both* writes to an out-parameter *and* returns a status code is called, and the caller uses the out-parameter without checking the return. The U-Boot capsule finding from the reference session is the textbook case (`get_dp_device` ignoring `efi_deserialize_load_option`).

## Detection

Hard to grep directly. Two passes work:

1. List functions that take an out-pointer and return a status:
   ```
   grep -rnE '^\w+\s+\w+\s*\([^)]*\*\s*\w+[^)]*\)\s*\{' <subsys> | head
   ```
2. For each, grep callers and check whether the return is captured:
   ```
   grep -rnE '\b<func_name>\s*\(' <subsys> | grep -v '='
   ```
   Calls without `=` are candidates.

Better: use clang's `[[nodiscard]]` warnings if the project builds with them, or compile with `-Wunused-result` and look at the diagnostics.

## When the pattern is buggy
- The validator function may return without fully initializing the out-parameter on the error path
- The validator function writes the out-parameter *before* its validation completes (so failure leaves stale/partial data referenced)
- The caller dereferences the out-parameter regardless of the return code

## Common false positives
- The out-parameter is the same memory as the caller's input and is harmless on the error path
- The caller checks a sentinel inside the out-parameter that the validator sets reliably on failure

## Cross-check
Look at *other* callers of the same validator. If every other caller checks the return and one doesn't, that's strong signal — the API contract requires the check, and the outlier is the bug. This is exactly how the U-Boot capsule finding was confirmed.

## Verify (Phase 4)
Logic-bug confirmation: write a worked example showing the validator returns failure and the caller proceeds anyway. If the downstream usage triggers a memory-safety bug, ASan will catch it; otherwise document the worked example.

## Fix shape
```c
ret = validate(&out, ...);
if (ret != OK) {
    cleanup();
    return ret;
}
```
