# Phase 4: Confirm

Goal: for each selected candidate, build a **sanitizer-level Proof-of-Concept** that triggers the bug under ASan / UBSan / MSan. A candidate that produces a sanitizer diagnostic is a *finding*; a candidate that doesn't is demoted with an explanation.

## Hard limit (repeat from SKILL.md)

PoCs go up to "the sanitizer caught it." They do not go further. No shellcode, no ROP, no mitigation bypass, no working RCE. If the user requests these, decline that specific request and continue with the rest of the workflow.

## Strategy for memory-safety bugs

Two paths, prefer (B) when feasible.

### A — Port-and-isolate

Used when the buggy function is small and self-contained (the `tcp_hole` case from the reference U-Boot work).

1. Identify the smallest set of functions, structs, and constants needed to reproduce the bug.
2. Create a standalone C file in `bug-hunter-out/<project>/<ts>/pocs/<candidate-id>.c`.
3. Copy the function(s) **verbatim** from the project. Add a comment at the top stating which file and commit they came from, so reviewers can diff against upstream.
4. Define minimal stand-in structs/types so the code compiles.
5. Heap-allocate the relevant container with `malloc` (not stack — ASan catches heap OOB more reliably and the diagnostic is cleaner).
6. Drive the function into the buggy state with the inputs from the candidate's "verify by" plan.
7. Compile with `gcc -g -fsanitize=address -fsanitize=undefined -o <id> <id>.c` and run.
8. Capture the full sanitizer output; save to `pocs/<id>.asan.txt`.

The U-Boot `tcp_hole` PoC is the reference shape — keep PoCs at that level of minimal-and-faithful.

### B — In-place sanitizer build

Used when the project's own build supports sanitizers (Linux kernel KASan, U-Boot sandbox build, libfuzzer harnesses already present).

1. Build the project with the relevant sanitizer flags (the project's `Makefile` usually has a switch; if not, edit `CFLAGS`).
2. Drive the target with a crafted input — feed it through the project's own entry points (e.g., feed a malformed file to a parser binary, feed a packet to a sandboxed network entry).
3. Capture the sanitizer report.

This is more credible to maintainers because it runs the *real* code path, not a port. Try (B) first; fall back to (A) if the build setup is too tangled to get working in the session.

## Strategy for logic / input-validation bugs

These often don't crash a sanitizer (no memory corruption involved). For these:

1. Write a worked example: "given input X, the function returns Y; the correct answer is Z."
2. If possible, instrument with `printf` or a small test driver that prints the intermediate state, demonstrating the wrong branch / wrong value.
3. Save the trace as `pocs/<id>.trace.txt`.

A logic finding without a worked example stays an "unconfirmed lead." Do not promote it.

## Strategy for Go targets

Go's runtime makes most bugs panics rather than corruption, so the "sanitizer" is usually just running the code and capturing the panic stack. Prefer, in order:

1. **Native fuzzing** — if the path parses untrusted bytes, write a `FuzzXxx` target and run `go test -fuzz=FuzzXxx -fuzztime=60s`. A crasher written to `testdata/fuzz/` is a reproducible, maintainer-credible confirmation. This is the highest-value Go confirmation technique, the rough analog of port-and-isolate + ASan.
2. **Targeted test driver** — a small `_test.go` that calls the path with the input that forces the bug (the error path for `go-nil-deref-after-error`, the out-of-range index for `go-slice-make-bounds`). Capture the `panic: runtime error: ...` line plus the goroutine stack; that stack is the proof, paste it into the report where ASan output would go.
3. **Race detector** — for concurrency findings, `go test -race ./...`. A race report (with both stacks) is the confirmation.
4. **`unsafe`/`cgo` paths only** — these can corrupt memory like C. Build with `CGO_ENABLED=1 go build -asan` (or `-race`, which also instruments some memory errors) to get a real OOB diagnostic. This is the only Go sub-class where you should expect a sanitizer-style report.

Resource-exhaustion findings (huge `make` from an untrusted length) are confirmed by *demonstrating the scaling*, not by actually OOMing the host: show the allocation tracks the attacker field under a `GOMEMLIMIT`/`ulimit -v` cap, or instrument with `runtime.ReadMemStats`.

Note severity honestly: an unrecovered panic in a CLI is low; the same panic reachable from an HTTP handler with no `recover` middleware is a remote DoS. If a framework `recover()` catches it, it's a per-request 500, not a process crash — say which.

## When confirmation fails

If you cannot get the sanitizer to fire after a serious attempt:

- Re-read the code. The candidate may have been wrong — there may be an upstream guard you missed.
- Try a different input shape (read the candidate's "invalidators" from Phase 2 — those are now the things to disprove).
- If after a reasonable effort it still won't trigger, demote the candidate. Record what you tried in the per-project notes so the next run doesn't repeat the dead end.

Demotion is not failure. It is the *correct* result for an unreal candidate, and maintaining that discipline is what keeps the skill credible.

## Output of Phase 4

For each candidate that confirmed:
- Promoted to "finding" with a stable ID
- PoC source and sanitizer output saved
- Notes on *exactly* what gets corrupted (which adjacent fields the OOB lands on, for example — this is critical for the severity discussion in Phase 5)

For each candidate that did not confirm:
- Marked demoted with one-line reason
- Per-project notes updated

Then summarize: "Phase 4 complete. <K> findings confirmed, <M> candidates demoted."
