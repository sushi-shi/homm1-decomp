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
produced library and checked against retail. `config/retail/import_libraries.tsv`
records a vendor library whose archive shape differs from VC4's: WING32.lib
is built with the pinned VC 2.0 LINK (`homm1 toolchain install --id vc20`),
whose import format carries its own null descriptor, and its members are
renamed `wing32.dll`. Without vc20 the tool warns and uses VC4's format.

Without `--order`, objects are linked in retail code order: each unit sorts by
its lowest claimed function RVA (dynamic-initializer pins excluded), because
LINK lays out `.text` in object order. `--manifest-order` keeps the given order.
Units from the BASE run (0x00473450 onward) are archived into `base.lib` and
searched after `netapi32.lib`, as retail's `Netbios` thunk at 0x0047343c
requires, and after `wail32.lib`, which retail pulls only in the second pass.
Their order is LINK's pull order. `--no-base-library` links them as
explicit objects. `config/heroes.def` supplies the export directory, module
name and stack reserve. The C runtime is VC4.0 LIBCMT: `/NODEFAULTLIB:libc.lib`
drops the LIBC the objects request, and `libcmt.lib` follows the Win32
libraries. There is no `/ENTRY`, and retail's `/OPT:REF` is the
default (`--keep-all` restores `/OPT:NOREF`): only 17 import jump thunks
survive in retail's game band. When a link fails, the full log and
the decorated unresolved list stay in `build/exe/`.

## Resources

`src/SOURCE/Heroes.rc` holds all seven retail resource payloads: the icon,
the `HEROES` About dialog and the menus `MNUADV`, `MNUDFLT`, `MNUCMBT` and
`MNUTOWN`, in retail payload order. The `rc` edge compiles it with the pinned
VC4 `RC.EXE` to `build/gen/heroes.res`. The icon is rebuilt from the user's
retail image in a temporary stage, and every payload is compared with retail
(report: `build/gen/heroes.res.json`). LINK converts the `.res` with
`CVTRES.EXE`, which it finds on the wine `PATH` beside `LINK.EXE`.

The edge exists only when the vc40 `resource_files` are installed. The
compiler release bundle does not carry them, so run
`homm1 toolchain install --id vc40 --media build/downloads/MSVC40.iso`.
`homm1 toolchain check` reports their state.
