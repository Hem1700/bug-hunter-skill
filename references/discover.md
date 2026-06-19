# Phase 2: Discover

Goal: produce a ranked list of **candidates** — places worth a human's attention. Not findings. A candidate becomes a finding only after Phase 4 confirmation.

The four techniques below are complementary, not alternatives. Use all four; their hits overlap on the strongest candidates.

## Technique A — Static analysis

Run available analyzers across the chosen subsystem. Capture raw output, then filter to memory-safety-relevant signal.

```
cppcheck --enable=warning,portability --inconclusive -q <subsys>/ 2>&1 \
  | grep -iE 'overflow|bound|buffer|integer|underflow|uninit|alloc|negative|array|index|null'
```

For clang:
```
scan-build -o /tmp/cb --status-bugs <build cmd>
```

For semgrep, prefer rules from `patterns/semgrep/`. Run with `--config patterns/semgrep/`.

**Treat every hit as a *lead*, not a finding.** Static-analyzer false positives are the norm. Filter aggressively before showing to the human:
- Drop pure style/format-string noise unless the format specifier is user-controlled
- Cluster duplicates (same function, same pattern → one candidate)
- Read the surrounding code for each hit and drop the ones with an obvious upstream guard the analyzer didn't model (this is the work that beats pure tooling)

## Technique B — Variant analysis

This is the highest-yield technique. For each past CVE in the project (loaded in Phase 1):

1. Read the patch commit, identify the *exact* bug pattern (e.g., "16-bit length field used in arithmetic that's stored in a 32-bit variable, allowing wraparound").
2. Grep the codebase for the same pattern in adjacent code that wasn't part of the original audit. Example from U-Boot:

   The 2024 SquashFS CVEs were `malloc(le32_value + 1)` with wraparound. Variant search:
   ```
   grep -rnE '(malloc|zalloc|calloc|realloc)\s*\([^)]*[\+\*][^)]*\)' fs/<other_parsers>
   ```
   Then read each hit to check whether the size has equivalent provenance and whether there's a bound.

3. Repeat across all CVEs in scope. CWE classes that variant-analyze well:
   - CWE-787 / 125 (buffer over/underflow): grep for unbounded `memcpy`/`memmove`/`strcpy`
   - CWE-190 (integer overflow): grep for arithmetic inside allocation sizes
   - CWE-193 (off-by-one): grep for `<` vs `<=` near size constants
   - CWE-457 (uninitialized variable use): grep for stack-allocated structs passed to functions that return error codes
   - CWE-476 (NULL deref): grep for return values not checked

4. Record each candidate with: the seed CVE, the variant location, why you think it applies, and what would invalidate it.

## Technique C — Targeted pattern hunting

Independent of CVE history, hunt language-specific dangerous patterns. The `patterns/` directory has a curated list — read the entries that match the current target language and subsystem. Each pattern entry has:
- A grep/semgrep recipe
- The conditions under which the pattern is actually buggy
- Common false-positive flavors to skip

Read `patterns/c-memory.md`, `patterns/c-parsers.md`, etc., depending on target.

## Technique D — Data-flow inspection (taint)

For network protocol parsers and other untrusted-input handlers:

1. Identify the **untrusted source** in the subsystem (the function that receives the bytes, e.g., `nfs_handler` in U-Boot, `recvfrom`, a deserializer entry point).
2. Identify the **dangerous sinks**: `memcpy`, `memmove`, allocations, array index expressions, control-flow decisions on integer values, format strings.
3. Walk the call graph from source toward sinks. At each step, ask "is there a length validation here?" The pattern `if (X > sizeof(Y)) return;` is a guard; `if (X > 0)` is not.

A tainted value reaching a sink **without** a validation between source and sink is a candidate. Note the call chain.

Useful starting query:
```
grep -rnE 'recv|read.*pkt|deserialize|decode|parse' <subsys>/ | grep -i 'unsigned int len\|size_t len\|u32 len'
```

## Chaining (during discovery)

While running A-D, watch for the chaining modes documented in `chaining.md`. In particular:

- If candidate X is "info-leak via OOB read" and candidate Y is "use-after-free that needs a known pointer," note them as a *potential chain* (Y becomes more dangerous if X works).
- If you find the same pattern in three different files, that's variant repetition — surface them as one finding with three locations.

## Ranking candidates

Produce a single ranked list. Sort by a composite score:

- **Confidence** (how likely is this actually a bug? 0–5): static-analyzer hits start at 2; variant analysis with clear seed CVE starts at 3; tainted-source-to-sink with no guards starts at 4.
- **Reachability** (0–5): unauthenticated remote = 5; authenticated remote = 4; local untrusted disk = 3; local-physical = 2; trusted-only = 1.
- **Severity-if-real** (0–5): OOB write = 5; OOB read of sensitive memory = 4; OOB read of stack/heap = 3; logic bug = 2; DoS only = 1.

`score = confidence + reachability + severity_if_real`. Sort descending. Group by subsystem so the human reviews them coherently.

## Output of Phase 2

Print a candidate list of the form:

```
[CAND-1] score=12 (C5/R5/S5) — net/tcp.c:728 — OOB write in tcp_hole SACK insertion
  Seed: variant of <ref CVE / pattern>
  Why: <one-paragraph rationale, including the path/guards/lack thereof>
  Invalidators: <what would prove this wrong; e.g., "if all callers cap cnt at 3"
  Verify by: <how Phase 4 would confirm>
```

Then: "Phase 2 complete. <N> candidates ranked. Which to confirm?"
