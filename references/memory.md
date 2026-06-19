# Memory: per-project notes and shared pattern library

Two storage tiers.

## Per-project notes

Location: `./.bug-hunter/<project>.json` in the user's working directory (not in the skill's directory). Created on first run for a given project; updated at the end of each run.

Schema:

```jsonc
{
  "project": "u-boot",
  "repo_url": "https://github.com/u-boot/u-boot",
  "license": "GPL-2.0",
  "last_audit_commit": "abcdef1234",
  "last_audit_date": "2026-06-18",
  "security_policy": {
    "contact": "u-boot@lists.denx.de, trini@konsulko.com",
    "private_first": false,
    "embargo_default_days": null,
    "format": "email patch via git send-email",
    "policy_doc_path": "doc/develop/security.rst"
  },
  "cve_history": [
    { "id": "CVE-2024-57254", "cwe": "CWE-190", "subsystem": "fs/squashfs", "pattern": "zalloc(le32 + 1) wraparound", "fix_commit": "..." }
  ],
  "validated_hardened": [
    { "path": "net/bootp.c", "as_of_commit": "abcdef1234", "notes": "truncate_sz caps at maxlen-1; off-by-one not present" }
  ],
  "open_candidates": [
    { "id": "CAND-7", "path": "fs/cramfs/cramfs.c:105", "summary": "symlink decompress lacks output bound check", "status": "deferred-low-priority", "rationale": "legacy fs, blast radius low" }
  ],
  "confirmed_findings": [
    { "id": "FIND-1", "subject": "efi_loader: get_dp_device ignores deserialize return", "severity": "medium", "disclosed_to": "u-boot list", "date": "..." }
  ],
  "project_gotchas": [
    "EFI device paths must be validated by efi_dp_check_length before any walker is called",
    "struct rpc_t is 1152 bytes; len bounded in nfs_handler entry"
  ]
}
```

How to use:
- On orient: load if exists. Use `validated_hardened` to skip re-auditing those paths *unless* their containing files have changed since `as_of_commit`. Use `project_gotchas` to inform candidate filtering.
- On confirm: add to `confirmed_findings`. Move from `open_candidates` if the candidate originated there.
- On demote: keep in `open_candidates` with `status` set to `dismissed` + a reason, so the next run doesn't re-surface it.
- On end of run: write the updated file. Never delete history entries — append/update only.

## Shared pattern library

Location: `patterns/` directory of the skill itself. Files in this directory are read on relevant Phase 2 runs.

Format: each pattern is a markdown file with frontmatter.

```yaml
---
id: c-alloc-arith-wraparound
languages: [c, cpp]
cwe: 190
severity-typical: high
seed-cves: [CVE-2024-57255, CVE-2024-57256]
---

# Allocation arithmetic wraparound

## Detection
grep -rnE '(malloc|zalloc|calloc|realloc|kmalloc)\s*\([^)]*[\+\*][^)]*\)' <subsys>

## When the pattern is buggy
- The arithmetic operand is read from untrusted input (file, packet, env)
- The operand width allows the arithmetic to wrap (e.g., u32 size; size + 1)
- There's no explicit check that the operand is bounded below INT_MAX / similar

## Common false positives
- Operand is a fixed enum or compile-time constant
- Operand is bit-masked or capped immediately before the alloc
- Operand is a 16-bit field where +1 cannot wrap a sufficient buffer
```

How to use:
- During Phase 2 Technique C, read pattern files whose `languages` match the current target and run the detection.
- For each hit, apply the "when buggy" criteria; drop the matches that fall under "false positives."

## Adding to the shared library

Phase 5 may propose new patterns. The skill does **not** silently write to the shared library. Instead:

1. Generate the proposed pattern file in `bug-hunter-out/<project>/<ts>/proposed-patterns/<id>.md`.
2. Tell the user: "I'd like to add this pattern to the shared library; review the file and confirm."
3. Only after the user confirms, copy to the skill's `patterns/` directory.

Reasoning: shared-library writes affect every future run on every project. Bad pattern additions create persistent false-positive noise. Human confirmation is required.

## Privacy considerations for notes

Per-project notes can contain pre-disclosure findings — these are sensitive. The skill should:

- Add `.bug-hunter/` to the user's local `.gitignore` if not present
- Warn the user if their working directory is in a repo and `.bug-hunter/` looks accidentally committed
- Never include private-disclosure draft contents in any output that goes to a public location

The skill never transmits notes anywhere. They live locally.
