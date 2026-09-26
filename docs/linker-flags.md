# Candidate linking

`homm1 link --dry-run` displays the candidate build; `homm1 link` invokes the
pinned VC4 linker. The contract lives in `scripts/homm1/graph/link.py` and the
manifest, including real libraries, import-library generation and MASM OMF units.
Do not add `/FORCE`, guessed definitions or dummy bodies to obtain an image.

Outputs belong in `build/exe/`. Unresolved definitions are reconstruction
findings. Object matching does not establish final placement or runtime
correctness; use [candidate-image checks](image-diff.md) when linking succeeds.
