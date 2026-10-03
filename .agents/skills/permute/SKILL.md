---
name: permute
description: Run classified HoMM1 permutation campaigns as bounded N-island/M-frontier approximation searches, then inspect diverse high-scoring compiler states and translate their clues into authentic source changes. Use when a reconstructed function is complete but below 100%, when asked to search for higher fuzzy states, when several walls need permutation-candidate classification, or when an exact disposable TU-state result must be understood rather than copied.
---

Adapted from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
Use HoMM1's pinned VC4 profile and absolute `VA(...)` source annotations.
The active score is code first (`data_matching=false`); data identity/addend
coverage is deferred. `AGENTS.md` and the user's instructions take precedence.


# Permute

Use permutation as an evidence loop after reconstruction and wall classification.
The objective remains exact retail structure, not the highest fuzzy spelling.
Never retain generated declarations, fake locals, volatile carriers, or unexplained
source distortions.

## Establish the live population

Work in `nix develop .#build`. Derive candidates rather than keeping a manual list:

```sh
homm1 build
homm1 permute candidates --output /tmp/permute-candidates.json
```

The command classifies normalized base/retail pairs by their first divergence.
Exclude EH funclets unless explicitly investigating EH. Public permutation rejects historical-100 functions. Use high-current unproven
functions for bounded pattern discovery. This campaign priority does not replace the low-HIST
order used for canonical reconstruction.

## Run N islands and retain M distinct states

Start with 32 islands and a four-state frontier; specify RVAs when auditing a
chosen band:

```sh
homm1 permute campaign --rva <target-rva> --islands 32 --frontier 4 \
  --output build/permute/target
```

For several candidates, repeat `--rva` or use `--targets N`. Increase islands or
source depth only after inspecting the previous frontier. The campaign crosses
deterministic compiler-state islands with class-appropriate, semantics-preserving
AST shapes. It continues after an exact result so M useful alternatives can be
retained.

Treat states as different only when their normalized target instruction and
ordered-relocation identities differ. Different probe text that produces the
same state is one solution. Ranking is fuzzy first, then retail size and
relocation-count distance; topology remains a separate structural clue.

## Inspect the frontier

Read `campaign.json`, each result's `frontier/frontier.json`, `retail.asm`, and
the retained candidate source/assembly. For each of the best three or four
distinct states:

1. Find its first real divergence from retail.
2. Compare what moved across the frontier: call set, branch skeleton, operand
   order, live range, stack slot, register, or relocation identity.
3. Inspect callers, callees, types, storage, and the authored function before
   proposing a source explanation.
4. State the reusable hypothesis in source terms, such as corrected ownership,
   lifetime, declaration order, first post-call use, control-flow shape, or an
   inlining boundary.

Route a referent frontier back to identity evidence. Route call-set differences
to inline reconstruction. Route branch/return changes to CFG reconstruction.
Use permutation directly for a proven regalloc/scheduling residue. Follow the
`wall-identifier` and `matcher` skills for those investigations.

## Apply one source A/B and repeat

Implement only an evidence-backed, semantically defensible source change. Never
copy an `exact-disposable.cpp` TU-state forest into source. Check the authored
change with `homm1 match <unit>`, then rerun the campaign so the next frontier
is conditioned on the improved reconstruction.

An exact candidate closes the search only when score, extent, full decoding, and
ordered relocation identity all pass. A sub-100 improvement is a clue, not a
commit criterion. Keep correct modeling changes even if unrelated current fuzzy
moves; the MAX gate (`homm1 verify check`) judges them at merge preparation.

Matching is checked by compilation and comparison. Run `homm1 build` after
source, claim, compiler, comparison or tooling changes.
Consolidate a genuinely reusable compiler mechanism under the admission rules
in `docs/patterns/README.md`; do not add campaign logs or per-function closure
entries. Commit tooling, documentation/skill work, and reconstructed source
changes in separate focused batches. See `docs/permuter.md` for command details
and artifact contracts.
