# Report template

One file per finding, named `<finding-id>.md`, placed in `reports/`. Plus a `SUMMARY.md` aggregating all findings with severity ranking.

## Per-finding report

```markdown
# <FIND-N>: <one-line subject>

**Severity:** <level> (see severity.md)
**Status:** confirmed | unconfirmed-lead | demoted
**Target:** <project> @ <commit-hash>
**Subsystem:** <subsys>
**Location:** <file>:<lines>, function `<func>`
**Discovered via:** <static-analysis | variant-analysis | targeted-pattern | data-flow | combination>
**Reported:** <pending | private-to-<contact>-on-<date> | public-on-<date>>

## Summary

<One paragraph, plain English, no jargon. A maintainer scanning the
SUMMARY.md should know from this paragraph alone whether to read further.>

## Root cause

<The technical defect — what the code does wrong. Quote the offending
lines with line numbers. Be specific about which variable is wrong,
which check is missing, which arithmetic wraps.>

## Triggering conditions

<Exactly what input shape causes the bug. If it's reachable from the
network, name the protocol field. If it's from a file, name the format
field. If from a variable, name the variable. Include numeric ranges:
"length field set to a value such that ... > sizeof(...)".>

## Impact

<Be honest. Three subsections:>

**Demonstrated:** <what the PoC actually shows — e.g., "16-byte OOB read
that gets copied back to the peer in outgoing SACK options">

**Plausible but not demonstrated:** <what could happen with more work,
clearly labeled as conjectural — e.g., "if struct layout changes,
the OOB could cross into adjacent heap allocations">

**Bounded by:** <what limits the impact — e.g., "the write lands on
in-object fields, not across the heap allocation boundary">

## Reproducer

PoC source: `pocs/<finding-id>.c`
Sanitizer output: `pocs/<finding-id>.asan.txt`

Run:
\```
gcc -g -fsanitize=address -fsanitize=undefined -o poc pocs/<finding-id>.c
./poc
\```

Excerpt from sanitizer output:
\```
<paste the most informative 10-20 lines of ASan output here>
\```

## Fix

Patch: `patches/<finding-id>.patch`

The fix <one-sentence summary of what changed>. This <one-sentence why
this fix is correct, ideally referencing a sibling pattern in the
codebase that does it right>.

## Variants checked

<If you ran variant analysis on this finding (Phase 5 / chaining mode 3 or 4),
list which other locations you checked and what you concluded. Examples:>

- <file>:<func> — same pattern, but bounded by upstream guard, safe.
- <file>:<func> — same root cause, fixed in same patch.
- <file>:<func> — not checked yet (deferred to next audit).

## Open questions / limitations

<What you didn't prove. What further investigation could establish.
This section is required, not optional — every finding has limits and
acknowledging them is what makes the rest credible.>

## Disclosure

**Channel:** <private to <contact> | public via <list/PR>>
**Reasoning:** <why this channel: project policy + severity>
**Draft:** `disclosure/<finding-id>.md`
```

## SUMMARY.md aggregate

```markdown
# bug-hunter run summary

**Target:** <project> @ <commit>
**Date:** <date>
**Phases completed:** orient, discover, review, confirm, produce
**Tools used:** <list>

## Findings

| ID | Severity | Subsystem | Subject | Disclosure |
|----|----------|-----------|---------|------------|
| FIND-1 | high | net/tcp | OOB write in SACK insertion (tcp_hole) | private |
| FIND-2 | medium | efi_loader | Ignored validator return in get_dp_device | public |

## Unconfirmed leads (deferred)

| ID | Subsystem | Subject | Reason deferred |
|----|-----------|---------|-----------------|
| CAND-7 | fs/cramfs | Decompressor output not bounded | legacy filesystem, low priority |

## Recommendations

<Anything project-wide observed during the audit — e.g., "two findings
share a root cause in array-shift-count handling; the project might
benefit from a code-review pass over similar memmove patterns elsewhere.">
```
