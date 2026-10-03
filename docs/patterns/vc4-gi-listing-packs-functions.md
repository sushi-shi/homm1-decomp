# /Gi with an assembly listing packs functions (HoMM1 VC4, measured)

Measured with the pinned VC4.0 `CL`/`C1XX`/`C2` (MSVC40.iso holds only this
one build of each). It resolves why retail SOURCE carries `/Gi` output but its
functions are packed.

## Observation

Pinned `C2` with `-Gi` gives every function its own `.text` section, aligned
to 16 bytes when the function is compiled for speed. The alignment comes from
the same per-function speed bit (IL option 0x800000, decoded at 0x00422948,
read at 0x0041acef) that selects speed codegen such as
`mov eax,[m]; push eax`. Its size form is `push dword ptr [m]`. Retail SOURCE
uses the speed form, so it can't get packing from that bit. LINK 3.00 respects
section alignment under `/OPT:REF`, `/OPT:NOREF`, `/INCREMENTAL:YES` and
`/DEBUG` alike.

An assembly listing turns the split off. With `/Gi /Zi` plus any of `/Fa`,
`/FAs`, `/FAc`, `/FAcs`, `/Fc` or `/Fl`, C2 emits one `.text` section and one
`.data` section, as in a non-`/Gi` compile. Every C1 effect of `/Gi` remains:

- `_$E2` (the `.CRT$XCU` wrapper) is the object's first function, and the
  next function starts directly after it at +0x15. `_$E1` stays at the global's
  definition. Compiled this way, PHILAI starts `_$E2@0`, `CheckDoMain@0x15`.
  Retail's Misc+PHILAI object is `_$E2` at 0x00419990 with `LogTruncate` at
  0x004199a5, so with Misc merged into the top of PHILAI the layout is
  retail's.
- `__LINE__` is relative to a per-function `__LINE__Var`.
- The C1 handle state shifts. TOWNMGR's 32 functions compile byte-identical
  (relocations masked) with `/Gi /Fa` and with plain `/Gi`.
  `SetArmyCommand`, `SetupCastle` and `RecruitHero` have retail distance 0
  in both.

Running C1XX with `-Gi` and C2 without it gives the same result. The listing
switch is one way a normal `CL` command produces it.

## Reading retail

- SOURCE functions are packed and none is 16-aligned. 28 of its 775
  functions have no caller, so they were not COMDATs, and SOURCE was compiled
  without `/Gy`. This fits `/Od /Zi /Gi` plus a listing switch.
- Every BASE C++ function has a caller (no unreferenced starts besides the
  MASM `_BitClear` and `MoveBitmapArea`). So BASE was compiled with `/Gy`,
  and `/OPT:REF` removed its dead functions. Its 16-byte alignment and `CC`
  fill come from the COMDATs. Its 4-aligned, unpooled literals fit `/Gi`
  without `/Gf`.
- Retail has no debug directory. PE linker version 3.00 is VC4.0's LINK.
  Nothing points to another compiler build.

## Open

Pinned C2 emits file-scope variables and `__LINE__Var` words ahead of all
literals, in every mode. Retail places a function's line word with that
function's data: TOWNMGR's line word 0x0048ed8c follows the literals of the
functions before `BuyBuild`, and RESMGR's four line/`__FILE__` pairs
interleave with its literals. Retail data is in parse order. No pinned flag
set reproduces this yet; tried plain `/Gi`, `/Gi` with listings, incremental
recompiles and C1-only `-Gi`.
