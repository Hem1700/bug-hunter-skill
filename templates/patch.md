# Patch template

The patch must be in the format the target project actually uses. Default below is the kernel/U-Boot mailing-list style (most common for low-level C projects). For GitHub-PR-centric projects, drop the `From`/`Date`/`Signed-off-by` mail headers and use a normal commit.

## Mailing-list style (default)

```
From <commit-hash> <date>
From: <Real Name> <real-email>
Date: <date>
Subject: [PATCH] <subsystem>: <one-line description>

<Body paragraph 1: what the bug is — describe the defect in code terms,
not severity terms. State the problematic call/path and what's wrong with it.>

<Body paragraph 2: how it's reached — name the entry point, the data flow
from untrusted input to the buggy code, any preconditions.>

<Body paragraph 3: what the impact is — be specific. "OOB write of N bytes
into the adjacent struct fields X and Y" not "memory corruption."
Cite the sanitizer output if relevant.>

<Body paragraph 4: what the fix does and why it's correct. Reference the
pattern (e.g., "every other caller of this validator already checks the
return value") to show the fix is consistent with the codebase.>

Fixes: <12-char hash> ("<original commit subject>")
Reported-by: <reporter, if different from author>
Signed-off-by: <Real Name> <real-email>
---
 path/to/file.c | X +++++++--
 1 file changed, X insertions(+), Y deletions(-)

diff --git a/path/to/file.c b/path/to/file.c
...
```

## Subject-line conventions

Look at recent commits in the project (`git log --oneline -50 -- <subsys>`) to pick the right prefix. Common conventions:

- Linux kernel: `<subsys>: <verb> <noun>` — e.g., `net/tcp: fix ...`
- U-Boot: same style — e.g., `efi_loader: check ...`
- Most other C projects: similar

The subject should be a *description*, not a marketing claim. "fix OOB write in SACK array compaction" is good; "CRITICAL: prevent remote heap corruption" is not.

## Notes on the body

- **Do not include CVE numbers** unless one has been assigned. Saying "this would be CVE-class X" is fine in the discussion; pre-emptively claiming a CVE is not.
- **Cite the sanitizer output** in the commit message if it's clean and short. Example:
  ```
  AddressSanitizer reports:
    READ of size 16 at <addr>, 0 bytes after 38-byte heap region
    #1 tcp_hole net/tcp.c:728
  ```
- **Be honest about impact bounds.** "OOB write of one struct entry, lands on retransmission state fields" is more useful (and more credible) than "potentially exploitable."

## Multi-site fixes

If a finding has multiple call sites (same root cause, several locations — see chaining mode 3), put them in one patch unless they're in unrelated subsystems. The commit message should enumerate the sites.

## Series patches

If the work requires multiple commits (e.g., a refactor first, then the fix), use `git format-patch -<N>` and number them `<NN>/<N>` in subjects. Add a `[PATCH 0/N]` cover letter explaining the series.

## Things to check before finalizing

- Author name and email match the human's `git config user.name`/`user.email`
- `Signed-off-by` matches author
- `./scripts/checkpatch.pl` (or project equivalent) passes
- `git apply --check` against an unmodified tree applies cleanly
- The patch builds the affected code (sandbox/userspace build of the touched code)
