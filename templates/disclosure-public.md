# Public disclosure / submission template

Used when project policy permits public submission and severity allows it.

## Mailing-list cover letter (for projects that take patches via email)

```
To: <project-list>
Cc: <maintainers-from-get_maintainer.pl-or-equivalent>
Subject: [PATCH] <subsys>: <one-line description>

<Same body as the patch commit message — see templates/patch.md. The
cover letter is just the patch itself; no separate intro is needed for
a single-patch submission.>

For a multi-patch series, prepend:

  Subject: [PATCH 0/N] <subsys>: <series description>

  Hi <list>,

  This series fixes <summary>. Patches:

    1/N <patch 1 subject>
    2/N <patch 2 subject>
    ...

  The series was tested by <how>. AddressSanitizer reports clean after
  the fix.

  Thanks,
  <Real Name>
```

## GitHub PR (for projects that take PRs)

```
Title: <subsys>: <one-line description>

Body:

  ## What this fixes

  <One paragraph: the bug, where it lives, how it's triggered.>

  ## Why this fix is correct

  <Paragraph explaining the fix, referencing similar code in the project
  that already does it right.>

  ## How this was found / verified

  <One paragraph: discovery method, then how it was confirmed under
  AddressSanitizer.>

  Sanitizer output before the fix:

  \```
  <relevant ASan excerpt>
  \```

  Sanitizer is clean after the fix.

  ## Notes

  <Anything else maintainers should know: related code that could use
  the same fix, follow-up patches that may be needed, etc.>

  Closes <issue if one was filed>
```

## Notes

- Subject line is description, not severity claim. Maintainers downgrade reporters who lead with hype.
- Don't claim CVEs in the public submission.
- Don't pre-emptively reply to your own message — let maintainers respond first.
- If the project requires `Signed-off-by` (DCO), include it. The skill must use the human's real identity, not a placeholder.
- After sending, expect a few days of silence before maintainers respond. Don't bump the thread aggressively.

## When the public submission was preceded by private disclosure

If you've already done a private disclosure (e.g., critical finding, embargo resolved), the public submission should:

- Acknowledge the prior coordination briefly: "This was reported privately on <date> and discussed with <maintainer>; the embargo lifted on <date>."
- Include the CVE if one was assigned during the embargo
- Otherwise look normal — don't dwell on the disclosure process in the patch
