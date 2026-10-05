# Version lineage

Heroes of Might and Magic for Windows shipped in four executable releases.
Each was reconstructed in turn: the source of one release is the starting
point for the next, and the differences below are those observed between the
pinned retail images. Build details for every executable (and the DOS
releases) are in [builds](../builds.md).

```text
Win95 1.0 (Feb 1996) -> Win95 1.1 (May 1996) -> Win95 1.2 (Aug 1997) -> Buka 2003 (Russian)
VC4.0                   VC4.0                   VC4.1                   VC6 SP5
HEROES.EXE              HEROES.EXE              HEROESW.EXE             HEROES.EXE
```

Each release also shipped a scenario editor (`EDITOR.EXE`, `EDITORW.EXE` for
1.2) built with the same compiler.

| Step | Detail | Address correspondence |
| --- | --- | --- |
| 1.0 → 1.1 | [Win95 1.1](win95-1.1.md) | [`win95-1.0-to-1.1.tsv`](../../config/retail/versions/win95-1.0-to-1.1.tsv), [review](../../config/retail/versions/win95-1.0-to-1.1-review.tsv) |
| 1.1 → 1.2 | [Win95 1.2](win95-1.2.md) | [`win95-1.1-to-1.2.tsv`](../../config/retail/versions/win95-1.1-to-1.2.tsv), [review](../../config/retail/versions/win95-1.1-to-1.2-review.tsv) |
| 1.2 → Buka 2003 | [Buka 2003](buka-2003.md) | [`win95-1.2-to-buka-2003.tsv`](../../config/retail/versions/win95-1.2-to-buka-2003.tsv) |

PE inventories (hashes, sections, imports) of the 1.2 and Buka images and
their DLLs are in [`images.json`](../../config/retail/versions/images.json).

## Win95 1.0 (February 1996)

The first Windows 95 release, built with VC4.0 using incremental `/Gi`
compilation, Miles (`WAIL32.DLL`) audio, Smacker (`SMKWAI32.DLL`) video and
WinG.

## Win95 1.1 (May 1996)

Same compiler and runtime libraries. Behaviour changes
([details](win95-1.1.md)):

- Network chat moves from F1 to F2 (combat and while waiting for a player).
- Town hover also recognizes the adjacent cells' secondary trigger.
- Hover refreshes after loading a remote turn and at a local human's turn.
- Identify Hero expires on every player change instead of daily.
- Restoring saved setup gives later human players with no handicap the first
  player's handicap.
- Erasing a map object also clears its animation flag.
- Combat flushes queued input and resets the mouse after choosing its music.
- Serial-port failures report the Windows error through a new
  `ShutdownComError` and shut down.
- `CDStop` waits for the MCI stop to complete (`stop CD wait`).
- Ultimate-artifact spread rolls run 30, 20, 20 instead of 20, 20, 30: same
  distribution, different result for a given RNG state.
- AI resource and event valuations accumulate their floating terms in a new
  order, which can change rounding.
- Loading message and About dialog say "version 1.1".

## Win95 1.2 (August 1997)

Rebuilt with VC4.1 as `HEROESW.EXE`; link order changed throughout. Behaviour
changes ([details](win95-1.2.md)):

- CPU detection asks Windows (`GetSystemInfo`) instead of probing EFLAGS/CPUID.
- Smacker moves to `SMACKW32.DLL`: palettes are copied and converted from 8 to
  6 bits per component by a new helper, and the movie preload flag changes.
- Miles moves to `MSS32.DLL` with the waveOut backend; stopping all samples
  releases and reacquires the digital device (up to 20 tries, 5 ms apart), and
  `StopSample` no longer services the music stream.
- Streamed samples are fixed to signed mono 8-bit PCM; the filename still
  selects the rate.
- NetBIOS send processing logs timestamped diagnostics.
- Resources are unchanged; the About dialog still says "Version 1.1".

## Buka 2003 (Russian Platinum edition)

Rebuilt by Buka with VC6 SP5, `/FIXED`, Russian text and Audiere audio.
Behaviour changes ([details](buka-2003.md)):

- All text in Windows-1251 Russian, with CP1251-aware capitalization, plural
  creature names in messages, a remapped font and a CP1251 keyboard table.
- Audiere replaces Miles and CD audio: Ogg music with a boolean source
  setting, louder samples (127 instead of 64) and new ambient volumes.
- Smacker playback gets its own loop and a Buka logo before the intro.
- Registry key and values renamed (`HMM1 ...`); the configuration loses
  `cdOffset` and `slowVideo`, so walk speed and AI step limits no longer
  depend on the CPU.
- The disc check looks for `Tracks\02-AudioTrack 02.ogg`.
- Gameplay fixes and changes: tavern animation timing, an artifact-swap guard,
  the adventure Close help text, hero-window skill labels, archive directory
  entries, the `MAKEFILEID` hash and a different random-call order for map
  generation.
- NWC diagnostic logging is removed.
