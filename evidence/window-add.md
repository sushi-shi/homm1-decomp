# Window insertion

AddWindow RVA 0x74170 ends at 0x7423e; two padding bytes are excluded. Retail loads the requested order as a signed short, tracks the resolved order in BX and returns with twelve argument-slot bytes removed. Its third slot is forwarded as a dword to the existing Open(short,signed char) method, preserving the established callee identity. The manager declaration therefore recovers AddWindow(heroWindow*,short,int).

Buka WINMGR supplies the doubly linked insertion body. HoMM1 has an additional guard rejecting a zero layer when the list is already nonempty, alongside the donor's rejection of a nonzero layer on an empty list. Its fixed-layer flag forces zero, and requested order -1 resolves to the previous tail layer plus one. The retail sequence also proves the next/previous link directions and focus/active-window updates. No donor-only logic was imported.

The ordinary body under the pinned cpp_o2 profile currently scores 91.09. Both sides contain 77 instructions, one call, sixteen branches, one return and one call relocation. Remaining divergence is register allocation (retail BX/ESI/EDI/EBP versus candidate BP/BX/SI/DI), plus zero initialization selection; local declaration order matching the donor does not change emitted code. No artificial register controls or frame locals were retained.

## Fizzle source capture

SaveFizzleSource 0x746b0 has 0x88 actual bytes ending at 0x74738, excluding eight padding bytes. All four coordinates/dimensions are loaded as signed words and the method removes sixteen argument-slot bytes. The complete donor-backed delete, bitmap allocation and BlitBitmap body matches exactly under the existing optimized WINMGR profile. Retail's bShowIt guard is retained; later donor negative-coordinate and screen-bound clamping checks are absent. Existing bitmap virtual deletion, constructor and BlitBitmap identities are reused, with no new definitions or storage debt.

ReleaseFizzleSource 0x74a60 has 0x19 actual bytes and seven trailing padding bytes. Its donor conditional virtual deletion and source-pointer clear use existing reviewed member layout and destructor identity. It forms the capture/release lifetime pair.

## Fade screen

FadeScreen 0x745a0 has 0xbf code bytes and one padding byte. Both direction and increment are signed words, correcting the later donor's dword API; all three argument slots are removed on return. Retail asserts direction is zero or one through its signed-word assertion line base at 0xa0c80 and WINMGR path at 0xa0c84, optionally installs the supplied palette, and polls sound around the fade. Its saved update state at 0xcac20 is a signed byte. Fade-in saves the current flags, clears them, calls FadeIn, merges the saved global byte into the local byte with a compound OR and restores the result; fade-out saves to that global, clears flags and calls FadeOut. The full 191-byte C++ body is exact. The three global declarations admit identity/layout only, without initializers or data coverage.
