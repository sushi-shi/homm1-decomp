# Sound volume and quality control

The contiguous retail routines DigitalReport `0x78340`, AdjustSoundVolumes
`0x78390`, AdjustMusicVolumes `0x78450`, and SetMusicQuality `0x785c0` have
actual extents 0x4d, 0xbd, 0x16c and 0x104. Trailing INT3 alignment is excluded.
Buka and PoL soundmgr supply the method family; retail controls the older
streaming implementation and the guards that differ from both later releases.

DigitalReport takes a sample handle and a signed word query (volume 1 or playing
4), returns a long, and guards only gbNoSound. AdjustSoundVolumes expands this
helper, including the constant switch comparisons, then calls ModifySample for
each effect handle beginning at index one. Its readiness guard is the DWORD at
this+0x67e (m_musicReady), not the later donor's digital-driver guard.

AdjustMusicVolumes uses that same readiness flag. A byte local selects saved
positions for tracks below seven or equal to 47 or 49; track 48 is absent from
this routine's retail condition. Unmuting sets the CD volume or sample volume
and calls PlayAmbientMusic with the selected saved position. Muting CD calls
the same CDSetVolume helper; muting a digital track saves ftell when selected,
then sets sample volume to zero. Both CDSetVolume expansions use the already
recovered helper. Assertion references prove the word at 0xa1708 and source path
at 0xa170c. Their declarations admit identity and extent only.

SetMusicQuality requires samplesReady, nonzero configured volume and cdStarted.
It stops the current CD or streaming sample, conditionally closes the file,
clears all sixty saved positions, stores the new source, updates cdReady from
source==2, and restarts the previous valid track. Its StopSample expansion has
the same ten one-millisecond service waits already recovered in other callers.
No later MIDI implementation or additional donor guard was imported.

## Automatic-inlining control

Three real-TU VC4 controls establish the sound unit's automatic-inlining
requirement. Under the earlier `/O2` profile, explicit inline DigitalReport
produces an exact AdjustSoundVolumes but omits its standalone definition.
Removing explicit inline produces the exact standalone 77-byte routine, but
retains a call in AdjustSoundVolumes that retail expands. Adding `/Ob2` to the
otherwise identical pinned profile emits both ordinary definitions and matches
both retail bodies exactly, including the updater's constant query switch.
The resulting `cpp_o2_inline` profile applies only to BASE/soundmgr. It reuses
VC4 flags already exercised by the codec profile; this is a retail-backed
sound-specific control, not a transfer of later compiler heuristics.

The complete build and source gates pass under this profile. SetMusicQuality
has matching instruction, call, branch, return and relocation counts, with an
open register-allocation difference. A bounded 32-state experiment reached
99.7590 only diagnostically; no probe score was banked and source was restored.
AdjustMusicVolumes subsequently reached exact code equality in the ordinary
integrated TU with its existing source unchanged, establishing that its earlier
helper-tail difference was sensitive to compiler state. No helper distortion or
generated declarations were needed.
