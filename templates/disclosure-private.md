# Private disclosure template

Used when project policy or severity dictates private-first disclosure.

```
To: <security-contact-from-policy>
Cc: <additional-maintainers-if-policy-says-so>
Subject: [security] <project>: <subsystem>: <brief description, no severity claim in subject>

Hi <name or maintainer team>,

I'd like to report a <bug class> in <project> that I believe warrants
private handling. Short summary:

<One paragraph: what the bug is, where it lives, and what the trigger
is. Plain English. State severity claim *with* its evidence in the next
section, not in the subject or this paragraph.>

# Affected version

<project> at commit <hash> (<short ref like "master as of YYYY-MM-DD">).
The bug appears to be present since commit <hash> when <introducing
change> was added.

# Vulnerability details

<2-4 paragraphs walking through the defect. Quote the offending lines.
Be precise — name the function, the variable, the missing check.>

# Severity

<level> — <2-3 sentences naming the specific attributes that put it
at that level. Distinguish demonstrated impact from plausible-but-not-
demonstrated impact. Do not inflate.>

Demonstrated:
  <e.g., "OOB write of 16 bytes onto adjacent in-object struct fields,
  reachable from a malicious TCP peer">

Plausible but not shown:
  <e.g., "potential information disclosure to the remote peer via the
  outgoing SACK options containing the OOB-read bytes">

Not demonstrated:
  <e.g., "no control-flow hijack primitive established">

# Reproduction

I'm attaching a standalone sanitizer-level PoC (poc.c). To reproduce:

  gcc -g -fsanitize=address -o poc poc.c
  ./poc

AddressSanitizer reports:

  <paste the relevant 5-10 lines of ASan output>

# Suggested fix

I'm attaching a patch (fix.patch) that <one-sentence summary of the
fix>. The fix matches the pattern used by <similar/sibling code> in
the same codebase: <reference>.

# Next steps

Please let me know:

  - Receipt acknowledgment
  - Whether you'd like to coordinate an embargo period; I'm happy to
    hold public disclosure until you're ready
  - Whether you'd like me to request a CVE or you handle CVE assignment
  - Any additional information or testing that would help

I'm reachable at this email and happy to discuss further.

Thanks,
<Real Name>
```

## Notes on filling this in

- Use the human's real name and email. If not configured, ask.
- Use only contacts found in the project's actual policy / docs. Do not invent.
- Keep the subject line opaque enough that an inbox preview doesn't leak the bug — many security mailboxes prefer this.
- Attach the PoC and patch as separate files; do not paste lengthy code into the body.
- Do not propose an embargo timeline; ask the project what they want.
- Do not pre-assign a CVE; let the project handle assignment.
