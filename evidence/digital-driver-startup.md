# Digital-driver startup

WAVE_init_driver at RVA `0x77870` ends at `0x77963`; the following thirteen
INT3 bytes are padding. Its sole caller is soundManager::Open at `0x77970`.
The preferred Buka source no longer has this Miles helper; PoL soundmgr.cpp
preserves the complete older routine and its unsigned rate/word arguments.

Retail probes the device count, queries a 52-byte WAVEOUTCAPSA, fills the
16-byte PCMWAVEFORMAT at `0xcc8e0`, and calls AIL_waveOutOpen. The native VC4
SDK records supply both layouts. Device zero and the exact two error dialogs
are independently visible in retail. The later donor's optional wave-output
preference branch is absent here and is omitted. The driver output pointer is
the remaining four bytes in the observed 56-byte local frame.

Sample rate is unsigned long; bit depth, channel count and error-display flag
are unsigned words. The retail right shift is logical, and the byte rate uses
both word inputs and the full dword sample rate. Code references establish the
wave-format object's identity and layout, without data-initializer coverage.

## Sound manager lifecycle

Open at `0x77970` ends at `0x77b2c`; four alignment bytes follow. The virtual
entry returns a signed word and consumes one word-typed stack slot. Retail uses
F6/F7 to select digital source 0 or CD source 2, records that selection in
`m_cdReady`, starts Miles, opens CD audio and allocates two 0x4000 stream
buffers. A failed digital driver disables sound, frees both buffers and shuts
Miles down. Success allocates the sample handles, clears the streaming state and
sixty saved positions, and enables fading. Both paths finish manager setup.
The larger later donor's MIDI fallback and track-flag tables are absent here.

The expanded AllocateSampleHandles loop visits indices 0 through 14. It proves
fifteen pointers beginning at `this+0x4e`, followed by an unresolved four-byte
slot at `+0x8a` and the existing count at `+0x8e`. The former fourteen-element
declaration came from the later donor. No unknown slot is given invented
semantics. The helper also clears the current music sample in this release.

CDStartup at `0x77110` ends at `0x771e7`. It formats the retail CD-open command
using `gcSoundPath[0]`, records MCI failure by clearing the started/selected
state and writing preferences, then searches Win32 auxiliary-device capabilities
for AUXCAPS_CDAUDIO. The actual 48-byte AUXCAPSA is at `0xcc8b0`, immediately
before the wave-format record. `m_cdStarted` at `this+0x698` is independently
written here and read by shutdown. The return carries the global unsigned MCI
error code on the successful path and zero on disabled/error paths; an unsigned
long return models that observed value, unlike the later donor's void routine.

Close at `0x77b30` ends at `0x77ca9`. The expanded CDShutdown helper is guarded
by `m_cdStarted`, sends stop and close commands, and uses the same real MCI error
handler already recovered in CDPlay/CDStop. Close first shuts down Miles,
closes CD audio, clears manager activity, disables sound, and frees both stream
buffers. Main at `0x77cb0` is the complete six-byte zero-result message handler;
the following ten INT3 bytes are excluded. These are whole retail bodies, not
placeholder lifecycle stubs.

The initial census omitted the six-byte Main entry at `0x77cb0`. Retail's
relocated vtable slot `0x8c5e8` points directly to it, corroborating the complete
`xor ax,ax; ret 4` body between Close and StartSample. The row is now admitted
in the retail census; no extent or admission check was bypassed.

The complete WAVE helper's first residual was register allocation, with equal
80-instruction, six-call, four-branch, four-return and eighteen-relocation
structure. A bounded 32-state VC4 experiment found an audited exact state at
trial 13 for unchanged function fingerprint `3e3a5fed5cdc`. The proof bundle is
`build/tu-state-noise/20260927-013343-BASE-soundmgr-0x77870/`: retail and candidate
normalized objects, compiler flags, original source, state manifest and trial
scores. `permute state --record-max` retained the exact peak after restoring
source. No generated declarations remain in the reconstruction. The analogous
CDStartup experiment reached only a sub-exact diagnostic state; it did not alter
the ledger or establish a bounded source wall.
