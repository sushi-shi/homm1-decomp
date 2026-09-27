# Target survey

The canonical target is Windows 95 `HEROES.EXE`, February 1, 1996, shipped
byte-identically on the USA Platinum and Millennium disc 1 releases.
`config/retail/targets.json` owns the executable hashes and sizes; `homm1 init`
verifies inputs and regenerates `build/analysis/{game,editor}.json`.
`homm1 inspect --json` reproduces PE layout, imports, exports and relocations.

The image is relocatable at VA `0x400000`, entry VA `0x4826c0`, with linker
3.00 and timestamp `0x31104c75`. Its `.text` is RVA `0x1000`, virtual size
`0x8a0a0`. Imports include Win32, WING32, wail32 and smkwai32. The same-disc
EDITOR.EXE is secondary evidence, with entry VA `0x429b70` and `.text` size
`0x30200`; it is not part of the game's matching denominator.

## Source and compiler clues

The surveyed Windows image has no PE debug directory, trailing overlay or
CodeView NBxx signature. No RTTI strings were found. This does not rule out
statically linked exception handling: VA `0x4010bf` registers an fs:[0] frame,
updates unwind state and points to handler VA `0x40120d`, which reaches the
runtime at `0x4803b0`. See [inline EH evidence](vc4-inline-eh.md).

Two exports provide binary-authored names: AppAbout at VA `0x45c15c` (ordinal 1)
and AppWndProc at `0x45bb45` (ordinal 2). Assertions preserve paths under:

- `D:\Heroes\Base\`: INPUTMGR.CPP, MOUSEMGR.CPP, OLDASM.CPP, RESMGR.CPP,
  Soundmgr.cpp, WINMGR.CPP.
- `D:\Heroes\Source\`: EVENTS.CPP, netlo.cpp, NOOPT.CPP, PATH.CPP,
  TOWNMGR.CPP, wingraph.cpp.

These are assertion-bearing units, not a complete TU list. The editor also
names EDITMGR, EDITOR and OVERLAY and its own wingraph.cpp. The initial survey
found 64 assertion sites across the game's 12 named units. Source-line bases
increase in code order in resmgr (598, 619, 639, 679), netlo (414, 538, 742)
and soundmgr (52, 605, 740, 808, 900, 1008, 1118). Wingraph shares base words;
[its diagnostic addends](wingraph.md) identify individual sites.

The original survey's blanket `/Od` claim was too broad. Current
`config/units.toml` contains measured profiles for different code families;
optimized BASE code and automatic-inlining controls are documented in
[sound evidence](sound-volume-control.md). Linker metadata and one exact
callback cannot establish every object's historical compiler or flags.
See [compiler evidence](../docs/compiler.md).

## Other releases

The 1997 Compendium PE build (`HEROESW.EXE`, linker 3.10) uses paths under
`F:\H1w95src\`. DOS releases use Watcom/DOS4GW LE images, with optimized
register calling conventions and no assertion paths in the surveyed builds.
They are secondary evidence, not interchangeable matching targets.
The [cross-release census](executable-function-census.md) records the inspected
Windows, DOS and Mac inputs, hashes, discovery methods and their limits.
