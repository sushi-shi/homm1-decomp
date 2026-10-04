# HoMM1 builds

This page catalogues the known HoMM1 executables, how each was built, and the
runtime libraries each one ships. This branch is implementing the [1.2 to Buka transition](buka-2003.md).
The active target is Buka 2003. Strict comparison remains unavailable while the address
and reference migration is reviewed (`config/retail/targets.json`).

Local copies are kept outside the repository in `~/Projects/homm1/exe`, listed
in its `MANIFEST.md5`. Archive.org item ids are given as `item` or
`item/file`. File offsets, RVAs and counts were measured from the bytes listed
here.

The comparison measurements below were made in the parent 1.0 repository;
"current reconstruction" in those comparisons refers to that snapshot.
See [the 1.1 migration](win95-1.1.md) for this fork.

"Shared with 1.0" means a function from the retail census, minus a few short
functions that have no ≥6-byte unrelocated anchor, appears byte-for-byte in the
other image. Relocated fields are masked for this comparison.

## Summary

| Build | File | Size | PE stamp (UTC) | Linker | Compiler | `__LINE__` style | Debug data |
|---|---|---|---|---|---|---|---|
| Win95 1.0 | `HEROES_win95_1996-02-01.exe` | 713216 | 1996-02-01 05:15:33 | 3.00 | VC 4.0 | /Gi line words | none |
| Win95 1.1 | `HEROES_win95_1.1_1996-05-07.exe` | 715776 | 1996-05-07 20:09:38 | 3.00 | VC 4.0 | /Gi line words | none |
| Win95 1.2 (HEROESW) | `HEROESW_win95_1997-08-29.exe` | 726016 | 1997-08-29 20:36:28 | 3.10 | VC 4.1 | /Gi line words | none |
| Buka 2003 (Russian) | `HEROES_buka_ru_2003-04-11.exe` | 692297 | 2003-04-11 14:42:15 | 6.00 | VC 6 | immediates | NB10 reference, no PDB |
| Editor 1.0 | `EDITOR_win95_1996-02-01.exe` | 305152 | 1996-02-01 01:08:14 | 3.00 | VC 4.0 | mixed | none |
| Editor 1.1 | `EDITOR_win95_1.1_1996-05-06.exe` | 306176 | 1996-05-06 22:18:49 | 3.00 | VC 4.0 | mixed | none |
| Editor 1.2 (EDITORW) | `EDITORW_win95_1997-07-30.exe` | 314368 | 1997-07-30 19:38:13 | 3.10 | VC 4.1 | mixed | none |
| Editor Buka 2003 | `EDITOR_buka_ru_2003-02-26.exe` | 340031 | 2003-02-26 15:31:29 | 6.00 | VC 6 | immediates | NB10 reference, no PDB |

The original VC 4.1 attribution was inferred from linker 3.10. The 1.2 port
subsequently verified the compiler against preserved media and retail-backed
controls; see [the 1.2 migration](win95-1.2.md). Every Win32 build sets OS version 4.0, subsystem
GUI 4.0 and image version 0.0. Each uses image base 0x400000, section alignment
0x1000 and file alignment 0x200, and has a zero PE checksum. The DOS builds are
Watcom C/C++32 with DOS/4GW and are listed briefly at the end.

## Common to the NWC Windows builds (1.0, 1.1, 1.2)

- **Sections:** `.text .rdata .data .idata .rsrc .reloc`, with characteristics
  0x010e (relocations kept, line numbers and local symbols stripped). The images
  have no COFF symbol table, no debug directory and no Rich header.
- **Compilation mode:** sources were compiled with /Zi /Gi (incremental).
  Assert sites load their line through a per-function `__LINE__` word:
  `movsx eax, word ptr [__LINE__Var]` followed by an add. They do not push an
  immediate. See [vc4-gi-line-var](patterns/vc4-gi-line-var.md) and
  [vc4-gi-incremental-compilation](patterns/vc4-gi-incremental-compilation.md).
  The images were linked without /DEBUG.
