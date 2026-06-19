# bug-hunter skill

A reusable security-research skill for finding, confirming, chaining, and reporting bugs in open-source codebases.

## What it does

- **Audits** any C/C++ codebase for memory-safety, input-validation, logic, and data-flow bugs
- **Ranks** candidates by confidence × reachability × severity-if-real
- **Confirms** memory-safety candidates with sanitizer-level PoCs (ASan/UBSan/MSan)
- **Chains** related findings (data flow, multi-bug, variant analysis, pattern repetition)
- **Generates** upstream-ready patches, reports, PoCs, and disclosure drafts
- **Remembers** per-project context and a shared cross-project pattern library

## Hard limits (built into the skill, not optional)

- PoC generation stops at sanitizer-crash proof. No exploits, shellcode, ROP, or mitigation bypasses.
- Findings must be confirmed (sanitizer PoC or worked logic example) before being reported.
- Disclosure defaults to the project's stated policy, with severity-based overrides for serious bugs.
- Speculation and unconfirmed leads are kept separate from findings.

## Structure

```
SKILL.md                          entrypoint Claude reads
references/                       phase-by-phase methodology
  orient.md                       phase 1: clone, fingerprint, history
  discover.md                     phase 2: static + variant + pattern + dataflow
  review.md                       phase 3: present candidates to human
  confirm.md                      phase 4: build sanitizer PoCs
  produce.md                      phase 5: patches, reports, disclosure
  chaining.md                     four chaining modes
  disclosure.md                   policy reading, private vs public
  memory.md                       per-project notes + shared library
  severity.md                     honest severity rubric
patterns/                         shared cross-project pattern library
templates/                        patch, report, PoC, disclosure formats
examples/                         a real walkthrough (U-Boot)
```

## How to invoke

Drop the skill into the path your runtime reads skills from. Then prompt with anything matching the description in `SKILL.md`'s frontmatter — "audit this repo," "find bugs in net/tcp.c," "variant-analyze CVE-X across the codebase," etc.

The skill runs in five phases; phase 3 is a hand-off point where the human reviews candidates before confirmation work begins.

## Adapting it

- New language: add language-tagged entries under `patterns/` and adjust `references/discover.md` Technique A (the analyzer commands).
- New bug class: add a `patterns/<name>.md` file with the frontmatter, detection grep, "when buggy" / "false positives" sections, and verification recipe.
- Project-specific gotchas: these go in per-project notes (created on first run), not the skill itself.
