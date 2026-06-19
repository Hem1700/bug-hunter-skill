---
id: go-slice-make-bounds
languages: [go]
cwe: 129
severity-typical: medium
seed-cves: [CVE-2022-41717]
---

# Go: attacker-controlled slice index, slice expression, or make() size

Go is memory-safe, so the failure mode here is not OOB corruption (the runtime bounds-checks) but **panic = denial of service** and **resource exhaustion = denial of service** — plus genuine OOB if the code uses `unsafe`/`cgo` to bypass bounds checks.

Three sub-shapes:

- **Index / slice-expression out of range (CWE-129):** `buf[n]`, `buf[a:b]`, or `buf[:n]` where `n`/`a`/`b` come from untrusted input without a bound check. A bad index panics (`index out of range`); a bad slice expression panics (`slice bounds out of range`).
- **`make` with attacker-controlled size (CWE-789 / CWE-190):** `make([]T, n)` where `n` is a length field from the request. A huge `n` allocates gigabytes (memory-exhaustion DoS); a negative `n` panics. Pre-allocating from an untrusted length field before reading the actual data is the classic shape (e.g., `make([]byte, hdr.Length)` before checking `Length` against what's actually on the wire).
- **`unsafe`/`cgo` bypass (CWE-125/787):** if the slice is built via `unsafe.Slice`, `reflect.SliceHeader`, or passed to C, the runtime bounds check is gone and the bug is real memory corruption — treat as the C classes.

## Detection
```
# attacker length straight into make
grep -rnE 'make\(\[\]\w+,\s*\w+(\.\w+)*\)' <subsys>     # then check the size's provenance
# slice expressions with a variable bound
grep -rnE '\w+\[[^]]*:[^]]*\]' <subsys> | grep -vE '\[:\]|\[0:\]'
# unsafe escape hatches — these turn DoS into corruption
grep -rnE 'unsafe\.(Slice|Pointer)|reflect\.SliceHeader|C\.' <subsys>
```
Tooling:
```
go vet ./...
staticcheck ./...          # SA6002 etc.
go test -fuzz=...          # fuzzing finds index/slice panics fast
```

## When the pattern is buggy
- The index / slice bound / `make` size derives from untrusted input (request field, file header, decoded length) with no validation against the actual data length or a sane cap
- For `make`: there is no upper cap, so a 4-byte length field can request a multi-GB allocation before any data is read
- For `unsafe`/`cgo`: a length or offset crosses into C or `unsafe.Slice` where Go's bounds check no longer applies

## Common false positives
- The index/bound is already validated (`if n < 0 || n > len(buf) { return err }`) before use
- `make` size is a small constant or capped (`if n > maxLen { return err }`)
- The slice expression uses `len()`/`cap()` of the same slice, which can't go out of range
- A deferred `recover()` at a per-request boundary downgrades a panic to a handled 500 (mitigation, not a fix — note it)

## Verify (Phase 4)
- **Panic shapes:** a `_test.go` or fuzz target that feeds the out-of-range index/bound; capture the `panic: runtime error: index out of range [n] with length m` and stack. Deterministic, no sanitizer needed.
- **Resource exhaustion:** demonstrate the allocation scales with the untrusted field — e.g., a 5-byte input declaring length `0x7fffffff` causes a multi-GB `make`. Show the allocation (memstats or an OOM under a `GOMEMLIMIT`/`ulimit -v` cap) rather than actually exhausting the host.
- **`unsafe`/`cgo` OOB:** build with the race detector and/or run under ASan via cgo (`CGO_ENABLED=1 go build -asan` on supported platforms) — this is the only Go sub-shape that produces a real memory-corruption diagnostic.

## Fix shape
Validate before use, and cap allocations from untrusted sizes:
```go
if n < 0 || n > len(buf) {
    return fmt.Errorf("offset %d out of range", n)
}
chunk := buf[:n]

const maxAlloc = 1 << 20
if hdr.Length > maxAlloc {
    return fmt.Errorf("declared length %d exceeds cap", hdr.Length)
}
out := make([]byte, hdr.Length)   // and only grow as data actually arrives
```
Prefer streaming/`io.LimitReader` over pre-allocating from a declared length.
