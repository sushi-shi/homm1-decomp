# Sample stream startup

Retail RVA `0x77cc0` contains `soundManager::StartSample`. The body and its
switch tables end at `0x77ff1`; the following fifteen `int 3` bytes are alignment.
The thiscall ABI consumes seven stack slots: mutable filename, output-buffer
pointer, two signed words, volume, channel selector and resume position. The
buffer pointer and first word are unused in this retail body. The actual
PlayAmbientMusic caller supplies channel selector zero. Only that branch
initializes the local channel; no default for other selectors is invented.

The stream-close branch expands StopSample, including the sample-zero test,
Miles end call, ten service/delay iterations, assertion, fclose and FILE reset.
Its shared source helper also appears in PlayAmbientMusic. Three reversed
filename characters select sample rate, bit depth and stereo. Both HoMM2
SAMPLE.cpp donors preserve this suffix protocol. Retail reverses the mutable
filename twice with `_strrev`; it does not copy or lowercase it.

The local path buffer starts eight bytes into the `0x168` local frame, leaving
352 bytes through the frame end. The declared capacity uses that observed span;
alignment means the original source capacity could be 349 through 352 bytes.
No surviving StartSample local declaration was found in the PoL debug records,
and its donor body is a stub. This is a used filename buffer, not extra padding.
Three `_access` calls are present: two on the sound-path candidate and one on
the data-path fallback. Retail calls fseek before testing fopen's result. These
behaviors are preserved. Access failure and null FILE share one error exit.

The volume conversion is the ordinary donor ConvertVolume helper, inlined for
music kind 101. It accepts configured volume 1 through 10, scales by
`(11 - configured) / 10`, enforces a positive minimum and clamps to 0..127.
Retail ModifySample at `0x78140` independently proves the two configuration
fields: music at gConfig+4 and effects at gConfig+8. Offset zero remains unknown.

The assertion word/file identities at `0xa1694` and `0xa1698`, sample-volume
array base `0xcc770`, sound-path base `0x91ea0` and data-path base `0x91be0`
are required by these code references. The full path/volume-array extents and
initializers remain unclaimed.

## CRT identities

RVA `0x8b070` reverses a NUL-terminated string with two converging pointers:
its VC4 symbol is `__strrev`. RVA `0x834c0` calls GetFileAttributesA, checks
the requested write mode against the readonly attribute and reports CRT errno:
it is `__access`, also independently labeled by the HoMM2 correspondence graph.
RVA `0x82040` is `_fseek`.

The previous `__purecall` identity at `0x834c0` was incorrect. The actual handler
at `0x83510` pushes runtime error 25 and calls `_amsg_exit` at `0x82860`.
The seven pure virtual slots at `0x8c5a0`, `0x8c5a4`, `0x8c5a8`, `0x8c678`,
`0x8c67c`, `0x8c680` and `0x8c68c` all point to `0x83510`. The identity was
corrected rather than relaxing call comparison.

## Shared volume helper and ModifySample

ModifySample at `0x78140` supplies an independent complete caller of both arms
of ConvertVolume. Its body and switch tables end at `0x78331`, before fifteen
alignment bytes. It scans all sample handles (the last duplicate wins), then
handles volume operations 1 and 100, start operation 5, or music volume 101.
The selector is a signed word and the value is a signed dword. Music mode
asserts `m_cdReady == 0`, not the later donor's global music-source setting.
Retail has neither the later logging calls nor its digital-driver guard.
Assertion line and filename are at `0xa16e8` and `0xa16ec`. Both volume paths
update the same existing word array after their Miles call. No unused local is
added from the later donor, since this optimized body provides no such census.

Initializing the found-channel result after the guards and limiting the index
declaration to the lookup loop produced an exact ModifySample compilation in
the integrated translation unit. Both normalized bodies have identical branch
destinations, switch tables, instructions and ordered referents. The frozen
source, headers, report and fingerprint are retained under ignored
`build/sol-integration/sample-family-exact/`. Later shared-header composition
changes only the loop's register allocation for the unchanged function source;
the audited peak is retained as MAX while CUR remains the current comparison.

The initial StartSample control used independent access and FILE failure
returns. An explicit common failure label moved the error tail and earlier
channel allocation away from retail; a braced positive arm was also tested.
Both diagnostic changes were removed. Failure-tail factoring and the allocator
remain open structural questions; the complete ordinary body is retained.
