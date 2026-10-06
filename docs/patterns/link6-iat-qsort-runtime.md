# LINK 6.00 import order follows the C runtime's `qsort`

Measured with the pinned VC6 SP5 `LINK.EXE` (6.00.8447) under wine, on
HoMM1 Buka's `HEROES.EXE` and `EDITOR.EXE`.

## Signature

Two links with the same objects, libraries and member pull order (equal
import thunk order in `.text`, equal `.idata$6` hint/name order) produce
different ILT/IAT orders within each DLL. The hint/name strings stay in pull
order; only the `.idata$4`/`.idata$5` slots are permuted.

## Observations

- `LINK.EXE` imports `qsort` from `MSVCRT.DLL`. Linking one object that
  references `n` imports of one DLL: for `n` up to 7 the first import moves to
  the end of the IAT, from 8 on the IAT is in reference order. That is the
  median-of-three `qsort` (wine's builtin `msvcrt`) on equal keys, small
  partitions by selection.
- One model reproduces every tiny link (one to three DLLs, one to three
  objects, imports pulled in one or two passes) and every editor slot: the
  array is the imports in pull order with the DLL's null thunk after the
  DLL's first pull run; the key is the DLL alone, so imports of one DLL compare
  equal and their order is whatever the sort leaves.
- The editor's retail IAT is the same array sorted by the VC6 runtime's
  middle-pivot `qsort`; the game's retail IAT is the median-of-three one's.
  Running the editor's LINK against the VC6 SP5 `MSVCRT.DLL` makes every slot
  equal retail; the game links byte-identically against wine's builtin.
- A native `MSVCRT.DLL`'s `time()` converts the wine session's local time with
  its own `TZ` parse. With LINK's clock frozen at the editor's retail link
  time, the stamps (header, debug directory, PDB signature) moved with the
  wineserver's zone (a Europe/Warsaw server: +2 h, America/New_York: -4 h,
  both in daylight time) and, independently, with the linker process's `TZ`
  (Europe/Warsaw: -1 h, America/New_York: +5 h; the VC6 parser does not read
  zoneinfo names). Only a UTC server and a UTC linker reproduce retail.
- The server's zone is that of whichever wine process started it: a client
  that finds no server starts one with its own environment. Every wine
  process the tooling starts therefore runs with `TZ=UTC`
  (`homm1.tool.wine.wine_env`); a starting `wineserver` cannot take over a
  running one, so a server in another zone is waited out, and the candidate
  link fails when its stamps differ from retail's.

## Use

When an IAT is a permutation of retail's but thunks and hint/name strings
already match, the linker's C runtime is the lever, not the library line or
the objects: unused libraries, extra non-import symbols, path lengths,
duplicate references and `/OPT:NOREF` leave the order unchanged in these
measurements.

## Does not establish

The key's order between DLLs (the editor's slots fit ADVAPI32, GDI32,
KERNEL32, USER32, audiere, WING32, which is not their name order) and which
Windows release each original link ran on.
