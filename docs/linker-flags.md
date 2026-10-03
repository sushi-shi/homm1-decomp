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

## Per-DLL import order (open; measured on `work/n2-exe`)

At master 3e819d4 the candidate `.idata` has the same descriptors, hints, RVAs
and per-DLL counts as retail, but 547 bytes still differ. Within each DLL the
ILT/IAT entries are a permutation of retail's. WINMM, ADVAPI32 and NETAPI32
already match. The order is not first-reference order in `.text`. It is not the
import library's member order either: `gdi32.lib` members and linker-member
symbols are alphabetical.

### Measurements with LINK 3.00

Test setup: one C object with `/Od` that takes the address of n GDI functions
through `windows.h` dllimport declarations, linked with `/NODEFAULTLIB`.

- **Symbol-table order.** The ILT follows the object's COFF symbol-table order
  of the `__imp_` externals. C1 emits these in reverse source order.
- **Small DLLs rotate by one.** For 2 to 7 imports the first entry moves to
  the end. With the null thunk, that makes 3 to 8 contributions per DLL.
- **No rotation from 8 imports.** From 8 up to at least 40 imports the order
  is unchanged.
- **Interpretation.** This is consistent with an unstable sort of the
  `.idata$4`/`$5` contributions whose small-partition path is a
  selection-style short sort with a cutoff of 8, as in the MS CRT `qsort`.
- **Several objects.** The `__imp_` symbol tables concatenate in link order,
  then the same rule applies. Swapping the two objects swaps their blocks.
- **Library position.** Placing `gdi32.lib` before or after the objects does
  not change the order.

### The full link does not reduce to this model

The candidate's GDI32 symbols come from three objects:

| Object | GDI32 references |
|---|---|
| `wingraph.obj` | 10 |
| `kbwin.obj` | `GdiSetBatchLimit` |
| `base:MOUSEMGR.obj` | `CreateBitmapIndirect`, `DeleteObject` |

The candidate gives `GdiSetBatchLimit GetDeviceCaps DeleteDC DeleteObject ...
CreateBitmapIndirect AnimatePalette`. That is wingraph's block reversed, with
`AnimatePalette` placed last.

Relinking with `wingraph.obj` moved to the end of the object list gives
`GetDeviceCaps DeleteDC AnimatePalette CreatePalette ... GdiSetBatchLimit
CreateBitmapIndirect DeleteObject`. The symbol shared with MOUSEMGR,
`DeleteObject`, now lands last.

Retail is `DeleteObject GetDeviceCaps GdiSetBatchLimit` followed by
wingraph's block in symbol-table order (`GetSystemPaletteEntries ...
CreatePalette AnimatePalette DeleteDC`) and then `CreateBitmapIndirect`. This
suggests that some object linked before wingraph references `DeleteObject`,
`GetDeviceCaps` and `GdiSetBatchLimit` in that symbol-table order. Retail's
WinG order (`WinGCreateBitmap` first) likewise implies a different producer
order.

### Next step if this resumes

Model the global sort over all DLLs' `.idata$4`/`$5` contributions, since the
sort key probably includes the DLL. Relinking the candidate takes about 6
seconds from `build/exe/HEROES.candidate.objs.rsp`, which makes it cheap to
test.
