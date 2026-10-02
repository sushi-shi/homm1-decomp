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
When a link fails, the full log and the decorated unresolved list stay in
`build/exe/`.
