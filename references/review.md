# Phase 3: Human review

Goal: hand off to the human reviewer cleanly so they can pick what to pursue. This phase is short by design; its purpose is to make the human's job easy.

## What to present

Show the candidate list from Phase 2, but enriched with:
- One paragraph of plain-English explanation per candidate (not just the technical grep hit)
- The *strongest evidence* for the candidate, quoted directly from the source with line numbers
- The *strongest counter-evidence* you found while filtering (e.g., "this looks unbounded, but there's a guard at line X — I think the guard is insufficient because <reason>; the human should sanity-check")
- Explicit "I am uncertain about ..." notes where they apply

## What not to do

- Do not pre-decide for the human. Present, don't sell.
- Do not hide weak candidates if they passed your filter — let the human eliminate them.
- Do not skip candidates that are "only" DoS or info-leak; the human chooses severity tolerance.
- Do not generate patches or PoCs yet. Phase 4 needs the human's go-ahead.

## Asking the human

After presenting, ask one direct question with the candidate identifiers as options. The user picks one or more to advance to Phase 4. If the user wants to skip review entirely and just confirm everything, that's their call — but at least one round of "any of these look off?" before the confirm phase saves wasted work on misreads.

## When the human pushes back

If the human says a candidate is wrong, ask them why before moving on. Two outcomes matter:

1. **They have project context you didn't have.** Update per-project notes (`memory.md`) so future runs avoid the same misread. Example: "this struct is always heap-allocated with a 16-byte trailing pad in the U-Boot allocator, so the apparent OOB is in-allocation." Record that.

2. **They're guessing.** Push back politely with the evidence you have. If still disputed, demote the candidate but keep it as an "unconfirmed lead" in the notes rather than deleting it.

## Output of Phase 3

A list of *selected* candidates to advance to confirmation, plus any updates to per-project notes from the review conversation.
