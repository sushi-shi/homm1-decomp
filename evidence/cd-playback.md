# Retail CD playback and helper recovery

`HEROES.EXE` RVA `0x771f0` is `soundManager::CDPlay(int,int,int,int)`:
its four stack arguments and `ret 0x10` distinguish it from manager startup.
The final return ends at `0x777a7`; nine following `int3` bytes are alignment,
so the source claim is `0x5b7` bytes, not the coarse census span `0x5c0`.

PoL 2.0 `src/BASE/soundmgr.cpp` preserves the CDPlay, HandleMCIError,
ValidatePreviousPosition, CDSetVolume and ServiceSound family. Preferred
Buka 2.1 has replaced this playback path with a different sound backend.
HoMM1 retail expands error handling five times, sample service three times,
position validation once and volume handling at the final two branches.
Ordinary inline definitions recover those boundaries under the pinned /O2
profile. The retail call at `0x77415` uses the VC4 `strcmpi` compatibility
entry (candidate COFF `_strcmpi`); the spelling `_strcmpi` would select the
separate double-underscore decorated entry instead.

HoMM1 has its own logical-to-CD track mapping and notification ranges. It
normalizes default volume before recording playback state, has no CD-ready
check in this body, and lacks the donor's later special last-disc-track arm.
The notification local is an unsigned byte: retail zero-extends it for
`CDPlayOnce = 1 - notify`. Its signed version changed one instruction and
one byte of extent; the unsigned version restores the instruction census.
The MCI error text is read directly at retail RVA `0xa13f4`.

Code-required storage identities (no initializer coverage):

| RVA | Entity | Extent evidence |
| --- | --- | --- |
| `0xa1000` | `CDPreviousPosition[60][15]` | indexed string records; validator limit 60, stride 15 |
| `0xa1384` | `CDPlayOnce` | dword assignment |
| `0xa138c` | `CDPlaying` | dword test/store |
| `0xa1390` | `CDTrackMap` | signed byte indexed loads; full extent deferred |
| `0xa14f8` | assertion line base | signed word load, plus four |
| `0xa14fc` | assertion filename | retail terminated filename |
| `0xcc668` | `lpszReturnString` | MCI result buffer, capacity 256 |
| `0xcc768` | `nMCIError` | MCI return dword |
| `0xcc7b0` | `CommandString` | MCI formatted command buffer |

The recovered class fields already have retail-backed offsets: current track
`+0x572`, fade steps `+0x682`, CD track `+0x68a`, playback volume `+0x68e`,
and auxiliary device `+0x692`. Music volume and source are fields of the
shared `gConfig` owner; no parallel storage is introduced.

The required CDStop callee at `0x76f00` ends at `0x7710e`, before two
alignment bytes. It reuses HandleMCIError and ValidatePreviousPosition.
Compared with PoL's later version, retail sends `stop CD` without `wait`,
does not check CD readiness or a nonnegative current track, and indexes
saved positions through the signed-byte CDTrackMap. Those differences are
retained; the later donor's additional guards are not introduced.
