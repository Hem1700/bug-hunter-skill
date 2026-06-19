# Disclosure

Default rule: **the project's policy decides, the severity gates the workflow.**

## Reading project policy

In Phase 1, the orient step locates the project's security policy. Extract these fields:

- **Contact**: security email(s), or "use the public list," or a Bugzilla/HackerOne/etc. endpoint
- **Embargo**: does the project request an embargo period? How long?
- **Scope**: what classes of bugs they want reported privately vs publicly
- **CVE**: do they handle CVE assignment, or do they expect the reporter to request one?
- **Format**: what format do they want the report in (free-form email, structured template, JSON)?

Save these to per-project notes so future runs reuse them.

## Default selection logic

For each confirmed finding, choose between **private** and **public** disclosure as follows:

1. If the project's policy explicitly says "report all security bugs privately first," → **private**, regardless of severity.
2. If the project's policy explicitly accepts public reports on the mailing list / tracker, → check severity:
   - Critical or High → **private** (override leniency; serious bugs deserve private-first regardless of project laxity)
   - Medium or Low → **public** is acceptable; choose per project norm
3. If no policy was found:
   - Critical or High → **private**, with a polite message to the contact you can find (top maintainers from `git shortlog`, or "security@" if such address exists on the project's domain)
   - Medium or Low → **public** in the project's normal submission channel
4. The user can override either way. If they request public when the rule says private, push back once with the rationale, then comply if they insist.

Document the choice and the reason in `reports/<finding-id>.md`.

## Severity gating (cross-references severity.md)

For findings rated **critical** (per the rubric in `severity.md`) where the policy or default says private:

- Generate the disclosure draft *before* the patch.
- Place the patch in `patches/private/` rather than `patches/`.
- In `reports/SUMMARY.md`, mark the finding with `[REQUIRES PRIVATE DISCLOSURE]`.
- Print a clear note to the user: "Critical finding — send the private disclosure first. The public patch will be ready when you are."

This is the "let project policy decide" behavior agreed to in design — different projects get different defaults, and the user can always override.

## Private disclosure draft

Use `templates/disclosure-private.md`. The draft should:

- Be addressed to the contact extracted from policy (not invented)
- Lead with a one-sentence summary so the reader knows whether to read further
- Include the affected component, version/commit pinned
- State severity claim with the *evidence* (sanitizer output) — do not just assert "critical"
- Attach (reference) the PoC and the patch
- Ask explicitly: "please acknowledge receipt; I'm willing to discuss embargo and coordinated disclosure timeline"
- **Do not** include any embargo timer in the first message — that's the project's call to set
- Sign off with the human's real identity

The skill must never send the email itself. Output is a draft for the human to send.

## Public disclosure / patch submission

Use `templates/disclosure-public.md` for the cover letter, and the project's normal patch format. Refer to Phase 5's patch generation.

For mailing-list projects: produce a `git send-email` invocation as part of the output, with the recipient list filled in from `scripts/get_maintainer.pl` (or equivalent) output.

For GitHub-PR projects: produce the branch + commit message; the human runs `gh pr create` themselves.

## What to never do

- Never email anything yourself. The skill produces drafts.
- Never invent maintainer addresses. Use what's documented or `git log`-derived; if uncertain, leave a placeholder.
- Never promise a CVE to maintainers — CVE assignment is theirs, the reporter's, or a CNA's, depending on policy; do not pre-empt.
- Never include the project's name in the *subject line* of a private disclosure if the policy says to omit it (some projects' security mailboxes ask for opaque subjects). Read the policy.
