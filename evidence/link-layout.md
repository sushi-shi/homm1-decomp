# Retail link layout

Measured with the pinned VC4 LINK (3.00) against diagnostic candidate images.
The candidate link itself (`homm1 link`) never uses `/FORCE`.

## Header and directories

- Optional header: stack reserve 0x10240, commit 0x1000; heap defaults.
- Export directory at 0x0048d600 (0x5b bytes): module name `HEROES.EXE`,
  `AppAbout` (ordinal 1, 0x0045c15c) and `AppWndProc` (ordinal 2,
  0x0045bb45), both by undecorated name. `config/heroes.def` states these.
- Import descriptors: WINMM, KERNEL32, USER32, GDI32, ADVAPI32, NETAPI32,
  smkwai32, WING32, wail32. LINK emits descriptors in the order it first pulls
  each library. See [Import libraries](#import-libraries) for wail32's place.
- Only 17 jump thunks survive in the game band (smkwai32 x11 and WING32 x6 at
  0x00480188). Thunks for the other 179 import slots are absent, so retail was
  linked with the default `/OPT:REF`.
- The CRT starts with exsup.obj at 0x004801f0. Naming `/ENTRY` pulls
  wincrt0.obj first. Retail has wincrt0 after the CRT members that SOURCE
  references, so the entry point was LINK's default.

## SOURCE objects, then the BASE library

The NETAPI32 `Netbios` thunk sits at 0x0047343c. It lies between comwin (the
last SOURCE-run object) and the BASE run at 0x00473450. LINK places
import-library members after every object on the command line, in the order
it pulls them. A thunk among game objects therefore means that the following
code was also pulled from a library, searched after netapi32.lib. A control
link with the SOURCE objects explicit and the BASE units in a library puts the
thunk at exactly 0x0047343c.

VC4 LINK pulls library members by walking the undefined-symbol list in the
order the symbols were first inserted (object symbol tables, in link order).
A simulation of that rule reproduces the control link's member order exactly
(31 members). The same rule reproduces retail's BASE order only under these
object compositions:

- miscwin, OLDASM, Iconm2bClip and PrintMemoryLeaks are one object. `Random`
  is the first BASE symbol that SOURCE references (HISCORE's constructor), but
  retail places the blitters before it. The four units share the `/O2` profile.
  OLDASM's assert names `OLDASM.CPP` (0x004a0c44), which fits an included
  file.
- The six unscaled icon renderers (Icon2b family) are one MASM object. The
  same applies to the two clipped renderers (Icon2bc, Iconf2bc). In each
  object, the procedures are separated by one `90h` byte (an in-module `EVEN`).
  LINK's fill between objects is `CCh`.
- The six LZHUF decoder routines are one object. They are byte-contiguous with
  odd starts. LZHUF references only `Decode`, yet retail begins the group with
  `LzhufMemmove`. They are now assembled as one module
  (`vendor/lzhuf/decoder/Decoder.asm`, unit BASE/LZHUFDEC). MASM resolves
  their seven mutual calls without relocations. The comparison gives each of
  these calls the REL32 relocation that the delinked target carries, and all
  six functions stay at 100. As separate DWORD-aligned objects they had added
  12 bytes of fill and the wrong order.

## Object boundaries in the SOURCE run

The `/Od` SOURCE objects are packed: their functions follow each other with no
fill, and each object's `.text` is 16-byte aligned. LINK pads between objects
with `CCh`. In retail, int3 fill before a 16-byte boundary therefore ends an
object. A function that starts at an unaligned address immediately after its
predecessor must share that predecessor's object. Applying both rules to the
claimed functions from 0x00401000 to 0x00473450 recovers these compositions:

- armyGroup (0x00447920) starts a new object after GAME's RestoreCell (ten
  `CCh`). Buka 2.1 keeps the same eleven methods, in the same order, in
  `SOURCE/ARMYGRP.cpp`. They are now a separate SOURCE/ARMYGRP unit, and all
  eleven are at 100.
- RemoteCleanup (0x00458520) starts a new object after SETUP's
  BaseSetupHandler (thirteen `CCh`). Dial (0x00459627) and WriteModemPacket
  (0x0045a16b) start at odd addresses directly after ModemSetup and
  ReadPacket. The object's .data (0x0049f808-0x0049fdb7) and .bss
  (0x004c7e70-0x004ca487) interleave the variables of the network bring-up,
  the modem helpers and the packet layer. For example, iNetNameIndex,
  iIDCtr, packetlen, iInOrderCtr and GameMode are consecutive. So everything
  from RemoteCleanup to TransmitAndWait is one TU, SOURCE/REMOTE (Buka
  REMOTE, Netbios and Modem code). Its data is defined after its functions in
  retail address order. ModemSetup's 16-byte start (where nbnet_init ends) is
  a coincidence. With the former SETUP, Modem and REMOTE scores carried over,
  the 52 functions keep 51 exact rows; GUIModemCommandExec stays at 95.81.
- army::army (0x00466490) starts a new object after WalkTowardArmy (eight
  `CCh`). The thirteen combatManager functions before it are Buka's
  `SOURCE/AI.cpp`, in the same order, and are now the SOURCE/AI unit with
  ARMY's profile. The AI functions keep their scores (their TU prefix is
  unchanged). The ARMY TU state changes: PowEffect and Walk are re-tuned to
  100 by local declaration order, while MoveAttack and SpecialAttack reach
  100 only as audited TU-state closures (`tu_state_noise` trials 1 and 7).
