# Reachability confirmation — design

**Date:** 2026-06-19
**Skill:** bug-hunter
**Status:** approved, pending implementation plan
**Scope:** documentation-only changes to the skill (8 files). No code.

## Problem

The skill ranks candidates partly on **reachability** (a 0–5 dimension in the
Phase 2 composite score) but never *confirms* it. Phase 4 confirms only one
thing: that the buggy function corrupts memory (or panics) when fed specific
inputs. The dominant confirmation technique — port-and-isolate (Strategy A) —
hand-feeds those inputs to an *extracted* copy of the function. An ASan crash
from that harness proves the bug exists; it does **not** prove an attacker can
deliver the trigger through a real entry point.

The result: an assumed reachability gets laundered into a "confirmed" finding,
and the severity score (which weights reachability heavily) inherits an
unverified assumption. The U-Boot example already self-criticizes exactly this
("building a worked example through the sandbox build would have been stronger"
for FIND-1) but the methodology never formalized the fix.

## Solution overview

Make reachability a **second, independent confirmation axis** in Phase 4, with a
three-tier ladder. A finding requires both the bug *and* a reachability tier of
at least Traced. The tier mechanically caps severity and gates disclosure.

### The two-axis Phase 4

Phase 4 confirms two orthogonal things per candidate:

1. **Bug confirmation** (existing): did the sanitizer / panic / worked example
   fire?
2. **Reachability tier** (new): Asserted · Traced · Proven.

A candidate becomes a **finding** only if it is bug-confirmed **and**
reachability ≥ Traced. A bug-confirmed candidate that is only Asserted stays an
"unconfirmed lead" — no patch, no disclosure draft, recorded in per-project
notes.

### Reachability tiers

- **Asserted** — the Phase 2 reachability score said "reachable," but no evidence
  has been produced. This is the default starting state of every candidate. Not
  promotable to a finding.
- **Traced** — a written call-graph trace from a *documented untrusted entry
  point* to the sink, in which **every precondition on the path is labeled**:
  `attacker-controlled`, `attacker-influenced`, or `assumed-environmental`. No
  `assumed-environmental` link may sit on the critical path without an explicit
  justification. This is the ceiling achievable by reading code plus a
  port-and-isolate PoC.
- **Proven** — the trigger was delivered through the **real entry point**: an
  in-place sanitizer build (sandbox/userspace harness invoking the actual entry
  function), fuzz-through-entry, or a malformed file fed to the real parser
  binary — and the sanitizer/panic fired from that path, not from an extracted
  function.

### Strategy A / B mapping (the tie-in)

The two tiers map onto the two confirmation strategies already documented in
`confirm.md`:

- **Strategy A (port-and-isolate)** maxes out at **Traced**. Its inputs are
  synthetic and hand-fed to an extracted function, so it proves the bug but not
  the delivery.
- **Strategy B (in-place / real entry point)** is what reaches **Proven**.

Stated explicitly so that "I want to claim Critical" mechanically pushes the
researcher toward Strategy B.

## Gating rules

Applied in Phase 4 (tier assignment) and enforced in Phase 5 (severity +
disclosure).

| Tier | How achieved | Severity cap | Disclosure |
|------|--------------|--------------|------------|
| **Asserted** | score only, no trace | capped at **Low** | **blocked** — stays unconfirmed lead, no draft/patch |
| **Traced** | written end-to-end trace, preconditions labeled | capped at **High** (no Critical) | allowed; trace attached as reachability evidence |
| **Proven** | driven through real entry point | **uncapped** (Critical possible) | allowed |

Severity is computed as `final = min(rubric_severity, reachability_cap)`, applied
**before** the existing honesty demoters in `severity.md` (adjacent-fields,
read-vs-write, etc.). The two stack: reachability caps the ceiling, the demoters
can lower it further.

Note on Asserted: because an Asserted candidate is by definition not a finding,
its disclosure block is the operative effect — it never reaches Phase 5 artifact
generation. The Low cap is the floor that applies *only* if the user explicitly
overrides the gate to record it anyway (the skill permits user override
elsewhere); it exists so an overridden Asserted candidate can never be dressed up
as more than Low.

## File-by-file changes (8 files, all docs)

1. **`references/confirm.md`** — restructure Phase 4 around the two axes. Add the
   reachability ladder section. Map Strategy A → Traced-max, Strategy B →
   Proven. Update "Output of Phase 4" to record both the bug-confirmation result
   and the reachability tier per candidate.

2. **`references/severity.md`** — add a "Reachability gate" subsection defining
   `final = min(rubric, reachability_cap)` and the Asserted→Low / Traced→High /
   Proven→uncapped caps, applied before the existing demoters. Add a worked line
   to the `tcp_hole` calibration example showing the cap interacting with the
   rubric.

3. **`references/produce.md`** — disclosure/patch generation requires reachability
   ≥ Traced. Asserted-only candidates produce no artifacts beyond a per-project
   notes entry. Cross-reference the severity gate in the severity-gating section.

4. **`references/disclosure.md`** — add the reachability precondition to the
   default-selection logic: no disclosure (private or public) for a candidate
   below Traced, regardless of apparent severity.

5. **`templates/report.md`** — add a `## Reachability` section to the per-finding
   template: tier (one word), named entry point, the trace (or the end-to-end
   repro command + output for Proven), and each precondition labeled. The
   Severity section must state "capped at X by reachability tier Y" when a cap
   fired. Add a Reachability column to the `SUMMARY.md` findings table.

6. **`references/discover.md`** — relabel the Phase 2 reachability score (0–5) as
   a **reachability hypothesis** explicitly "to be confirmed in Phase 4." Ranking
   still uses it for triage; the candidate output marks it unconfirmed.

7. **`SKILL.md`** — update the Phase 4 high-level-workflow bullet ("build a
   sanitizer-level PoC **and** establish the reachability tier"), update the
   finding definition in hard-limit #2 to require reachability ≥ Traced, and add
   an operating principle: *"Confirm reachability, not just the bug."*

8. **`README.md`** — "What it does": Confirms = bug **+** reachability tier. Add a
   short "How findings are gated" note to the usage section explaining the three
   tiers and the cap+block behavior, so a capped/blocked finding is not a
   surprise to the user.

## Example annotation (calibration)

In `examples/u-boot-walkthrough.md`, retro-tag both findings with their tiers
under the new model:

- **FIND-2 (`tcp_hole`)** — port-and-isolate ⇒ **Traced**, which caps it at High.
  This is consistent with its existing "medium-to-high" rating. Note what Proven
  would have required: a crafted TCP peer driving the SACK state against the
  sandbox build.
- **FIND-1 (capsule)** — caller-contract logic trace ⇒ **Traced**. Note what
  Proven would have required: a crafted `Boot####` variable fed through the
  sandbox build.

A short calibration note: neither finding reached Proven, both were correctly
reportable at Traced (cap at High), and the new model makes the latent
self-criticism in the example an explicit, repeatable rule.

## Out of scope

The other gaps surfaced in the earlier review are **not** part of this change:
already-fixed/known-CVE pre-disclosure gate, maintainer-silent fallback, optional
CVSS vector, endianness/architecture handling, MSan operationalization, and the
false-positive feedback loop. They remain open follow-ups.

## Success criteria

- Every confirmed finding in a run carries an explicit reachability tier and the
  evidence for it.
- A bug-confirmed-but-Asserted candidate cannot produce a disclosure draft.
- Severity is provably the min of the rubric value and the reachability cap.
- The U-Boot example demonstrates the tiering on real findings.
- No port-and-isolate PoC is ever described as establishing Proven reachability.