- **Per-unit profiles:** these are recorded for 1.0 in `config/units.toml`
  `[flags]`.
  - SOURCE game units are `/Od /Zi /Gi /Fa /G5 /Ob1`, packed into one `.text`
    section per object; twenty of them also use `/GX`.
  - Optimized BASE units are `/Ox /Gy /Zi /Gi /G5`, some with `/Ob2`.
  - FINDPATH and SEARCH are optimized SOURCE units.
  - FONT, WINDOW and RESMGR are BASE `/Od /Gy` units.
- **Exception handling:** 81 `fs:[0]` registrations and 79 `0x19930520` FuncInfo
  records. There are no RTTI type descriptors (`.?AV`), so no /GR.
- **String pooling:** BASE functions start on 16-byte boundaries filled with
  `int3` (/Gy). Literals are not pooled (no /Gf): 183–184 duplicate literal
  strings remain.
- **C runtime:** static VC LIBCMT, linked with `/NODEFAULTLIB:libc.lib`. There is
  no MSVCRT import, the TLS and critical-section imports of the MT CRT are
  present, and the `R60xx` runtime-error table and the
  "Microsoft Visual C++ Runtime Library" string are embedded.
- **Stack and heap:** stack 0x10240 reserve / 0x1000 commit, from
  `config/heroes.def`; heap 0x100000 / 0x1000.
- **Dynamic loading:**
  - DirectDraw is loaded at run time: `LoadLibraryA("DDRAW.DLL")`, then
    `GetProcAddress("DirectDrawCreate")`, with "Error loading DDRAW.DLL" on
    failure.
  - WinG 1.0 is imported statically (6 functions).
  - Network play uses `NETAPI32!Netbios`. The serial link uses the Win32 comm
    API through KERNEL32.
- **System imports:** WINMM (7: aux*, waveOut caps, mciSendString for CD
  audio), USER32 (51–53), GDI32 (12), ADVAPI32 (4 registry calls), KERNEL32
  (84 in 1.0/1.1, 89 in 1.2).

## Windows 95 1.0 — `HEROES.EXE` (parent target)

- **sha256** `0d707d3456aacd470f4da601ac388a8b0be8976414b4ef689c8b849b9b2a1ce8`,
  md5 `58a5ddcc48793618632d48dff2522fa9`, 713216 bytes. The version string
  reads "Loading Heroes of Might and Magic for Windows 95 (version 1.0)".
- **Sources:** `***REMOVED***` (redump NWC/3DO collection), in these
  entries:
  - "(USA) (Windows 95)"
  - "(USA) (Rerelease) (2000-06-28)"
  - Millennium Edition Disc 1 and Disc 1 Alt
  - Platinum Edition Disc 1

  Also: `***REMOVED***`,
  `***REMOVED***/HEROES.BIN`,
  `***REMOVED***` and
  `***REMOVED***`. On every disc it
  sits at `/HEROES/HEROES.EXE`, and inside the InstallShield 3 archive
  `/HEROES/HEROES.Z`.
- **Section layout (RVA/virtual size):**
  - `.text` 0x1000/0x8a0a0
  - `.rdata` 0x8c000/0x165b
  - `.data` 0x8e000/0x47f40
  - `.idata` 0xd6000/0x1480
  - `.rsrc` 0xd8000/0x1728
  - `.reloc` 0xda000/0x8d00
- **Entry point and exports:** entry RVA 0x826c0. Exports are `AppAbout` (1)
  and `AppWndProc` (2).
- **Source roots:** `D:\Heroes\Base`, `D:\Heroes\Source`. Assert file strings
  are INPUTMGR, MOUSEMGR, OLDASM, RESMGR, Soundmgr and WINMGR (Base), plus
  EVENTS, netlo, NOOPT, PATH, TOWNMGR and wingraph (Source). There are 55
  line-word assert sites, 1 immediate and 10 other.