- One object begins at 0x00419990. Its first function is a 0x15-byte dynamic
  initializer that calls the 0x1a-byte initializer at 0x0041f2a9, which
  constructs SVSearchArray. Next come Misc's nine logging functions, starting
  at the odd address 0x004199a5, and then PHILAI from 0x00419f16, also odd.
  Misc and PHILAI are therefore one TU. VC4 emits the two initializer
  functions next to each other at the global's definition (`_$E2` and then
  `_$E1`, confirmed by test compiles with and without `/Z7` and `/GX`). The
  outer function at the start of the object is not reproduced by any source
  order tried so far. Merging Misc into the top of PHILAI keeps the nine
  logging functions at 100. However, ten PHILAI functions regress and three
  reach 100, so the exact count falls from 60 to 53 and the merge is not
  applied. With the current split, PHILAI ends 13 bytes late and FINDPATH
  starts 16 bytes late. This 16-byte shift persists through HERO and into the
  BASE run. The SOURCE-run alignment no longer absorbs it, because ARMYGRP
  and REMOTE now begin on their own boundaries.

## Function-level linking in BASE

Every BASE C++ function begins on a 16-byte boundary with int3 fill. The
`/Od` SOURCE objects are packed. FONT, WINDOW and RESMGR therefore use the
`/Od` profiles plus `/Gy`. With this profile, `??_Gfont` falls between the
constructor and `~font`, as in retail. The object comparison also gains five
exact functions and loses one operand-order row (`GetBackdropAtLoc`).

## C runtime: LIBCMT

Retail links the VC4.0 multithreaded LIBCMT.LIB, not the single-threaded
LIBC.LIB that the objects request. The pinned VC4.0 LIBC's tidtable.obj is
empty, while retail carries LIBCMT's `_mtinit` (0x00484050: `_mtinitlocks`,
`TlsAlloc`, a 0x74-byte `calloc`'d per-thread block, `TlsSetValue`,
`GetCurrentThreadId`), `_initptd` and `_getptd` (`GetLastError`,
`TlsGetValue`, ..., `SetLastError`). No `TlsFree` is imported: `_mtterm` has no
caller in an executable. The DNA census masks relocations and compares
retail's CRT band (0x004801f0 to the end of `.text`, 44700 bytes) with each
library:

| Library | exact bodies | exact bytes |
| --- | --- | --- |
| VC4.0 LIBC.LIB | 106 | 14686 |
| VC4.0 LIBCMT.LIB | 189 | 35052 |

The LIBCMT matches include `_lock`/`_unlock`, `_mtinitlocks`, the `*_lk`
stream and low-level I/O variants and `_isctype`. Every `_isctype` caller is
inside the CRT. No game function uses an `_MT`-dependent macro, so the game
code cannot distinguish `/MT` objects from `/ML` objects linked with
`/NODEFAULTLIB:libc.lib libcmt.lib`. The candidate link uses the latter
and leaves compiler profiles unchanged. VC 2.x media hold only older LIBC
revisions, and VC 4.1/4.2 postdate the February 1996 build, so no other CRT
revision is needed. The evidence census rows carry the LIBCMT identities.

With LIBCMT, the candidate imports exactly retail's 0x334-byte IAT.

## CRT member order

LINK searches each library over the undefined-symbol list in insertion order.
Explicit objects come first, then the entry symbol, then the members of
`base.lib`. OLDNAMES is searched after LIBCMT, so a call spelled with an
OLDNAMES alias (`chdir`, `close`) reaches its LIBCMT member only in a later
pass. A simulation of this rule over the candidate's objects reproduces the
candidate's LIBCMT member order. Measured against retail's 103-member order,
it scores 87. Retail's order needs these direct LIBCMT spellings:

