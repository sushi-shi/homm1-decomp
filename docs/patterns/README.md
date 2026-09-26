> Imported from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
> This is donor evidence, not a validated HoMM1 VC4 rule. Commands and source
> examples describe that donor. Re-prove applicable mechanisms with VC4.

# Compiler patterns

Historical observations from the Giten/Gruntz MSVC 5.0 SP3 campaigns.
Start with [the index](INDEX.md). These are clues for reading compiler output,
not a catalogue of matching victories or proof of original source.

## Keep it small and falsifiable

- Prefer correcting an existing mechanism over creating another entry.
- Include a recognizable signature, a bounded observation or minimal reproducer,
  and what the observation does **not** establish.
- Separate compiler measurements, historical reports, and source hypotheses.
  A successful example does not establish a universal compiler rule.
- New entries need a distinct reusable mechanism and reproducible evidence.
  Scores, confidence ratings, campaign logs, per-function stop verdicts, and
  lists of failed spellings do not belong here.
- Failed searches do not prove impossibility. Matching bytes do not uniquely
  determine types, scope, source spelling, or ownership.
- Link to canonical tool, build, and lineage documentation instead of copying
  their contracts here. Record HoMM1 validation under `evidence/`.

## Historical material

The former collection mixed useful observations with speculative rules,
contradictions, and stale function state. It was removed, not moved into another
live database. Removal is not a finding that every old observation was false;
equally, an old exact-match claim is not an endorsement of its explanation.

The [pre-cleanup tree](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/) remains available in Git.
Commit-pinned links elsewhere are historical provenance, **not current recipes**.
For local recovery:

```sh
git show b27b05deb:docs/patterns/<old-file>.md
```
