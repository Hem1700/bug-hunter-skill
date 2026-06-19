# Severity rubric

Severity is the single most over-claimed thing in security reports. Maintainers smell hype from the subject line. Use this rubric and stick to it.

## The five-level scale

**Critical** — All of:
- Remote and unauthenticated *or* triggered by a file the target normally accepts from an untrusted source
- Memory corruption (OOB write, UAF, type confusion) *or* demonstrated arbitrary control of program counter
- Confirmed via sanitizer or working PoC

**High** — Any of:
- Remote OOB read of memory that meaningfully helps an attacker (sensitive data, ASLR-relevant pointers)
- Local-privilege memory corruption in a trust-boundary-crossing context (e.g., firmware, kernel)
- Authentication / cryptographic bypass

**Medium** — Any of:
- Remote denial-of-service or hang
- Memory corruption only triggerable from a higher-privilege context
- Information disclosure of non-sensitive memory
- Logic bug that lets an attacker bypass an intended check but not gain code execution

**Low** — Any of:
- Local denial-of-service
- Memory bug requiring contrived inputs unlikely in practice
- Code quality issues that *could* become exploitable after future refactors

**Info** — Findings worth recording but not strictly bugs:
- Hardening opportunities
- Confusing code that's correct but easy to break

## What demotes a "looks-critical" finding

Be honest about each of these:

- **Adjacent fields, not adjacent objects.** OOB into a neighboring struct field is impact-bounded; OOB into a neighboring heap allocation is much worse. Check which one you actually have.
- **Read vs write.** OOB reads are info-leaks and DoS; OOB writes are corruption. Treat them differently.
- **Reachability gates.** "Triggered by attacker-controlled file" loses severity if there's an upstream signature check that runs first.
- **Demonstrated impact.** Sanitizer caught it ≠ exploitable. If you don't have a working primitive (you shouldn't, per the hard limit), don't claim one.

The U-Boot `tcp_hole` finding from the reference session is **medium-to-high**, not critical, because:
- The OOB lands on adjacent in-object fields, not a neighboring allocation
- No demonstrated control-flow hijack
- But remote + unauthenticated + write tier elevates it above plain medium

That kind of calibration is the model.

## What inflates a finding incorrectly

Avoid these failure modes:

- Calling something "RCE-class" because the *bug class* is associated with RCE in textbooks, without showing the path here
- Citing "could escalate to" without saying what the escalation primitive would be
- Adding "critical" to the report subject because it sounds more urgent — maintainers downgrade for the next finding from you when this happens
- Reporting denial-of-service in code that already requires high privilege as a high-severity bug

## Reporting severity

In the report, include:

- The level (one word)
- The reasoning, in 2–3 sentences naming the *specific* attributes that put it at that level
- Honest caveats: what you have not demonstrated

Example (from `tcp_hole`):
> Severity: **medium-to-high**. Remote and unauthenticated (any peer in a TCP connection can drive the SACK state), and produces an OOB write — those raise it above plain medium. However, the write lands on adjacent in-object fields (`retry_cnt`, `retry_timeout`) rather than past the heap allocation, and no control-flow hijack is demonstrated; this keeps it below "critical." Exploitability beyond information disclosure, state corruption, and denial of boot is not established.
