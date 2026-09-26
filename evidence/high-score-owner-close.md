# High-score owner and Close

Retail RVA 0x1228 (0x5d bytes) was unnamed in the initial snapshot. HoMM2 Buka SOURCE/HISCORE.cpp:51–56 and PoL corresponding Close provide the complete body: fade out eight steps, remove and delete owned window, deactivate.

HoMM1 baseManager is packed 0x30 bytes; high-score animation frames are ten shorts at +0x30, monster types ten shorts at +0x44, campaign-score flag byte +0x58, window pointer +0x59. These real members survive in both donor headers and are accessed by retail Open/Main/Update. HoMM1 adds a short at +0x5d: constructor sets 0x32f and Main at RVA0x138a tests it against the message type. m_dispatchMask is a descriptive inferred name for that real member. No padding is admitted. Class inheritance requires short Open(short) and short Main(tag_message&) matching the authoritative HoMM1 base virtual ABI. Existing mapped placeholder bodies receive signature-only correction.

Close compiled to exactly the retail 93 bytes, 34 instructions, three calls, one branch and five relocations. Initial 99.71 score was solely unlabelled FadeScreen745a0/operator delete805e0. Mirroring already reviewed root referent identities resolves both without changing body source; full build reports 100. No CRT body or data coverage is claimed. FadeScreen identity is corroborated by donor and adjacent retail callers; operator delete identity is already root-reviewed against its free wrapper.

The constructor is not claimed yet: its extra score-type flag at VA4c794c still needs domain/owner evidence. No artificial locals or storage were added.
