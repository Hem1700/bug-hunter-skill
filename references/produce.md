# Phase 5: Produce

Goal: for each confirmed finding, generate the full output set. This is the artifact phase. By now the work is done — what remains is packaging.

## Output set per finding

Each finding produces these artifacts under `bug-hunter-out/<project>/<timestamp>/`:

1. **Patch file** (`patches/<finding-id>.patch`) — upstream-ready in the project's preferred format
2. **PoC source + sanitizer output** (`pocs/<finding-id>.c` and `pocs/<finding-id>.asan.txt`) — from Phase 4
3. **Report entry** (`reports/<finding-id>.md`) — structured finding writeup
4. **Disclosure draft** (`disclosure/<finding-id>.md`) — pre-filled message to maintainer/security contact, formatted per project policy
5. **Diff view** (included in the report) — before/after of the fix, with annotations

Plus one consolidated `reports/SUMMARY.md` listing all findings with severity ranking.

## Generating the patch

Use `templates/patch.md` as the template. Key requirements:

- **Format the project uses.** Linux/U-Boot/most kernel projects: emailed patches via `git format-patch`. GitHub-centric projects: a branch with a PR-ready commit. Check the project's CONTRIBUTING file in Phase 1 and follow it.
- **Minimal diff.** Change only what's needed to fix the bug. Style cleanups go in separate patches.
- **Commit message structure**:
  - One-line subject in the project's conventional prefix (e.g., `efi_loader: ...`, `net: ...`)
  - Body: what the bug is, how it's triggered, what gets affected, what the fix does
  - `Fixes:` tag pointing to the commit that introduced the bug, if found via `git blame` / `git log -S`
  - `Signed-off-by:` line — use the human's real identity from their `git config`, not a placeholder. Ask if not set.
- **Run the project's checks** (`scripts/checkpatch.pl`, `cargo fmt`, etc.) if they exist and shell tools allow.

If the project's policy requires private-first disclosure (see `disclosure.md` and Phase 1 notes), the patch goes in `patches/` but the disclosure draft makes clear it's an attachment for the security contact, not for the public list. Adjust the cover letter accordingly.

## Generating the report entry

Use `templates/report.md`. The report is what the human reads to understand and explain the finding. It must include:

- **Summary**: one-paragraph plain-English description
- **Location**: file, function, lines (anchored to the commit pinned in Phase 1)
- **Root cause**: technical description of the defect
- **Triggering conditions**: exactly what input shape causes it
- **Impact / blast radius**: what gets corrupted, what's adjacent, what an attacker could observe or affect — be specific and honest (see `severity.md`)
- **Severity**: with the rubric from `severity.md`, not a vibe
- **PoC reference**: link to the PoC file and sanitizer output
- **Fix**: the diff plus a paragraph explaining why this fix is correct
- **Limitations / open questions**: what you couldn't prove

## Generating the disclosure draft

Read `disclosure.md` for how to choose private-vs-public based on project policy and severity. The skill must not silently choose for the user; surface the policy lookup and the chosen default, and accept override.

Template the draft using `templates/disclosure-private.md` or `templates/disclosure-public.md` as appropriate. Fields to fill:

- To/CC (from project policy or maintainers list)
- Subject (clear, includes project + subsystem + class)
- Body: short summary, severity claim, attached PoC + patch, request for acknowledgment + embargo discussion if relevant
- Sign-off

Do **not** invent maintainer email addresses. If you didn't find them in Phase 1, leave a placeholder and tell the human to fill it in.

## Severity gating (cross-reference with disclosure.md)

If the finding is rated "critical" (see `severity.md` for the rubric) **and** the project policy requires private-first disclosure for that class, then in this phase:

- Generate the disclosure draft *first*, output it prominently
- Generate the patch but place it in `patches/private/` (not the default `patches/` location) and label it as "attach to private disclosure"
- Note in `SUMMARY.md` that this finding requires the human to confirm private disclosure has been sent before any public submission

This is the "force the workflow" path. For non-critical findings or projects with lenient policies, all artifacts go to their default locations and the human is free to choose.

## Updating persistent state

After producing artifacts, update:

- Per-project notes (`memory.md`): record the finding, the audit pass it came from, and any new validated-hardened areas discovered along the way.
- Shared patterns library (`memory.md`): if a finding represents a generalizable pattern that wasn't already in `patterns/`, propose an addition. Do not silently add — flag it for the user to confirm before writing to the shared library, since it affects all future runs.

## Output of Phase 5

A printed summary to the user:

```
<N> findings produced.
  <id>: <subject>  [severity]
    patch:      <path>
    poc:        <path>
    report:     <path>
    disclosure: <path>  [private|public]
  ...
Summary: bug-hunter-out/<project>/<ts>/reports/SUMMARY.md

Recommended next steps:
  1. Review reports/SUMMARY.md
  2. For each [private] disclosure, send to the listed contact and await acknowledgment
  3. For each [public] patch, run the project's submit workflow (checkpatch.pl + git send-email, or PR)
```
