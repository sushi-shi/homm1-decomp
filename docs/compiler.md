# Compiler bring-up and first matched fragment

The working compiler is **Visual C++ 4.0, compiler 10.00.5270**, with the
profile `/nologo /c /Od /Z7`. `/Z7` supplies measured COFF function extents. `AppAbout` at VA `0x0045C15C`, RVA `0x0005C15C`,
matches 144 bytes, including both resolved relocation operands. The original
source file/TU name is unknown; `src/SOURCE/AppAbout.cpp` is a reconstruction
fragment named after the verified PE export, not an inferred original TU.

## Reproduce

From the repository root:

```sh
nix develop .#build
homm1 init --exe /path/to/HEROES.EXE
homm1 toolchain install --id vc40 --media /path/to/MSVC40.iso
homm1 toolchain check --id vc40
homm1 build
objdiff-cli diff -p build/objdiff -u app_about -o /tmp/app_about.json
```

The original Microsoft media used here is preserved as
[`MSVC40.iso` in the MSVC4x archive](https://archive.org/details/msvc4x).
Its SHA-256 is
`961326efbfbd299794e2cbb102e9ff3bfe78ebf91a11b89978095f59e0aea93e`.
`config/toolchains.json` is authoritative for media and component hashes.
Provisioning extracts pinned compiler passes, diagnostics, linker/PDB support,
native compiler runtime, SDK headers and CRT/import libraries. VC4 currently
has 354 pinned files. SDK filenames are installed in lowercase for native
Clang analysis; Wine's compiler uses the same verified bytes.

`homm1 probe --contracts` checks the compiler ABI, enum storage and class-layout
fixtures, source-to-object names, and code parity with/without `/Z7`. This is
compiler evidence; the `/GX` fixture does not set a game-wide exception profile.

There is no automatic network download in `init` or `build`. Supply the media
locally; all installed files, Wine state, extracted target objects and reports
stay under ignored `build/`. Compiler execution always uses this repository's
Wine prefix. Inherited `CL`, `_CL_`, `INCLUDE` and `LIB` cannot alter the profile.

## What identifies the compiler, and what does not

The bootstrap callback source was tested using original Microsoft compiler media:

| Candidate | Compiler banner | Linker banner | `/Od` callback | `/O2` control |
| --- | --- | --- | --- | --- |
| [VC2.0 MSDN package](https://archive.org/details/en_vc_2.0) | 9.00 | 2.50 | 144 bytes, exact | 73 bytes, differs |
| [VC2.2 subscription update](https://archive.org/details/msft-visual-cplusplus-22) | 9.10 | 2.55 | 144 bytes, exact | 73 bytes, differs |
| [VC4.0](https://archive.org/details/msvc4x) | 10.00.5270 | 3.00.5270 | 144 bytes, exact | 73 bytes, differs |

Retail's PE header reports linker 3.00. The VC4.0 linker banner is consistent
with that observation, correcting the original survey's attribution to the
VC2.0 era. However, this does **not** prove every linked object was compiled
by VC4.0: all three generations produce identical code for this function.
The exact historical compiler and CRT revision, packing, exception settings,
and engine-wide flags require more independent probes. No `/GX` or `/Zp`
claim is made from this callback.

To repeat the controls after provisioning each candidate with `toolchain install`:

```sh
homm1 probe --ids vc20 vc22 vc40
```

This recompiles each candidate with `/Od` and an `/O2` negative control. The
command succeeds only when all `/Od` cases match and all `/O2` cases differ;
compiler failures and unresolved references fail the command. The report at
`build/probes/report.json` includes compiler component, source, object and
retail hashes plus byte/relocation results. MSVC's `.debug$F` FPO records are
debugger metadata, excluded from the code comparison.

## Retail evidence for AppAbout

- The export directory identifies `AppAbout`, ordinal 1, at `0x0045C15C`.
- The complete body ends with `leave; ret 16`; the return starts at
  `0x0045C1E9`, and the next prologue starts at `0x0045C1EC`. Size: `0x90`.
- Four stack arguments and `ret 16` agree with the Win32 stdcall dialog ABI.
- Message `0x110` returns true. Message `0x111` splits the command into ID,
  control handle and notification code; even the unused local stores remain.
  ID 1 calls `EndDialog(hDlg, 1)`. Other paths reach the service call and return
  false. There is no explicit default arm in the matching source: adding a
  default/break emits an additional five-byte jump with `/Od`.
- The operand at RVA `0x0005C1AC` is a HIGHLOW relocation to the USER32
  `EndDialog` IAT entry at RVA `0x000D661C`.
- The relative-call operand at RVA `0x0005C1DA` resolves to VA `0x0044F640`.
  `PollSound` is the recovered HoMM2 correspondence; see [pilot evidence](../evidence/poll-sound.md).

Reproduce the instruction evidence with:

```sh
homm1 disasm 0x0045C15C --size 0x93
homm1 inspect --json
```

## Correction to the exception-handling survey

At VA `0x004010BF`, retail pushes an unwind sentinel and handler address,
links a frame through `fs:[0]`, and later updates its unwind-state slot around
construction. The handler at `0x0040120D` loads a table pointer and jumps to
`0x004803B0`. The absence of an ASCII `__CxxFrameHandler` name was not evidence
that the statically linked machinery was absent. The original blanket
“no C++ exception handling” conclusion has been removed. This observation
does not, by itself, settle the compiler options for every TU.
