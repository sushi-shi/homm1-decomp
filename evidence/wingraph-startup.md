# InitGraphics (retail RVA 0x552c)

Preferred Buka 2.1 `SOURCE/wingraph.cpp` at VA 0x4b1911 supplies the DLL
connection, graphics-setting selection and backend initialization. Pinned
HoMM1 retail has the same sequence without Buka's debug LogStr calls.
The 0x60-byte candidate matches 100% in code mode.

Retail reads the selected graphics record at VA 0x4c6ad4 plus
`giCurExe*0x18` (selector VA 0x492e30). ReadPrefsFromFile at RVA 0x5cba1
passes VA 0x4c6aa8 to memset and fread with size 0x134, establishing the
actual gConfig owner base and persisted size. The prior census lump at
0x4c6a94 is the distinct gNextSoundPollTick, not the config owner.

Neighboring graphics/default functions confirm a six-word record (showMenu,
x, y, width, height, fullScreen), fullScreen at record offset 0x14,
config gfx offset 0x18, and a two-element array. Buka/PoL exeGfxConfig
has the same fields plus later colorMouseCursor, absent in HoMM1's stride.
The old, unused provisional config type's HoMM2-style sound offsets are
superseded by this retail-backed layout. PollSound retail references establish musicVolume at owner+4 and
musicSource at owner+0xb8. The prior standalone gMusicVolume/gMusicSource
identities are interior config fields; sound callers now use those fields.
Defaults at RVA 0x5c9fe write 2/3 to offset 0, whose name remains unrecovered.
No config initializer or data-byte coverage is claimed.
