---
id: c-untrusted-length-walk
languages: [c, cpp]
cwe: 125
severity-typical: high
seed-cves: [CVE-2023-22745]
---

# Pointer walking by attacker-controlled length

The pattern: a loop advances a pointer by a length field read from untrusted input, with no validation that the length is non-zero or that the new pointer is within bounds. Result: infinite loops on zero-length, OOB reads on sub-header-size lengths, or run-off-end on inflated lengths.

UEFI device-path nodes, ELF program headers, ASN.1 SEQUENCEs, and TLV-style protocols are all classic instances.

## Detection
```
grep -rnE '(ptr|p|cur|hdr|node|opt)\s*[+]=\s*\w+(->|\.)length' <subsys>
grep -rnE 'while\s*\([^)]*length[^)]*\)' <subsys>
```

Plus inspection of any walker function that iterates over packed records.

## When the pattern is buggy
- The length is read from untrusted bytes (network packet, file, variable)
- No minimum-length check exists at the walker (e.g., length < sizeof header)
- No maximum-length check exists relative to the remaining buffer
- The walker is reachable from an entry point that doesn't pre-validate the structure

## How to verify upstream
The validator-exists pattern is common: a function like `is_dp_valid()` or `check_length()` may be defined and called on *some* paths but not *all*. Always check whether each untrusted entry point invokes the validator. This is the "gap audit" technique from the reference U-Boot work.

To list possible entry points without the validator:
```
grep -rn '<walker_func>' <subsys> | grep -v '<validator_func>'
```

Then for each call site, look one frame up to check whether the validator was called before reaching the walker.

## Common false positives
- The walker checks `if (length < HEADER_SIZE) break;` already — bounded
- The walker is preceded by `if (validate(buf, len) != OK) return;` in every caller
- The "length" field is constant or compile-time-bounded

## Verify (Phase 4)
Craft input where one record's length is 0. Run the walker. If it spins (catch with a step counter in the PoC) or reads past the buffer (ASan), the bug is real.

## Fix shape
Either add the validator to the missing path, or add a per-step check inside the walker:
```c
while (record < end) {
    if (record->length < sizeof(*record) || record->length > (end - record))
        return -EINVAL;
    record = (void *)record + record->length;
}
```
