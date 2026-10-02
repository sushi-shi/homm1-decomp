# Candidate linking

`homm1 link --dry-run` displays the candidate build; `homm1 link` invokes the
pinned VC4 linker. The contract lives in `scripts/homm1/graph/link.py` and the
manifest, including real libraries, import-library generation and MASM OMF units.
Do not add `/FORCE`, guessed definitions or dummy bodies to obtain an image.

Outputs belong in `build/exe/`. Unresolved definitions are reconstruction
findings. Object matching does not establish final placement or runtime
correctness; use [candidate-image checks](image-diff.md) when linking succeeds.

The vendor import libraries are synthesized by `homm1.graph.implib` from the
retail import table. The reviewed import-thunk rows of
`config/retail/function_referents.tsv` supply what the table cannot: the
caller-side stdcall decoration of WinG's undecorated exports and the names of
smkwai32's ordinal-only imports. Hints and ordinals are re-read from each
produced library and checked against retail.

Without `--order`, objects are linked in retail code order: each unit sorts by
its lowest claimed function RVA (dynamic-initializer pins excluded), because
LINK lays out `.text` in object order. `--manifest-order` keeps the given order.
Units from the BASE run (0x00473450 onward) are archived into `base.lib` and
searched after `netapi32.lib`, as retail's `Netbios` thunk at 0x0047343c
requires. Their order is LINK's pull order. `--no-base-library` links them as
explicit objects. `config/heroes.def` supplies the export directory, module
name and stack reserve. There is no `/ENTRY`, and retail's `/OPT:REF` is the
default (`--keep-all` restores `/OPT:NOREF`). See
[link layout](../evidence/link-layout.md). When a link fails, the full log and
the decorated unresolved list stay in `build/exe/`.
