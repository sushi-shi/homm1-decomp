# HoMM1 executable function-body census

Measured 2026-09-26 with Ghidra 12.1.2 and Capstone 5.0.7. This compares
**estimated retained out-of-line bodies**, not the number of source functions
which were never inlined. A function can have a standalone body and also be
inlined at some call sites. Static runtime/library code is included; this is
not yet a census of corresponding game functions.

| Executable | Estimated bodies, excluding stubs/thunks | Bodies reached by a direct call |
| --- | ---: | ---: |
| Windows game, 1996-02-01 (pinned) | 1,228 | 1,117 |
| Windows game, 1997-08-29 (Compendium) | 1,237 | 1,122 |
| DOS German retail, 1995-10-09 | 1,515 | 1,232 |
| DOS English 1.2 patch, 1995-10-12 | 1,501 | 1,219 |
| DOS demo 1.2, 1995-11-28 | 1,488 | 1,205 |
| DOS Compendium, 1997-08-01 | 1,486 | 1,203 |
| Mac PowerPC 1.0, file date 1996-09-05 | 1,271 | 1,160 |
| Windows editor, 1996-02-01 | 598 | 535 |
| Windows editor, 1997-07-30 | 611 | 540 |

The Mac PowerPC result is 43 bodies (3.50%) above the pinned Windows result.
Both Windows games have 1,247 automatically discovered internal function
entries before exclusions. Mac has 1,534, but 256 of these entries are CFM
import stubs and seven are forwarding thunks. Counting those stubs as game
helpers would substantially exaggerate the Mac advantage.

These totals do not establish that Mac retained 43 additional shared-source
helpers: different platform code, runtime libraries, game revisions and
boundary-discovery errors also affect the counts. Likewise, the higher DOS
counts do not establish less inlining. The editors have different feature
coverage and are shown separately from game comparisons.

## Method and limits

- PE and PEF files were imported into fresh Ghidra projects with default
  analysis, two analysis CPUs and a 180-second per-file timeout. All analyses
  completed without reaching the timeout. No reconstruction labels or
  hand-admitted starts were applied.
- The export counts internal function entries, excluding external function
  placeholders. Ghidra forwarding thunks are then removed. The second column
  counts distinct remaining entries targeted by a decoded direct call. It
  omits bodies reachable only through indirect calls, callbacks or tail jumps.
- Mac import glue was additionally identified by its six-instruction sequence
  and the PEF loader's imported transition-vector references. All 257 import
  stubs resolve; 256 were present in the automatic function inventory. The
  inspection reused HoMM3's PEF/loader/glue readers from local revision
  `d17b60bc3`. HoMM1's loader has a nonzero reserved relocation-header halfword
  (`0x00d4`); this field was ignored in an in-memory parsing copy. Original
  bytes, hashes and Ghidra inputs were preserved.
- DOS LE images were decoded through their bound MZ header, object table and
  page map. Only executable object 1 was imported, at its declared base
  `0x10000`; the DOS extender stub was excluded. Capstone linear-disassembly
  direct-call destinations that also appear as instruction boundaries, plus
  the LE entry point, seeded Ghidra disassembly and function creation. LE
  fixups and the non-executable data object were not loaded. These counts are
  less certain than the native PE/PEF results: embedded data may cause false
  seeds and indirect-only functions may be missed. They are not suitable for
  admitting reconstruction boundaries.
- As a check on automatic discovery, the pinned executable has 1,247 internal
  entries in this run versus 1,250 non-padding starts in the current manually
  owned retail partition. This difference is retained, not silently corrected.
- The Mac application also contains 68K CODE resources. This table measures
  its PowerPC slice only. The 68K slice has not received a comparable analysis.
- Other Archive.org listings (OEM, Platinum, Windows 1.1 patch, etc.) have not
  all been extracted and deduplicated; this is the locally available corpus
  plus the newly extracted Mac application, not every published release.

## Mac provenance and reproducibility

Source: [Archive.org Macintosh disc](https://archive.org/details/***REMOVED***),
`Heroes of Might and Magic (USA).zip`, archive SHA-1
`ccfe9e0819d7bff05e7f115818dff789e7e2c9bb`.

The disc's Smaller Installer 2.0.1 payload is a Compact Pro catalogue with a
different wrapper. A temporary standard Compact Pro wrapper preserves all
compressed streams and their offsets; `unar` extracts both application forks.
Their combined raw CRC-32 is `f3df8c72`, matching the installer catalogue.
The included readme identifies version 1.0, September 4, 1996; the application
file timestamp is September 5, 1996. The Archive.org item's nominal 1995 date
does not identify this build.

| Input | Size | SHA-256 |
| --- | ---: | --- |
| Mac PowerPC data fork | 569,600 | `cb11c4bef14f8c68138aa8ba0956c7e972ae9dae624b4fddc707f74f00efc666` |
| Mac resource fork, without AppleDouble wrapper | 853,397 | `636930ab4b7892e7a3f0730e672ddcfec6f175acdc65bd4a01d2dc2b63dfca23` |

Ignored working files are under `build/research/function-census/`:
`summary.json` records every executable hash, counts and report filename;
`Census.java` exports Ghidra functions and call sites; `prepare_dos.py` and
`SeedDos.java` record the DOS discovery method; `summarize.py` applies the
exclusions. Projects, logs, extracted forks, installer catalogue and the
temporary extraction script remain there. No source claims, build contracts
or matching tooling were changed for this research.
