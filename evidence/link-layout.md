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
- LIBC starts with exsup.obj at 0x004801f0. Naming `/ENTRY` pulls wincrt0.obj
  first. Retail has wincrt0 after the CRT members that SOURCE references, so
  the entry point was LINK's default.

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

## Function-level linking in BASE

Every BASE C++ function begins on a 16-byte boundary with int3 fill. The
`/Od` SOURCE objects are packed. FONT, WINDOW and RESMGR therefore use the
`/Od` profiles plus `/Gy`. With this profile, `??_Gfont` falls between the
constructor and `~font`, as in retail. The object comparison also gains five
exact functions and loses one operand-order row (`GetBackdropAtLoc`).

## CRT member order

The CRT member order matches retail, except where source spellings differ.
Under the measured pull rule, retail references `__chdir` directly before the
entry point (kbwin uses `_chdir`, not OLDNAMES `chdir`) and `__stricmp`
directly (from BASE). It reaches `_strrev` only through OLDNAMES, because
`strrev` is pulled last.

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