- kbwin calls `_lseek` and `_chdir` (lseek and chdir precede wincrt0). HoMM2's
  matching CD-drive code spells both with the underscore.
- SAMPLE and soundmgr call `strrev` through OLDNAMES (strrev is the last
  member). HoMM2's SAMPLE spells `strrev`.
- `close`, `read`, `open`, `write`, `strnicmp` and `stricmp` directly follow
  wincrt0, ahead of soundmgr's `rewind`. RESMGR calling `_close`, `_read`,
  `_open` and `_lseek`, plus soundmgr calling a direct `stricmp` spelling,
  reproduces close, read, open and stricmp. Nothing in the current units
  places `write` or `strnicmp` there.

With all of these, the simulation scores 100 of 103.

Comparison now resolves an OLDNAMES reference as LINK does. Each OLDNAMES.LIB
member is a weak external that defaults to the runtime name (`_lseek` to
`__lseek`). LIBCMT's stricmp.obj defines the function `__stricmp` and the
untyped label `__strcmpi` at the same offset, so `strcmpi` also reaches
`__stricmp`. `homm1.compare.runtime_aliases` reads these records from the
pinned libraries. The canonicalizer then names every undefined OLDNAMES
reference by its runtime function, and the reviewed referents use the runtime
symbols (`__lseek`, `__chdir`, `__close`, `__read`, `__open`, `__tell`,
`__write`, `__stricmp`, `__strnicmp`). The two spellings of one call then
compare equal.

The source now uses retail's spellings in functions at 100: kbwin's
SetupCDDrive (`_chdir`, `_lseek`), `sample::sample` (`strrev`), RESMGR
(`_close`, `_read`, `_open`, `_lseek`) and soundmgr's CDStop, CDIsPlaying and
CDPlay (`_stricmp`). Every edited function stays at 100. Measured on a
diagnostic link of the candidate objects, the LIBCMT member order matches
retail in 97 of 100 positions, up from 85, and the simulation gives 99 of 103.
Retail's order can also be counted over 112 members, including the referent-
named `access`, `__purecall` and `write`. On that count the candidate,
together with the `access` change below, matches 107, up from 95.
The remaining differences are as follows:

- soundmgr's StartSample still calls `_strrev` directly, so strrev is pulled
  early instead of last. Changing the spelling to `strrev` leaves StartSample
  at its bank (97.77) and raises the simulation to 100 of 103. It is not
  applied here because StartSample is below 100.
- `access` (0x004834c0) follows stricmp in retail, where soundmgr's direct
  `_access` pulls it. kbwin's ReadPrefsFromFile spelled `_access` and pulled
  it ahead of fwrite (80 bytes early). It now calls `access`, as HoMM2's
  ReadPrefsFromFile does, and stays at 100.
- `__purecall` (0x00483510) follows access in retail. The candidate pulls it
  directly after wincrt0, because BASEMGR's constructor stores baseManager's
  vtable, whose three slots are `__purecall` in retail too (0x0048c5a0). Why
  retail's BASEMGR object did not insert `__purecall` that early is open.
- `write` and `strnicmp` directly follow `open` in retail. No current unit
  before soundmgr references `__write` or `__strnicmp`. BASE C++ objects use
  `/Gy` and retail links with `/OPT:REF`. A function that nothing called would
  be discarded from the image, but its references would still pull these
  members. Neither the image nor the donors identify such a function.

## Resources

Retail `.rsrc` (0x000d8000, 0x1728 bytes) holds seven payloads, all language
1033, in this data order: `RT_ICON` 1 (32x32, 16 colors), `RT_GROUP_ICON`
`HEROES`, `RT_DIALOG` `HEROES`, and the `RT_MENU` resources `MNUADV`,
`MNUDFLT`, `MNUCMBT` and `MNUTOWN`. LINK/CVTRES place data in `.res` record
order. The directories sort names alphabetically, so the data order is the
resource-script statement order. `src/SOURCE/Heroes.rc` compiled by the VC4
RC.EXE reproduces all seven payloads. Linked into a diagnostic image, its
`.rsrc` has retail's size and equals retail byte-for-byte once data-entry
RVAs are taken relative to the section, except the 12 directory
`TimeDateStamp` fields. CVTRES writes its conversion time there, and retail's
0x31104c71 lies 4 seconds before the PE header stamp 0x31104c75, so these
fields are build-time values like the header stamp.

## Import libraries

