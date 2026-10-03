# Candidate linking

`homm1 link --dry-run` displays the candidate build; `homm1 link` invokes the
pinned VC4 linker. The contract lives in `scripts/homm1/graph/link.py` and the
manifest, including real libraries, import-library generation and MASM OMF units.
Do not add `/FORCE`, guessed definitions or dummy bodies to obtain an image.

Outputs belong in `build/exe/`. Unresolved definitions are reconstruction
findings. Object matching does not establish final placement or runtime
correctness; use [candidate-image checks](image-diff.md) when linking succeeds.

Vendor import libraries are synthesized from the pinned retail import table;
produced hints and ordinals are checked against it. 1.2 uses `MSS32.DLL` and
`SMACKW32.DLL`, replacing `WAIL32.DLL` and `SMKWAI32.DLL`. The new Smacker
calls use direct dllimport IAT references. WinG retains its reviewed import names and calling conventions.

Objects are linked in retail code order, using each unit's first claimed
function (excluding dynamic initializers). BASE begins at RVA `0x73200`,
after the NetBIOS/WinG thunks, and is archived into `base.lib`, searched after
`mss32.lib`. `/NODEFAULTLIB:libc.lib` selects VC4.1 `LIBCMT.LIB`.
`/STACK:0x10240,0x1000` preserves retail's reserve/commit. No `/DEF` is passed:
1.2 has no exports, and even an empty named module definition makes LINK
create an unwanted export directory. `config/heroes.def` records only the
historical stack contract. No `/ENTRY` or `/FORCE` override is used.

## Resources

`src/SOURCE/Heroes.rc` holds all seven retail resource payloads: the icon,
the `HEROES` About dialog and the menus `MNUADV`, `MNUDFLT`, `MNUCMBT` and
`MNUTOWN`, in retail payload order. The `rc` edge compiles it with the pinned
VC4 `RC.EXE` to `build/gen/heroes.res`. The icon is rebuilt from the user's
retail image in a temporary stage, and every payload is compared with retail
(report: `build/gen/heroes.res.json`). LINK converts the `.res` with
`CVTRES.EXE`, which it finds on the wine `PATH` beside `LINK.EXE`.

The VC4.1 release includes the pinned resource tools; `homm1 toolchain check`
verifies them. All seven 1.2 resource payloads equal 1.1, including the About
dialog's unchanged “Version 1.1” text.

## Historical VC4.0 per-DLL import-order investigation

The candidate `.idata` has retail's descriptors, hints, RVAs and per-DLL counts,
but within GDI32, WinG and some other DLLs the ILT/IAT entries are a
permutation of retail's (WINMM, ADVAPI32 and NETAPI32 match). The order is not
first-reference order in `.text`, nor the import library's (alphabetical)
member order.

Measured with LINK 3.00 on one `/Od` C object taking the address of n GDI
functions through `windows.h` dllimport declarations, `/NODEFAULTLIB`:

- The ILT follows the object's COFF symbol-table order of the `__imp_`
  externals; C1 emits these in reverse source order.
- For 2 to 7 imports the first entry moves to the end (3 to 8 contributions
  with the null thunk); from 8 to at least 40 the order is unchanged. This is
  consistent with an unstable sort of the `.idata$4`/`$5` contributions whose
  small-partition path is a selection-style short sort with cutoff 8, as in
  the MS CRT `qsort`.
- Several objects concatenate their `__imp_` symbol tables in link order before
  the same rule applies; library position does not matter.

The full link does not reduce to this per-object model. The candidate's GDI32
symbols come from `wingraph.obj` (10), `kbwin.obj` (`GdiSetBatchLimit`) and
`base:MOUSEMGR.obj` (`CreateBitmapIndirect`, `DeleteObject`). Retail is
`DeleteObject GetDeviceCaps GdiSetBatchLimit`, then wingraph's block in
symbol-table order, then `CreateBitmapIndirect`, which implies an earlier
object referencing those three in that order; retail's WinG order
(`WinGCreateBitmap` first) likewise implies a different producer order. The
sort key over all DLLs' contributions probably includes the DLL. Relinking from
`build/exe/HEROESW.candidate.objs.rsp` takes about 6 seconds.
