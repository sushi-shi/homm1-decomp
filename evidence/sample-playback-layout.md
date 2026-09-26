# HoMM1 sample playback layout

Retail February 1996 HEROES.EXE is authoritative. `LoadPlaySample` (RVA
0x55932) stores playback channel group 2 at sample offset +0x1a and returns
`SAMPLE2` in EAX/EDX. The `MemorySample` referent (RVA 0x78f80) adds +0x0e
to the sample pointer before using the playback record: channel at record
+0x0c, sample rate +0x10, format +0x14, volume +0x18, loop count +0x1c.
The retail sample constructor (RVA 0x7fa60) stores its incoming arguments
at object +0x1a, +0x26, +0x2a and parses the playback rate at +0x1e and
format at +0x22. Thus the existing packed fields must be ordered as
active sample, data, size, channel type, sample rate, format, volume, loop
count. No new padding or initializer coverage is claimed.

Preferred Buka 2.1 `include/BASE/sampleData.h` has a changed playback
layout. Secondary PoL 2.0 `include/BASE/sampleData.h` preserves this exact
field order, and `src/SOURCE/KB.cpp` preserves the two-pointer `SAMPLE2`
LoadPlaySample source with channel group 2 and the MemorySample result.
The HoMM1 source uses these existing real fields directly.