- **Vendor imports:**
  - `wail32.dll`: 18 `_AIL_*` digital-sample calls.
  - `smkwai32.dll`: 11 functions by ordinal.
  - `WING32.dll`: 6 functions.
- **Usefulness:** the parent repository matching target and source of this fork.

## Windows 95 1.1 — `HEROES.EXE` (target)

- **sha256** `bb69db9112dc48c9eec26c4700cae2265c2c7838410c23ed3a70e57131bfc681`,
  md5 `0c3f9b12b6608faad9ac6f7ced223605`, 715776 bytes. The version string
  reads "(version 1.1)".
- **Sources:**
  - `***REMOVED***`, entry "(USA) (Windows 95) (Rerelease)" Track 01.
  - `***REMOVED***`, file "Ultimate Strategy Archives Disc
    One.bin".
- **Patch:** the 1.0→1.1 RTPatch (`PATCH.EXE` + `PATCH.RTP`, readme dated
  07/15/96) is in `***REMOVED***/***REMOVED***.zip`. It is also in
  `***REMOVED***` at `pcg_2.10_nov_1996.iso/PATCHES/HEROES11.EXE`.
  The patch was not applied here.
- **Section layout:**
  - `.text` 0x1000/0x8a670
  - `.rdata` 0x8c000/0x165b
  - `.data` 0x8e000/0x483f0
  - `.idata` 0xd7000/0x1480
  - `.rsrc` 0xd9000/0x1728
  - `.reloc` 0xdb000/0x8d88
- **Entry point and exports:** entry 0x82c90; same exports as 1.0.
- **Toolchain and imports:** same toolchain, source roots, import set,
  vendor DLLs and per-unit profiles as 1.0. 949 census functions are shared
  with 1.0. There are 56 line-word assert sites.
