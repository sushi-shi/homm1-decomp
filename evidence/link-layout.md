# Retail link layout

Measured with the pinned VC4 LINK (3.00) against diagnostic candidate images.
The candidate link itself (`homm1 link`) never uses `/FORCE`.

## Header and directories

- Optional header: stack reserve 0x10240, commit 0x1000; heap defaults.
- Export directory at 0x0048d600 (0x5b bytes): module name `HEROES.EXE`,
  `AppAbout` (ordinal 1, 0x0045c15c) and `AppWndProc` (ordinal 2,
  0x0045bb45), both by undecorated name. `config/heroes.def` states these.
- Import descriptors: WINMM, KERNEL32, USER32, GDI32, ADVAPI32, NETAPI32,
  smkwai32, WING32, wail32. LINK emits descriptors in library command-line
  order, so the explicit library line follows this order.
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
  `LzhufMemmove`. Merging the MASM units resolves their mutual calls without
  relocations, while the delinked targets keep call relocations. Four
  functions then score below 100 in the per-function comparison. The units
  remain separate until the comparison treats an in-object MASM call as its
  relocated equivalent.

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
  ReadPacket. REMOTE now holds RemoteCleanup through nbnet_init, as Buka's
  `REMOTE.cpp` and Netbios code do. Modem holds ModemSetup through
  TransmitAndWait, in Buka's `Modem.cpp` order. ModemSetup begins on the
  16-byte boundary at which nbnet_init ends. All 52 functions are at 100.
- army::army (0x00466490) starts a new object after WalkTowardArmy (eight
  `CCh`). The thirteen combatManager functions before it are Buka's
  `SOURCE/AI.cpp`, in the same order. Splitting them out of ARMY leaves the
  AI functions' scores unchanged. However, the TU state of ARMY changes:
  DoAttack, SpecialAttack and DamageEnemy improve, while AttackTo, SpellEffect,
  PowEffect, DoHydraAttack and Walk regress. The exact CUR count falls from 27
  to 26, so the split is not applied.
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
retail in 97 of 100 positions, up from 85. The simulation gives 99 of 103.
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
