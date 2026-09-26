# Ambient music playback

Pinned retail RVA `0x786d0` ends at `0x7893e` (extent `0x26e`); the two
following INT3 bytes belong to alignment. Buka 2.1 and PoL 2.0 establish the
sound-manager family, but their later music backends differ. The HoMM1 body
is recovered from the pinned instructions, not copied from a later backend.

The entry guards read sound disable, music readiness (`+0x67e`) and sample
readiness (`+0x38`). CD readiness (`+0x694`) selects CDPlay; the other arm saves
eligible track positions with ftell, ends the active stream, services sound
while waiting, closes its FILE, selects an 82m/82s/62s filename, and starts a
sample. The filename occupies 40 stack bytes and the start call receives an
address of a local data pointer. The loop flag is an unsigned byte widened
to the short parameter home; volume defaults to 64 or one during fading.

The result from StartSample at `0x77cc0` is stored at `this+0x34`, proving
that field is a SAMPLE pointer. The previously provisional integer spelling
is replaced with `m_activeSample`. No object offsets or data bytes change.

The StopSample sequence is present both in this body and in StartSample,
and separately at `0x780f0`: sound-disable guard, comparison against the
first sample handle, AIL_end_sample, then ten service/delay iterations for
that handle. PoL retains the helper boundary; HoMM1 lacks its digital-driver
guard and log calls. ServiceSound is the already recovered inline helper.

DelayMilli at `0x644c4` calls KBTickCount, adds its long argument, and calls
DelayTilMilli. Both donor NOOPT headers give the same cdecl long signature.
New code-required data identities are the signed assertion line word at
`0xa1728` and terminated filename at `0xa172c`; initializer matching remains
deferred. Existing ftell/fclose, CDPlay, MCI and Miles identities are reused.

The ordinary candidate reproduces the retail extent, instruction count,
ordered referents, and every branch address and destination. The remaining
differences are argument registers and instruction scheduling. Moving the
StopSample flag initialization below its early guard follows the surviving
donor source and improves the generated code. A bounded compiler-state
experiment found one state across all trials and no exact closure; it retained
no probe source and changed no score ledger. Source-level lifetime and caller
composition remain open, rather than being declared a bounded wall.
