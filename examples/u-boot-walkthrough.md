# Example: U-Boot audit walkthrough

This is a record of a real session that used this methodology. It produced two findings:
1. **FIND-1** — ignored validator return in EFI capsule code (medium, public-submitted)
2. **FIND-2** — OOB read+write in TCP SACK handling (medium-to-high, would have been private-first)

The point of this example is not the specific bugs but the *shape* of the process. Refer back to it when running a new audit to calibrate.

## Phase 1: Orient

- Target: `github.com/u-boot/u-boot`, cloned `--depth=1`, ~38k files, ~40k LoC in `net/`, ~33k LoC in `lib/efi_loader/`.
- License: GPL-2.0 — open, no friction.
- Security policy: `doc/develop/security.rst` — accepts list submissions, security contact is Tom Rini.
- CVE history (web-searched): the 2024 SquashFS/ext4 cluster (CVE-2024-57254 through 57259) was the most informative — six bugs, all in filesystem parsers, all variants of the same alloc-arithmetic and length-handling patterns.
- Attack surface map: EFI loader (modern, large, untrusted boot variables), network stack (remote), filesystem parsers (variant ground), capsule update path.

## Phase 2: Discover

Started with **filesystem parsers** as a variant-analysis target:

```
grep -rnE '(malloc|zalloc|calloc|realloc)\s*\([^)]*[\+\*][^)]*\)' fs/erofs fs/exfat fs/btrfs fs/cramfs fs/jffs2 fs/cbfs
```

Two hits. One was `malloc(CLUSTER_SIZE(...))` — safe. The other was cramfs symlink `malloc(size + 1)`, which *looked* like the SquashFS pattern but turned out to use a 24-bit `size` field that couldn't wrap. **However**, reading further surfaced a different bug in the same function — the decompressor used `avail_out = 4096*2` regardless of destination size. This is the `c-decompressor-unbounded-output` pattern (now in `patterns/`).

User chose to skip cramfs (legacy fs, low blast radius) and pivot.

Pivoted to **EFI loader** with static analysis:

```
cppcheck --enable=warning,portability --inconclusive -q lib/efi_loader/
```

Most hits were format-string false positives. Real candidates:
- `efi_signature.c` signature-list walker — turned out to be well-guarded
- `efi_device_path.c` device-path walker — turned out to have a real entry-point validator (`efi_dp_check_length`)

The interesting question became: is `efi_dp_check_length` called on every untrusted entry point? **Gap audit** time:

```
# who validates
grep -rn "efi_dp_check_length" lib/ cmd/ boot/

# who walks (unguarded)
grep -rn "efi_dp_size\|efi_dp_match\|efi_dp_next" lib/efi_loader/
```

This surfaced `efi_deserialize_load_option` — called from 4 places, but only 3 of them check the return. The outlier was `get_dp_device` in `efi_capsule.c`. Cross-checking the other callers confirmed they all check the return → outlier was the bug.

## Phase 3: Review (interactive)

Presented to the user:

> CAND-1: `get_dp_device` in `efi_capsule.c` ignores `efi_deserialize_load_option` return value. Three other callers check it. Reading the deserializer: on failure it may leave `lo` partially set (with `lo.file_path = data` pointing at unvalidated bytes) or completely uninitialized (if early-bailed). The capsule code then walks `lo.file_path` with `efi_dp_split_file_path` → `efi_dp_size`, which is the `efi_dp_next`-based walker that assumes the path was pre-validated. Reachable from `Boot####` EFI variable, which is untrusted in some threat models.

User approved.

## Phase 4: Confirm

For FIND-1 (capsule), confirmation was logic-trace, not sanitizer:
- The deserializer source clearly shows the early-return paths before `efi_dp_check_length`
- The cross-check on the other callers shows the API contract
- A sanitizer PoC wasn't built; the call-graph trace was deemed sufficient because the bug is a logic flaw in API usage, not a memory-corruption bug whose exploitability needs proof.

In retrospect, building a worked example (feed a malformed Boot variable to the sandbox build, observe the unvalidated walk) would have been stronger. The trace was accepted because the pattern (every-other-caller-checks-this) is a strong signal.

For FIND-2 (`tcp_hole`), confirmation was sanitizer:
- Ported `tcp_hole` and its helpers verbatim into `/home/claude/poc_tcp_hole.c`
- Drove into `cnt=4, i=0, cnt_move=2` state with crafted hill values
- ASan caught READ of size 16, 0 bytes after the 38-byte heap region
- Second PoC for the *insertion* path drove into `cnt=4` full-array state with a non-overlapping hole sorting before the last entry → ASan caught WRITE of size 16

Both PoCs confirmed two facets of the same root cause (wrong shift-count in `memmove`). This is **chaining mode 3** (intra-project variant analysis) — same finding, multiple sites, one fix.

## Phase 5: Produce

FIND-1 (capsule) — public, because severity was medium and project policy accepts list submissions:
- Patch generated with proper `Signed-off-by`, run through `checkpatch.pl` (clean), recipients from `get_maintainer.pl` (the script is `get_maintainer.pl` singular in U-Boot, not `get_maintainers.pl` — small detail, gotcha now recorded in per-project notes).
- Submitted via `git send-email` to `u-boot@lists.denx.de` + Tom Rini + the two EFI maintainers.

FIND-2 (`tcp_hole`) — would default to **private** based on severity-based override (high impact tier + remote), even though U-Boot policy accepts list submissions in general. Patch covers both sites in one diff. Disclosure draft prepared but not yet sent at end of session.

## Calibration notes

Things this session got right:
- Variant analysis on the SquashFS CVEs surfaced a real bug pattern
- Honest demotion of cramfs (correct call — legacy fs)
- Sanitizer PoCs for `tcp_hole` instead of asserting from code reading
- Honest severity calibration (medium-to-high, not "critical RCE")
- Reading project policy → public for medium, private for higher

Things this session could have done better:
- FIND-1 didn't have a sanitizer worked example; the logic trace was the only confirmation. Stronger would have been a sandbox build with crafted Boot variable.
- The static-analyzer pass on the EFI loader produced a lot of noise; better cppcheck configuration (include paths) would have helped filter.

These notes inform `references/confirm.md` (logic-bug confirmation should include a worked example by default) and the per-project notes for U-Boot.