Retail `.idata` is 0x1480 bytes. With VC4-format import libraries for all
nine DLLs, the candidate's was 0x146c, and its ILT/IAT groups came in a
different order. Three link facts close both differences. The candidate's
descriptor table, ILT/IAT group placement and hint/name layout now equal
retail's.

- **Two null descriptors.** Retail has 20 zero bytes at 0x004d60b4 (the
  terminator after the nine descriptors) and 20 more at 0x004d60c8, before
  the first ILT at 0x004d60dc. VC4 import libraries define
  `__NULL_IMPORT_DESCRIPTOR`. The VC 2.0 LINK 2.50 import format (the 1994
  SDK libraries still in VC4's `lib/`, such as ctl3d32.lib and mapi32.lib)
  names its descriptor `<DLL>_IMPORT_DESCRIPTOR` and its terminator
  `NULL_IMPORT_DESCRIPTOR`. One library of each format in a link therefore
  produces two `.idata$3` terminators. WinG 1.0 dates from 1994. WING32.lib,
  rebuilt with the pinned VC 2.0 LINK (`/DLL /IMPLIB` over the same stub),
  gives retail's size, and its hints still check against retail.
- **ILT/IAT group order.** Retail's ILT groups run ADVAPI32, GDI32, KERNEL32,
  NETAPI32, USER32, WINMM, smkwai32, wail32, WING32. LINK 3.00 sorts the
  `.idata$4`/`.idata$5` contributions by archive member name, case-sensitively
  (renaming only the member headers moves a group). WING32 after wail32
  needs a member name that sorts after `wail32.dll`. LIB 2.50 names members
  after the `LIBRARY` statement, which would also lowercase the DLL string,
  and retail's string is `WING32.dll`. The vendor's tool is not known. The
  build renames the members `wing32.dll`, the smallest change that gives
  retail's order.
- **wail32 is pulled in the second pass.** Hint/name entries follow pull
  order. Retail's wail32 entries (0x004d72b4 to 0x004d7474) come after the
  second-pass USER32 and GDI32 entries, and its descriptor is last. Only
  BASE's soundmgr calls AIL. So wail32.lib was searched before the library
  that supplies soundmgr: it pulls nothing in the first pass and everything
  in the second. The candidate line puts it after netapi32.lib and before
  base.lib. Any slot between gdi32.lib and base.lib gives the same image.

The `.idata` bytes still differ in the slot order inside each DLL's ILT/IAT
(WINMM, KERNEL32, USER32, GDI32, smkwai32, WING32 and wail32). Within one
member-name group, the order is neither pull order nor name order. For
example, retail's WING32 slots follow pull order and the candidate's are a
rotation of it. The slot order depends on LINK's internal symbol ordering
and is left for data matching.

## Resources in the candidate

`homm1 link` puts `build/gen/heroes.res` on the link line when the pinned
RC.EXE and CVTRES.EXE are installed (`homm1 toolchain install --id vc40
--media build/downloads/MSVC40.iso`). The linked `.rsrc` is 0x1728 bytes, as
in retail, and differs only in the 12 directory `TimeDateStamp` fields.

## Remaining candidate differences

Measured on the candidate linked from master e571578 plus the import-library
changes above. Section virtual sizes, candidate minus retail: `.text` +0x22,
`.rdata` -0x20, `.data` -0x450 (with `.bss`), `.idata` 0, `.rsrc` 0, `.reloc`
-0xe8 (follows the other sections).

### `.text`

The claimed game and BASE functions end 0x30 late. Each step comes from one
cause:

| Where | Step | Cause |
| --- | --- | --- |
| Misc/PHILAI, 0x004199a5 to 0x00424810 | +0x10 | object composition: `_$E2` (0x15 bytes) is emitted beside `_$E1` mid-PHILAI instead of at 0x00419990, and Misc ends in its own 16-byte-aligned object |
| FINDPATH `FindCombatPath` | +0x10 | body 2 bytes long under `/Gy` crosses a 16-byte boundary |
| KB `InterpretCommandLine`, `UpdateAppSpecificMenus` | -0x10 | bodies 1 byte short each; the KB object ends one paragraph early |
| miscwin `FadeIn` | +0x10 | body 4 bytes long under `/Gy` |
| BITMAP `CopyTo` | +0x10 | body 4 bytes long under `/Gy` |

Other short or long bodies are absorbed by object padding: TOWNMGR `BuyBuild`,
PHILAI `DoDimensionDoor`, `DoAI` and `DetermineHeroToMove`, ADVMGR `DrawCell`
and `ComboDraw`, GAME `NewGameHandler`, `SGenRand`, `CheckHeroConsistency`
and `GetNumThievesGuilds`, REQUEST's constructor, FLY `CanFit`, kbwin
`AppWndProc`, EVENTS `EraseObj`, ARMY `DrawToBuffer` and `Walk`, and in BASE
`BlitBitmapToScreen`, `PostprocessPalette`, `ClippedMonoIconToBitmap`,
`FizzleForward`, `MemorySample` and `EncodeData`. SEARCH
`FindNearestObject` is 1 byte long.

The CRT band then shrinks by 0xe of fill. Its members are the same, but four
are out of place: `__purecall` comes directly after wincrt0, `write` and
`strnicmp` come at the end, and `strrev` comes early (see
[CRT member order](#crt-member-order)).

### `.rdata`

- ADVMGR: retail has 8 more bytes between `glEnvironmentVolume` (5 longs at
  0x0048c390) and the first double at 0x0048c3b0. The candidate's double is
  at 0x0048c3a8.
- Retail ends `.rdata` with 8 zero bytes and the unreferenced string
  `Heroes of Might and Magic` (0x0048c818 to 0x0048c840), after the CRT
  constants. No relocation refers to it. Its owner is unknown.
- `.xdata$x` has the same 70 FuncInfo records. Its tail differs by +8.

### `.data`

The first differing byte, with relocated fields masked, of each object that
has claimed identities. Where it lies is the class.

- **SEARCH**: retail's 8 bytes at 0x0048e170 are SeedPosition's initialized
  counter. The candidate has none, so every later object starts 8 early.
- **Assert line words and `__FILE__` arrays.** Retail emits them after the
  object's literals: TOWNMGR's pair follows SplitArmyHandler's literals at
  the end of the object, and wingraph's and netwin's line words are
  interleaved with literals. The candidate's file-scope definitions are
  emitted first. This affects wingraph, TOWNMGR, netwin, PATH, EVENTS,
  RESMGR, MOUSEMGR, soundmgr, INPUTMGR and EXEC. The pinned VC4
  (`/Od`, `/Od /Gy`, `/O2`, C and C++, file-scope or function-local `static`)
  always emits initialized variables before literals.
- **Literals under `/Gy`.** The pinned VC4.0, VC 2.0 and VC 2.2 all put each
  literal in its own 8-aligned `.data` COMDAT under `/Gy` (and `/O2`). Retail
  BASE objects pack their literals at 4-byte alignment: RESMGR's `File Error:
  .AGG File not valid` follows the 20-byte `Can't open file: %s` at +0x14,
  where the candidate has +0x18. Retail also keeps the literals of functions
  that the candidate does not contain (WINMGR's `wb` and its second
  `CCYCLE%02d.BIN`, WINDOW's `Default Construct`). RESMGR's and WINMGR's
  variables follow some of the object's literals in retail. The candidate's
  variable section comes first and is 8-aligned. No pinned compiler and flag
  set has been found that keeps `/Gy` functions but leaves literals in one
  section.
- **Definition order or storage class.** SPELLS has an extra 4-byte variable
  before `spelmous.mse`. PHILAI has the known interleave of
  `bSVSearchArrayInUse`. ADVMGR has an extra 16 zero bytes after
  `giCheatSeq`. In GAME, `gbShowMapInfo` is followed by variables instead of
  `%s\n`. CMBTMGR's `cCombatBkgNames` pointer table sits where retail has
  `PREBATTL.82M`. In KB, `gMinExpForLevel` sits where retail has a byte table
  (+0x151a). REMOTE's variables after `iBaudBits` differ. kbwin has zero
  variables where retail has `Heroes`.
- **Literal order or content.** ARMY: after `gbSecondShot`, retail has
  `perishes`, the candidate `swrdsman`. HERO at +0x44: retail has `\n%d`, the
  candidate `%ld`. REQUEST: retail's `bigfont.fnt` comes before the candidate's
  `%s%s`.
- Byte-equal over the candidate's extent: CURSOR, SETUP, SMACKMGR, NOOPT,
  WINDOW, Icon2b and BMAP2.

### `.bss`

The objects whose identities are out of order are those in
[bss-name-order.md](bss-name-order.md): wingraph, netwin, PHILAI, ADVMGR,
SPELLAI, GAME, KB, REMOTE, WINMGR, soundmgr and LZHUF. The names decide this
order. SEARCH, CURSOR, COMMAND, FINDPATH, REQUEST, SMACKMGR, kbwin, EVENTS,
ARMY, comwin and MOUSEMGR keep a constant offset.

### `.idata`

The slots inside each DLL's ILT/IAT are in a different order, as described in
[Import libraries](#import-libraries).
