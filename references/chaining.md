# Chaining

Four distinct modes. They are not alternatives — apply each that fits.

## Mode 1: Data-flow chaining (taint analysis)

The chain is: **untrusted source → series of transformations → dangerous sink**, with no validation in between.

How to detect:
1. Identify untrusted sources in the subsystem (network recv, file read, variable read, IPC). See Phase 2 Technique D.
2. Follow the data through the call graph. When the data is passed as a function argument, recurse into the callee. When stored in a struct field, treat reads of that field as continued taint.
3. Mark sinks (`memcpy`, allocations, array indexing, format strings, control flow on integer values).
4. A path source → sink with no validation in between is a chain.

How to report: name the source, the intermediate functions, the sink. Example:

> *Chain: `nfs_handler` (UDP recv) → `nfs_pkt_recv` (dispatch, no length check) → `nfs_readlink_reply` → `memcpy(&rpc_pkt, pkt, len)` (stack overflow if len > sizeof rpc_pkt). Guard exists in `nfs_handler` at line 86 — chain is broken. Confirmed safe.*

That last sentence is critical — show the guard you found, do not claim the chain is open if it's actually closed.

## Mode 2: Multi-bug exploitable chains

The chain is: **two independent bugs that compose into a worse bug**. Common shapes:

- **Info-leak + memory-safety bug needing a known address.** Bug A leaks a heap pointer to the network peer; bug B is a use-after-free or arbitrary-write-where that needs a known pointer. Alone, A is medium and B is unexploitable; chained, the pair is critical.
- **Length-field corruption + downstream copy.** Bug A lets the attacker influence a length field stored in shared state; bug B uses that length field unchecked elsewhere.
- **State-machine misuse + protected operation.** Bug A drives the state machine into a state it shouldn't reach; bug B allows an operation that's only safe in that state.

How to detect: after each finding is confirmed, search the same audit for findings whose effect could *enable* this one, or whose precondition could be *enabled* by this one. The Phase 2 candidate list is a useful seed — pair up findings that mention complementary primitives.

How to report: explicitly write the chain as a sequence, with each step's prerequisites. Do **not** generate working exploit code for the chain (this is the hard limit). Describe the chain in prose; sanitize the PoC to demonstrate each *constituent* bug, not the composed exploit.

## Mode 3: Variant analysis (intra-project)

The chain is: **same root cause, multiple call sites**.

How to detect: when you confirm a finding, grep for the same pattern in the rest of the codebase. The U-Boot reference work surfaced this: the SACK `memmove` count bug existed in **two** places (the merge path and the insertion path) in the same function. A patch that fixes only one is incomplete.

How to report: list all instances under one finding ID, with one fix patch that addresses all of them. Do not split into multiple disclosures when the bugs share a root cause.

## Mode 4: Pattern variants (cross-project)

The chain is: **same pattern, same project, different subsystems** — or **same pattern across projects** if the shared pattern library has prior instances.

This is the long-running benefit of the shared pattern library. After confirming a finding, propose adding the pattern (anonymized to the underlying CWE shape) to `patterns/`. Future runs on other projects pick it up automatically.

How to report: not a separate finding — instead, it feeds back into the discovery technique. Surface to the user during Phase 5 as "this pattern is now in the shared library; future audits will check for it."

## Avoiding spurious chains

Do not claim a chain exists if:
- The intermediate guard is just one you forgot to check — verify the full path
- The two bugs are in disjoint subsystems with no shared state
- The "composed" effect is the same as the individual effect (e.g., two DoS bugs composing into a "bigger" DoS — not a real chain)

A claimed chain that turns out to be broken costs more credibility than a missed chain. Be conservative.
