# Retail relocations

HoMM1's pinned HEROES.EXE retains relocation evidence. It does not need Giten's
synthetic `.reloc` build-copy step for a /FIXED DDS.EXE. Raw retail bytes,
RVA/VA conversion, relocation sites and exact import identities are authoritative.

Reviewed function/data referents are under `config/retail`. Source declarations
can add a reviewed name without claiming a body. The synthetic PDB and delinked
objects model these identities; they are not independent retail evidence.

Current code-mode comparison relaxes eligible data-reference symbols/addends,
while function, import and EH targets remain checked. Use raw object/retail
inspection for identities hidden by that transform. Do not copy donor
relocation tables, IAT section assumptions or guessed nearest-symbol ownership.
