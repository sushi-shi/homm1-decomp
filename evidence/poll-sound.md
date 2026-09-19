# Incremental PollSound evidence

Target: pinned February 1996 Win95 HEROES.EXE. Addresses below are RVAs unless
prefixed with VA. These names are recovered correspondences, not original PDB
symbols. No whole-TU ownership or data-byte match is asserted.

Reference: HoMM2 `decomp-gold-2.1-buka`, commit
`299514f88900c0cf30ba03422c72830a38fc1cb7`, `src/SOURCE/KB.cpp`
(`PollSound` at VA 0x465BF0 and `ForcePollSound` at VA 0x465D05),
`src/SOURCE/kbwin.cpp` (`AppAbout` and `KBTickCount`), and their headers.

## Identity and extents

- AppAbout ends by calling VA 0x44F640. HoMM2's matching callback calls
  `PollSound` in the same position after the dialog-message switch.
- RVA 0x4F640 has a signed timer comparison, byte re-entry guard, 30 ms
  deadline update, foreground-conditional sound-manager member call, remote
  polling call, and guard reset. It ends with `ret` at 0x4F6B1: size 0x72.
- The next entry, RVA 0x4F6B2, sets the same deadline to `KBTickCount() - 1`
  and calls PollSound. Its `ret` at 0x4F6D1 gives size 0x20. HoMM2's adjacent
  `ForcePollSound` has precisely this behavior.
- The following entry at 0x4F6D2 starts a distinct SEH frame. It is not included.

HoMM1 runs the service when `now >= deadline`, then tests its guard. HoMM2's
later implementation tests the guard first, uses `now > deadline`, and also
services mouse/color timers. Those later behaviors are not copied to HoMM1.

## Minimal reference identities

| RVA | Recovered role | Evidence |
| --- | --- | --- |
| 0x5DC9B | `KBTickCount` | Wrapper calls the GetTickCount IAT entry at 0xD6478. |
| 0x78940 | `soundManager::PollSound` | ECX loaded from the manager pointer before the call; callee reads instance state and polling timers. HoMM2 provides the name correspondence. |
| 0x5A584 | `PollRemote` | Following unconditional service call; callee checks networking state and dispatches transport-related work. Name follows HoMM2; body remains unclaimed. |
| 0x94180 | `gbInPollSound`, plain char | `movsx` byte read, byte stores of 1 and 0. |
| 0x9FE78 | `gbForegroundApp`, int | Dword zero comparison gates sound-manager polling. |
| 0xC6A94 | next sound-poll deadline, long | Signed compare with KBTickCount and dword writes of now+30 / now-1. |
| 0xC7CA8 | `gpSoundManager`, pointer | Dword load into ECX preceding the member call. |

Only the four accessed storage extents are admitted. The timer's enclosing
array and the sound-manager layout remain unresolved; the deadline slot has
the descriptive local identity `gNextSoundPollTick`. HoMM2's larger timer array
does not by itself prove HoMM1's complete array bounds or ownership.

The sound-manager declaration exposes only a method ABI. Its unresolved layout
is registered in the cleanliness configuration; allocation, by-value use,
sizeof and pointer arithmetic require further evidence.

## Compiler implications

The two small KB functions compile directly under the existing VC4 `/Od`
profile. The called sound-manager body at 0x78940 visibly has a different,
optimized-looking prologue. This is further reason not to generalize the
fragment profile to all game code.

All call/fixup rows were decoded from original retail instructions and checked
against encoded operands. Exactness includes those operands and their reviewed
site/type/symbol/addend identities. No data initializer is scored.
