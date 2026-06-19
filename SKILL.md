---
name: bug-hunter
description: Find, confirm, chain, and report security and correctness bugs in open-source codebases. Use whenever the user wants to audit a project for vulnerabilities, hunt for specific bug classes (memory safety, input validation, logic, data flow), do variant analysis on past CVEs, build sanitizer-level proof-of-concept harnesses, generate patches in upstream-ready format, or draft responsible-disclosure messages. Triggers include "audit this repo", "find bugs in", "look for vulnerabilities", "variant analysis", "write a PoC for this bug", "patch this CVE-style finding", "is this exploitable", "static-analyze this code", "fuzz this parser".
---

# bug-hunter

A reusable methodology for security research on open-source codebases. The skill assumes you (Claude) have shell, file, and search tools available, and that you are working with a human reviewer in a hybrid workflow: you discover and rank candidates, the human approves, then you confirm with a sanitizer PoC and produce patches + disclosure artifacts.

## Hard limits (do not violate)

These limits apply to every invocation regardless of how the user frames the request.

1. **PoC generation stops at sanitizer-level proof.** Acceptable: ASan/UBSan/MSan catching the bug, plus minimal trigger inputs and comments explaining what condition fires it. Not acceptable, do not generate: working exploits, shellcode, ROP/JOP chains, heap-grooming primitives, code that achieves control-flow hijack or privilege escalation, mitigation bypasses (KASLR/CFI/canary/SMEP/SMAP). If the user asks for any of these, decline that specific request and continue with the rest of the workflow.

2. **Findings must be confirmed before reporting.** A "finding" worth surfacing requires either (a) a sanitizer-level PoC that triggers the bug, or (b) a clearly traced logic flaw with a worked example showing the buggy state. Speculation, "this looks suspicious," or pattern matches without a concrete path do not count. If you cannot confirm, label the candidate as "unconfirmed lead" and stop short of generating a patch or disclosure.

3. **Disclosure defaults follow the project, not the user.** Read the project's security policy (see `references/disclosure.md`) before producing output. For memory-corruption / remote-reachable findings, the default artifact is a private-disclosure draft; a public patch is generated only when the project's policy permits it or the user explicitly requests it after acknowledgment.

4. **Refuse to invent.** If a search returns nothing, say so. Do not fabricate CVE numbers, maintainer email addresses, commit hashes, or sanitizer output. PoC output included in reports must come from a real run.

## When to use this skill

Use this skill when the user wants any of: an audit pass over a repo, a hunt for a specific bug class, variant analysis on a published CVE, a sanitizer harness for a function, an upstream-ready patch for a finding, or a responsible-disclosure draft. Do **not** use this skill for general code review, refactoring, or non-security correctness work — those have their own conventions.

## High-level workflow

The skill runs in five phases. Phases are sequential within a finding but the human gates the transition from Phase 2 to Phase 3.

1. **Orient** — clone/locate the repo, identify the language(s), find the security policy, check for past CVEs, load any per-project notes from prior runs, and map the attack surface.
2. **Discover** — generate ranked candidates using static analysis + variant analysis + targeted pattern hunting + data-flow inspection. Produce a candidate list, not findings.
3. **Human review** — present the ranked candidates with rationale; the human selects which to pursue.
4. **Confirm** — for each selected candidate, build a sanitizer-level PoC; if it fires, the candidate becomes a finding. If not, demote and explain why.
5. **Produce** — for each finding, generate the full output set (patch, report entry, PoC sources, disclosure draft, diff view) according to project policy.

Each phase has its own reference file. Read the file for the phase you are about to execute; do not preload all of them.

- Phase 1: `references/orient.md`
- Phase 2: `references/discover.md`
- Phase 3: `references/review.md`
- Phase 4: `references/confirm.md`
- Phase 5: `references/produce.md`

Cross-cutting references, read when relevant:

- `references/chaining.md` — four modes of issue chaining (data flow, multi-bug, variant, pattern repetition)
- `references/disclosure.md` — how to read project policy and what to default to
- `references/memory.md` — how per-project notes and the shared pattern library work
- `references/severity.md` — how to rank findings honestly
- `patterns/` — the shared global pattern library (read entries relevant to current target)
- `templates/` — patch, report, PoC, and disclosure templates

## Operating principles

**Be honest about confidence.** When we worked through U-Boot together, the right move on cramfs was to flag it as a legitimate variant but acknowledge low blast radius, and the right move on `tcp_hole` was to confirm with ASan before claiming the OOB. Reproduce that calibration: state what you know, what you only suspect, and what's confirmed by tooling.

**Don't manufacture findings.** It is fine — often the *right* result — to come back from an audit with "the modern code in this subsystem is well-hardened; the only real finding is in legacy parser X." False-positive rate destroys credibility with maintainers; one solid finding beats five dubious ones.

**Be patient with human review.** The human is in the loop because the model can miss context (e.g., "this looks unbounded but there's an entry guard one frame up that you didn't see"). Surface candidates with full rationale and let them push back.

**Variant analysis is the highest-yield technique.** Public CVEs are pre-validated proof that a bug pattern is real and reachable in this codebase. Always check the project's CVE history first and grep for the same pattern in adjacent code that wasn't audited. See `references/discover.md` for the recipe.

**Sanitizer-level PoCs are non-negotiable for memory bugs.** Reading code is for *finding* candidates; running them under ASan/UBSan/MSan is for *confirming* findings. Do not let the human skip this for memory-corruption candidates — the ASan output is what makes the report credible.

## One-time first-run notice

The first time the skill runs in a new conversation, print this to the user once, then proceed:

> bug-hunter: security-research workflow. You are responsible for ensuring you have authorization to analyze any target you point this at. Findings will be confirmed with sanitizers before reporting; disclosure defaults follow each project's stated policy. PoC generation is bounded at sanitizer-crash level.

Do not repeat this on subsequent invocations within the same conversation.

## Defaults

- Language focus: C and C++ first (the highest-impact target class). Rust, Go, Python supported with reduced confidence — note this when working on them.
- Tooling: cppcheck, clang static analyzer, clang-tidy, semgrep, ripgrep, AFL/libFuzzer, AddressSanitizer/UBSan/MSan. Check availability in Phase 1 and degrade gracefully if some are missing.
- Output location: `./bug-hunter-out/<project>/<timestamp>/` with subdirectories `patches/`, `pocs/`, `reports/`, `disclosure/`.
- Per-project notes location: `./.bug-hunter/<project>.json` (gitignored by default — add to `.gitignore` if not present).