- **Real code changes against 1.0:**
  - `combatManager::Main` and `advManager::CheckHandleNetPlayerWait`: message
    bound 0x3b→0x3c.
  - `combatManager::ProcessCombatMsg`: switch range 0x27→0x28, one new case.
  - `advManager::ProcessHover`: two added neighbour-cell checks for town
    entrances.
  - `advManager::LoadRemote`: an added call.
  - `game::NewGame`: larger frame, with new per-player initialization.
  - `game::NextPlayer`: added a byte store at `+0x25c` and a call.
  - `game::PerDay`: the same store removed.
  - `advManager::EraseObj`: clears a mapCell flag bit.
  - `combatManager::Open`: reordered calls.
  - `com_init`, `com_rcv`, `com_snd`, `comm_wrt_task`: serial code rewritten,
    and new strings appear ("Initialize communications paramaters", "Suggested
    solutions: … lowering the BAUD rate").
  - `ShutdownComError` is the added function, between `init_anchor` and
    `com_init`, at 1.1 RVA `0x072e57`. The earlier attribution before
    `army::FlyTo` was a discovery error.
  - `NewMap` changes the ultimate-artifact RNG call order to 30/20/20.
  - `CDStop` sends `stop CD wait` instead of `stop CD`.
  - `GiveExperience` changes its `/Gi` line static from 1110 to 1113.
- **Other code differences:** mirrored integer comparisons, reordered integer
  sums and address calculations, and register assignments preserve the observed
  decisions. Floating accumulation in `RVConversion` and
  `ValueOfEventAtPosition` also changes; identical rounding is not assumed.
  The initial masked-byte survey could not establish unchanged source or
  behavioral equivalence. The complete per-function findings and current
  reconstruction limits are in [the 1.1 review](win95-1.1.md#changed-function-review).
- **Readme (patch) fixes:** cursor refresh after AI turns; cursor over castles;
  first combat monster behaviour; Identify Hero visibility.
- **Usefulness:** a second compile-state sample from the same compiler and tree.
  For eight functions that are never exact against 1.0, 1.1 contains exactly the
  bytes the current reconstruction produces: StartSample, StopAllSamples,
  SetMusicQuality, UpdBottomViewEnemyTurn, LoadGame, CalcDifficultyRating,
  HandleRemoteDeadPlayerExit and army::DrawToBuffer. As a target it would trade
  those for its real changes and for 1.0-exact functions it flips. It is not a
  better target by that metric. This fork implements it as a separate target.

## Windows 95 1.2 — `HEROESW.EXE`

- **sha256** `2de85b097b2f7fd538b053619cf7c60ad21394c86b89ffe7587eb252430f45fe`,
  md5 `6e2047ae9dfe7501b32e6d9d85669b44`, 726016 bytes. The version string
  reads "(version 1.2)".
- **Sources:**
  - `***REMOVED***/***REMOVED***.ISO`
  - `***REMOVED***` Compendium (USA) Disc 1
  - `***REMOVED***`, the Compendium Disc 1 `.BIN`

  The Compendium disc also carries DOS `HEROES.EXE` v1.3 and `EDITORW.EXE`.
- **Section layout:**
  - `.text` 0x1000/0x8b5a2
  - `.rdata` 0x8d000/0x2190
  - `.data` 0x90000/0x48ca4
  - `.idata` 0xd9000/0x162a
  - `.rsrc` 0xdb000/0x1728
  - `.reloc` 0xdd000/0x9284
- **Entry point and exports:** entry 0x81770. There is no export directory
  (AppAbout and AppWndProc are not exported).
- **Source roots:** `F:\H1w95src\Base`, `F:\h1w95src\source`. There are 56
  line-word assert sites, so it is still /Gi.
- **CRT:** a newer static LIBCMT. It carries `setlocale` country tables
  ("american english", "united kingdom", …) and has no CRT
  "Assertion failed: %s, file %s, line %d" string.
- **Vendor imports:**
  - `mss32.dll` (Miles Sound System 3.6C, 21 calls): adds
    `_AIL_digital_handle_release/reacquire` and `_AIL_set_preference`.
  - `smackw32.dll` (Smacker 3.0r, 10 calls by name): `_SmackOpen@12`,
    `_SmackSoundUseMSS@4`, ….
  - `WING32.dll`: unchanged.
- **Code against 1.0:** 613 census functions are shared with 1.0. On the /Od
  units the code differs widely from both 1.0 and the current reconstruction. It
  reproduces 1.0's bytes for twelve optimized functions that are never exact
  against 1.0: EncodeData, UpdateEncoderTree, the iconWidget, textWidget and
  textEntryWidget `Main` functions, inputManager::Open, FizzleForward,
  BlitBitmapToScreen, ClippedMonoIconToBitmap, FindCombatPath, FindNearestObject
  and SeedPosition.
- **Usefulness:** a different compiler and CRT; not a matching target for the
  VC4.0 toolchain. Evidence for optimized-unit source shape.

## Buka 2003 Russian — `heroes.exe`

- **sha256** `34233110eff3c5689664ded89577486e3fe8d6961d917c381172248a08a654db`,
  md5 `901fdb7daa130aa168584ec35d6205cd`, 692297 bytes.
- **Sources:**
  - `***REMOVED***`, Buka Platinum anthology: the rar holds an
    ISO; the game is under `/AUTORUN/LAUNCH/SETUP1/`, InstallShield 6 cab group
    "Program Executable Files".
  - `***REMOVED***`, Buka New Year edition: `/HEROES1S/`, with
    identical executable and DLL bytes.
- **Registry key:** `SOFTWARE\Buka\3DO\Heroes of Might and Magic Platinum\1.000`.
- **Section layout:** `.text .rdata .data .rsrc`, with no `.reloc` and
  characteristics 0x010f (/FIXED).
  - `.text` 0x1000/0x88589
  - `.rdata` 0x8a000/0x3c12
  - `.data` 0x8e000/0x4b680
  - `.rsrc` 0xda000/0x1870
- **Entry point, exports and stack:** entry 0x79a38, no exports, stack
  0x10240/0x1000 (the same STACKSIZE as NWC's `.def`).
- **Rich header (VC6-era product ids, named per the published comp.id
  table):**
  - 62 C++ objects (id 11) from compiler build 8966.
  - C objects (id 10) from build 8047.
  - Linker (id 4) and cvtomf (id 5) from build 8447.
  - cvtres (id 6) from build 1735.
  - MASM (id 14) from build 7299.
- **Compilation:** the build is not /Gi. All 52 decoded assert sites push an
  immediate line number.
  - In the sampled assert functions of every unit, locals are addressed
    through `[ebp±x]`. This includes units that 1.0 builds /Ox (MOUSEMGR,
    WINMGR, INPUTMGR), so those functions are unoptimized.
  - FINDPATH and SEARCH are exceptions: complete-TU controls establish
    `/O2 /Ob2 /G5`; retail inlines Clear and QuickDistance in FindCombatPath.
    See [the reviewed pathfinding evidence](../config/retail/buka-pathfinding.json).
  - There is no /GZ fill.
  - /GX: 125 `fs:[0]` frames, 109 FuncInfo records; no /GR.
  - Static VC6 LIBCMT.
- **Debug directory:** CodeView NB10 pointing to
  `E:\Users\igorl\VSS\HMM\HMM1\temp\release\game\heroes.pdb`.
- **Source roots:** `E:\Users\igorl\VSS\HMM\HMM1\Source\Base` and `\Game`. Assert
  file strings are the 1.0 set with `Soundmgr.cpp` replaced by `SMACKMGR.CPP`.
- **Assert line numbers:** EVENTS 1093/1094, INPUTMGR 191, MOUSEMGR 266/322/343,
  NOOPT 16, OLDASM 191, PATH 322/323/339/340, RESMGR 599/620/640/680, SMACKMGR
  178, TOWNMGR 1555/1556/1570/1571, WINMGR 551, netlo 475/516/722, and wingraph
  56, 81, 99, 104, 109, 122, 134, 138, 145, 233, …. The following equal the
  `#line` values in this repository's 1.0 reconstruction:
  - RESMGR, WINMGR, NOOPT and PATH;
  - wingraph through line 233;
  - MOUSEMGR 266.

  Lines in edited files are offset.
- **Imports:**
  - `mss32.dll`: 5 calls; MSS 3.50F, DLL dated 1996-10-23.
  - `smackw32.DLL`: 9 calls by ordinal; a Watcom-built Smacker, 1996-11-19.
  - `audiere.dll`: `_AdrOpenDevice@8`, `_AdrOpenSampleSource@4`; built with VC6,
    2003-01-06.
  - `WING32.dll`: 6 calls.
  - `NETAPI32!Netbios`.
  - WINMM only `waveOutGetNumDevs/GetDevCapsA`; the MCI CD-audio calls are gone.
- **Edits against NWC:** music is Ogg Vorbis through Audiere
  (`Tracks\NN-AudioTrack NN.ogg`, `HeroesNN.ogg`), replacing CD audio and the AIL
  sample path. The binary contains Russian (cp1251) text and the Buka splash
  `buka.smk`.
- **Usefulness:** not a VC4 matching target. It is a non-incremental donor for
  source correspondence and absolute assert line numbers.

## Editors

- **Common:**
  - Editor-specific units (`Editor\EDITMGR`, `EDITOR`, `OVERLAY`, `wingraph`)
    use immediate line numbers, so they are not /Gi.
  - BASE units use line words: 9–10 sites.
  - 160–162 of the BASE census functions are shared byte-for-byte with the
    corresponding HEROES build.
  - Imports are WINMM, USER32, GDI32, ADVAPI32, WING32 and the AIL/MSS startup
    subset (6–7 calls); no Smacker.
  - Stack is 0x100000/0x1000. DirectDraw is loaded dynamically, as in the game.
- **1.0:** `c572831acdb56009e07343de25d67a10c29b50c8b5f984ffb1c24eee906f0cc2`.
  - On the same discs as game 1.0.
  - Root `D:\Heroes\{Base,Editor}`.
  - `.text` 0x1000/0x30200.
  - Exports `AppAbout`, `AppWndProc`.
  - The GOG DOSBox package (`heroes-of-might-and-magic.-7z`) ships this file
    with 51 bytes patched (sha256 `28c76ebd3209…`).
- **1.1:** `6342f3e8e251605cbac685c698e9b84c9f2ec44d473a75b95ac031dc05dff28c`.
  - On the same discs as game 1.1.
  - `.text` 0x1000/0x302b0.
  - Patch readme: map-object cycling fix, single save without viewing.
- **1.2 (EDITORW):**
  `99df6c1e8f2563129fadccc268ce0b46a8baec85ea3f7eefe670daec61dcb644`.
  - Root `F:\h1w95src\{base,editor}`, linker 3.10, MSS imports, no exports.
- **Buka 2003:**
  `103380e9a8e4030ba25a76447bef1d6f3d05c47b0caf2f6748620dc3487f0485`.
  - VC6, /FIXED, immediate asserts (43), no MSS or Smacker imports, `audiere.dll`.
  - NB10 path `U:\HMM\VSS\HMM1\temp\release\editor\editor.pdb`.
  - Root `U:\HMM\VSS\HMM1\Source\{Base,Editor}`.

## Runtime libraries

| File in `~/Projects/homm1/exe` | Shipped with | sha256 (prefix) | Stamp / linker | Identification |
|---|---|---|---|---|
| `WAIL32_win95_1996.dll` | 1.0, 1.1 | `2a2134551ad7` | 1995-11-22 / 2.60 | Miles AIL V3.03 for Win32, 104 exports; NB10 `R:\NET\LIBS\AIL\WAIL\DEV\V3\wail32.pdb` (not shipped) |
| `SMKWAI32_win95_1996.dll` | 1.0, 1.1 | `129f724e4300` | 1995-11-28 / 2.18 | Smacker for Win32s with WAIL sound (`_SetSmackWAILDigDriver`), Watcom-built, 32 exports |
| `SMACKW32_win95_1996.dll` | 1.0, 1.1 | `10ba3312e71e` | 1995-10-04 / 2.18 | Smacker for Win32s, Watcom-built, 31 exports (not imported by the game) |
| `WING32_win95_1996.dll` | 1.0, 1.1, 1.2, Buka | `bb1f552e2525` | 1994-09-21 / 2.23 | Microsoft WinG 1.0 (file version 1.0.0.37) |
| `MSS32_win95_1997.dll` | 1.2 | `121465991209` | 1997-06-10 / 4.20 | Miles Sound System V3.6C, 164 exports |
| `SMACKW32_win95_1997.dll` | 1.2 | `50279571bf85` | 1997-08-07 / 4.20 | Smacker 3.0r (VERSIONINFO 3.0.0.0), MSVC-built |
| `MSS32_buka_2003.dll` | Buka | `a2912c7f0047` | 1996-10-23 / 4.20 | Miles Sound System V3.50F |
| `SMACKW32_buka_2003.dll` | Buka | `0d9973230fd4` | 1996-11-19 / 2.18 | Smacker (Watcom-built), requires MSS 3.50F |
| `AUDIERE_buka_2003.dll` | Buka | `10f197563769` | 2003-01-06 / 6.00 | Audiere (C++ with RTTI), Ogg playback |

The 1.0 and 1.1 discs carry identical DLL bytes, both loose and inside
`HEROES.Z`. The 1.0 and 1.1 discs carry the DirectX 1 redistributable
(`DDRAW.DLL` 4.02.0095), and the 1.2 disc a later one. DirectDraw is loaded at
run time, never imported.

## DOS builds

All are Watcom C/C++32 with DOS/4GW Professional and Miles AIL 3 DOS drivers;
they carry no debug information. `KB.EXE` (King's Bounty, sha256
`5e72b627c1e3…`, on every Windows disc) is a Borland C++ DOS program.

| File | sha256 (prefix) | Version string | Archive.org sources |
|---|---|---|---|
| `HEROES_dos_en_patch12_1995-10-12.exe` | `feacfeac7d64` | v1.2 | ***REMOVED*** OEM, NWC "(USA) (OEM)", `***REMOVED***`, PC Player 01/96, ***REMOVED*** |
| `HEROES_dos_de_1995-10-09.exe` | `afeef9223422` | (German) | `***REMOVED***/CD01.img` |
| `HEROES_dos_demo12_1995-11-28.exe` | `ca05cf263e59` | Demo v1.2 | PC Gamer 03/96, PC Action 16, Game Head 9 |
| `HEROES_dos_1997-08-01.exe` | `0518cde12e53` | v1.3 | `***REMOVED***`, Compendium Disc 1 |
| — | `b53e33f82f92` | (USA retail) | NWC "(USA)" |
| — | `d055e92497b7` | (Simplified Chinese) | `***REMOVED***/HEROES_SIM.iso` |
| — | `f9340223a04b` | (Traditional Chinese) | `***REMOVED***/HEROES.iso` |
| — | `0cbd9272d2cb` | CES 1995 demo | `***REMOVED***`, Score 02/96 |

The GOG package and two repacks contain byte-patched copies of `feacfeac…`
and `b53e33…`. These are `0e87d302…`, `274a0954…` (also on
`***REMOVED***`, the 2001 "12 в 1" disc) and the patched GOG editor.

## Debug symbols

No PDB, DBG, MAP or symbol file for any HoMM1 build was found.

- **NWC Windows builds (1.0, 1.1, 1.2 and their editors):** no debug
  directory, no COFF symbol table, no Rich header, no RTTI names.
- **Buka 2003 game and editor:** NB10 references only, to
  `E:\Users\igorl\VSS\HMM\HMM1\temp\release\game\heroes.pdb` and
  `U:\HMM\VSS\HMM1\temp\release\editor\editor.pdb`.
- **Disc and archive file lists checked:** the Buka Platinum ISO, all three
  setup cabs (HoMM1, HoMM2, HoMM3), the Buka New Year ISO and HoMM1 cab, the
  NWC 1.0, 1.1 and 1.2 discs, `HEROES.Z`, and the "12 в 1" disc. None contains
  `*.pdb`, `*.dbg`, `*.sym`, `*.idb`, `*.obj` or a linker map. `_SETUP.LIB` is an
  InstallShield script library, and the `*.MAP` files are game maps.
- **Other Buka products from the same rebuild:**
  - HoMM2 `HMM2PL.exe` and `EDT2PL.exe`, 2003-04-04, VC6: NB10
    `e:\Users\igorl\VSS\HMM\HMM2\temp\release\...` only, and no PDB on the
    disc.
  - HoMM3 `Heroes3.exe`, `h3maped.exe` and `h3ccmped.exe`, 2003-04: no PDB path.
- **Magazine discs:** 116 Russian magazine and collection items on archive.org
  were listed: Игромания, Game.EXE, Навигатор игрового мира and Страна Игр
  2002–2005, plus the HoMM-titled Russian collections. None contains HoMM1
  executables or debug files. Their "HEROES" paths are HoMM3/4 maps, guides and
  images.
- **Third-party libraries:** the only shipped PE with debug records is
  `WAIL32.DLL` (Miles). Its NB10 record names a PDB that is not present.

## Local file index

`~/Projects/homm1/exe/MANIFEST.md5` lists every file. These were added for
this catalogue:
- `HEROES_win95_1.1_1996-05-07.exe`, `EDITOR_win95_1.1_1996-05-06.exe`
- `HEROES_buka_ru_2003-04-11.exe`, `EDITOR_buka_ru_2003-02-26.exe`
- the runtime DLLs in the table above
