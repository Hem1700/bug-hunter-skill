// PoC template — sanitizer-level proof only.
//
// Hard limit: this file demonstrates that the bug exists by triggering
// a sanitizer diagnostic. It does NOT contain shellcode, ROP chains,
// mitigation bypasses, or any code intended to gain control flow.
// If the bug is exploitable beyond what the sanitizer shows, that
// work happens elsewhere — not in this artifact.

/*
 * Finding: <FIND-N>
 * Target:  <project> @ <commit>
 * Source:  <path/to/source-file.c>:<lines>, function <func_name>
 * Class:   <CWE-N: short description>
 *
 * The function below is copied verbatim from the upstream source
 * (commit <hash>). Surrounding structs/types are minimized so the
 * PoC compiles standalone.
 *
 * Build:
 *   gcc -g -fsanitize=address -fsanitize=undefined -o poc <this-file>
 * Run:
 *   ./poc
 * Expected: sanitizer reports <type> at <function>:<line>.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- minimized type/struct definitions --- */
/* TODO: copy the minimal set of typedefs, #defines, and structs
 * needed for the buggy function to compile. Keep field layouts
 * identical to upstream so the bug reproduces faithfully. */


/* --- verbatim from <source path> --- */
/* TODO: copy the buggy function (and any helpers it directly calls)
 * VERBATIM from upstream. Mark each block with a comment indicating
 * the source file and line range. Do not "clean up" or refactor —
 * faithful copies make the PoC credible to maintainers. */


/* --- driver --- */
int main(void)
{
    /* TODO: allocate the container on the heap (not the stack) so
     * ASan catches OOB cleanly. Size it to end exactly at the
     * boundary the bug crosses, so OOB-by-one is detected. */

    /* TODO: drive the function into the buggy state with the
     * specific input values identified in the candidate's
     * "verify by" plan. */

    /* TODO: call the buggy function. The sanitizer should fire
     * before this returns. If control reaches the next line,
     * the bug did not trigger — investigate. */

    fprintf(stderr, "[poc] returned without sanitizer error (unexpected)\n");
    return 0;
}
