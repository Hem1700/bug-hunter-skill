---
id: go-nil-deref-after-error
languages: [go]
cwe: 476
severity-typical: medium
seed-cves: []
---

# Go: nil dereference after an unchecked error

The pattern: a function returns `(value, error)`, the caller ignores or mishandles the `error`, and then uses `value` — which on the error path is the zero value (often a `nil` pointer, `nil` map, or `nil` interface). Dereferencing it panics. In a long-running server, an attacker who can reach this path with crafted input gets a remote panic = denial of service (Go panics unwind the goroutine and, if unrecovered, crash the process).

This is the Go analog of `c-ignored-validator-return`. It is the single most common reachable bug class in Go services, because Go's explicit-error idiom makes the ignored-error footgun ubiquitous.

## Detection
```
# errors dropped with the blank identifier
grep -rnE '\b\w+,\s*_\s*:?=\s*\w+\(' <subsys>

# the result used despite an error that is checked-but-not-returned-early
grep -rnE ':?=\s*\w+\([^)]*\)\s*;?\s*$' <subsys>   # then read for a missing `if err != nil`
```
Far better than grep: run the dedicated analyzers.
```
go vet ./...
errcheck ./...        # flags every unchecked error return
staticcheck ./...     # SA5011: possible nil dereference; SA4006 etc.
nilaway ./...         # Uber's nilness analyzer, if available
```
`errcheck` + `staticcheck SA5011` together catch most real instances.

## When the pattern is buggy
- The error is dropped (`_`) or logged-but-not-returned, and control falls through to use the value
- On the error path the returned value is `nil`/zero (the common Go contract: "on error, the value is unspecified — usually the zero value")
- The value is then dereferenced, indexed, ranged over with assumptions, or has a method called on it
- The path is reachable from untrusted input (an HTTP handler, an unmarshaled request, a parsed file), so the panic is attacker-triggerable

## Common false positives
- The error is genuinely safe to ignore and the value is valid on both paths (e.g., `strings.Builder.Write` never errors; `fmt.Fprintf` to a buffer)
- A `nil` check on the value guards every dereference regardless of the error
- The panic is caught by a `recover()` in a deferred handler that the framework guarantees (many HTTP frameworks recover per-request — this downgrades severity from crash to per-request 500, note it but don't call it safe in non-HTTP contexts)
- The function's documented contract is "value is always valid, error is advisory"

## Verify (Phase 4)
Go's "sanitizer" for this class is just running it — a nil deref panics deterministically.

1. Write a small `_test.go` driver (or a `main`) that calls the path with the input that forces the error (e.g., malformed bytes into the parser, a lookup that misses).
2. Run it; capture the `panic: runtime error: invalid memory address or nil pointer dereference` plus the goroutine stack. That stack **is** the proof — paste it into the report like ASan output.
3. For server code, show the panic is unrecovered at the relevant boundary (or, if recovered, document it as a per-request DoS rather than a process crash).

Native fuzzing is excellent here when the path parses untrusted bytes:
```
go test -fuzz=FuzzParse -fuzztime=60s
```
A crasher in `testdata/fuzz/` is a reproducible confirmation.

## Fix shape
Handle the error before touching the value:
```go
v, err := lookup(key)
if err != nil {
    return nil, fmt.Errorf("lookup %q: %w", key, err)
}
// v is safe to use here
return v.Field, nil
```
If a recover boundary is the intended safety net (HTTP middleware), that mitigates impact but is not a fix — the nil deref should still be eliminated.
