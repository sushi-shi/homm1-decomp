# DirectDraw palette update

Retail RVA 0x4673 (0x11c bytes) was an existing empty mapped body, baseline14.25%. Reconstructed under the low graphics-family assignment.

Preferred Buka SOURCE/wingraph.cpp546-589 and secondary PoL439-478 preserve the same two-local body. PoL names the HRESULT result0 and retains diagnostic line-base +18/+22; Buka explicitly aliases result to result0 for retail compilation. Ordinary donor spelling result produced99.83% from swapped local slots; authentic result0 produces100% in the intended TU.

Retail guards busy and foreground, loops palette entries10..245, converts signed RGB components by shift2, flags PC_NOCOLLAPSE, calls ProcessAssert and IDirectDrawPalette::SetEntries slot6/offset0x18, then DDSD on failure. All78instructions,3calls,9branches,16relocations match. The pointer-to-int assertion conversion is the surviving donor interface contract.

IDirectDrawPalette is the genuine seven-method Microsoft interface transcribed from Buka build/toolchain/msvc/include/DDRAW.H665-677, external pure declarations only. Diagnostic base atVA0x48e8c0 is a signed word loaded before both diagnostics; this is an identity/layout claim, not initializer coverage. The source filename is HoMM1 retail D:\Heroes\Source\wingraph.cpp.
