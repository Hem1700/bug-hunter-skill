# Phase 1: Orient

Goal: build a working mental model of the target before hunting. A bad orient phase wastes the rest of the run.

## Steps (run in order)

### 1. Establish the target

Confirm the user's target. Acceptable forms: a local directory, a git URL, or a project name + path the human has already cloned. If a URL was given, clone with `--depth=1` initially (you can `--unshallow` later if you need `Fixes:` tags or history).

Record:
- Target path: `<dir>`
- Primary language(s): use file-extension census (`find . -type f -name '*.c' | wc -l` etc.)
- Build system: `Makefile`, `configure`, `CMakeLists.txt`, `Cargo.toml`, `setup.py`, `Kconfig`, …
- License: read `LICENSE`/`COPYING`/SPDX headers — surface to user, do not gate on it.
- Latest commit (`git log -1 --format='%H %ci %s'`) — pin the audit to a specific tree.

### 2. Find the security policy

This drives the disclosure default later. Check, in order:

```
SECURITY.md
.github/SECURITY.md
docs/security*
doc/develop/security*       # u-boot
Documentation/admin-guide/security-bugs.rst   # linux
```

Extract: security contact email(s), supported versions, embargo policy, CVE handling, whether public PRs are acceptable for security bugs.

Store the findings in the per-project notes (see `memory.md`). If none found, fall back to severity-based defaults documented in `disclosure.md`.

### 3. Pull CVE history

Variant analysis is the highest-yield technique. Get a list of past CVEs:

- `web_search` for "<project> CVE site:nvd.nist.gov" and "<project> security advisory" — pick the recent ones (last 3-5 years are most relevant; older bugs are often in code that has since been rewritten).
- For each CVE, capture: CWE class, affected subsystem, affected file(s), one-line description, fix commit if available.
- Note any CVEs that affect parsers similar to the ones in this codebase but in *other* projects (cross-project variant analysis seeds, e.g. "this U-Boot has an ext4 parser and ext4 has had off-by-one CVEs in other firmware too").

Save to per-project notes as `cve_history`.

### 4. Load per-project notes

If `.bug-hunter/<project>.json` exists from a prior run, load:
- Areas previously confirmed hardened (do not re-audit these unless they've changed since the noted commit)
- Open candidates that were deferred
- Project-specific gotchas the human flagged

See `memory.md` for the schema.

### 5. Inventory the attack surface

Build a ranked list of subsystems by potential interest. Score on three dimensions:

- **Reachability** of input: remote network > network local > on-disk untrusted > local-physical > local-trusted
- **Complexity** as a proxy for likelihood of bugs: lines of code, number of distinct file formats parsed, number of state transitions
- **Recency**: new subsystems are less audited than old ones (the EFI loader and TCP stack in U-Boot were both newer than the bootp code that's been picked over for decades)

Useful queries (adjust for language):
```
find <subsys> -name '*.c' | xargs wc -l | sort -rn | head
grep -rnl '\(memcpy\|memmove\|strcpy\|strcat\|sprintf\|malloc\|alloc\)\b' <subsys>
```

Subsystems to look for in any C project:
- Network protocol parsers (highest priority for remote bugs)
- File-format / image parsers (filesystems, executables, archives, media)
- IPC / message decoders
- Configuration / variable / environment parsers
- Cryptographic verification paths
- Bootloader / firmware-update / capsule paths
- Argument / command-line parsing if reachable from untrusted input

Produce: a short "attack-surface map" with 5–10 subsystems ranked.

### 6. Check tooling availability

Before discover phase, verify the tools you'll need:

```
which cppcheck clang clang-tidy semgrep rg
echo | clang -fsanitize=address -x c - -o /dev/null 2>&1
```

If something's missing, attempt to install it (`apt-get`, `pip`) or note it as unavailable and degrade. Document what you have in the notes.

## Output of Phase 1

A short orient summary printed to the user containing:
- Target + commit
- Security policy summary (or "none found, will default to ...")
- Top 3-5 past CVEs and their pattern
- Attack-surface map (5-10 subsystems, ranked, with one-line rationale each)
- Available tools

Then hand control back: "Phase 1 complete. Ready to discover. Proceed with subsystem [top pick] or specify another?"
