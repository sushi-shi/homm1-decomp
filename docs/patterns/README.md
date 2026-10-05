# Compiler patterns

Reusable observations about the compilers this reconstruction uses: VC4.0 for
the NWC Win95 builds, [VC4.1](vc41-win95-1997.md) for Win95 1.2, and VC6 SP5
for the Buka build. Each note states the compiler it was measured on. Notes
headed as unmeasured hypotheses describe MSVC 5.0 behaviour that has not been
re-measured on this target's compilers.
Start with [the index](INDEX.md). These are clues for reading compiler output,
not a catalogue of matching victories or proof of original source.

## Keep it small and falsifiable

- Prefer correcting an existing mechanism over creating another entry.
- Include a recognizable signature, a bounded observation or minimal reproducer,
  and what the observation does **not** establish.
- Separate compiler measurements from source hypotheses.
  A successful example does not establish a universal compiler rule.
- New entries need a distinct reusable mechanism and reproducible evidence.
  Scores, confidence ratings, campaign logs, per-function stop verdicts, and
  lists of failed spellings do not belong here.
- Failed searches do not prove impossibility. Matching bytes do not uniquely
  determine types, scope, source spelling, or ownership.
- Link to canonical tool, build, and lineage documentation instead of copying
  their contracts here. Record HoMM1 validation in the pattern note itself.
