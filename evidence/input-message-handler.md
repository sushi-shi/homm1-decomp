# INPUTMGR mouse-message handler

`MouseMessageHandler` is the 0x280-byte function at VA `0x0047BE30` in
`BASE/INPUTMGR`.  Its seven-entry switch covers the contiguous Win32 mouse
message range `WM_MOUSEMOVE` through `WM_RBUTTONDBLCLK`.  The event codes,
capture calls, coordinate conversion, 32-entry ring-buffer update, and
`mouseManager::SetPointer(1000)` call agree with the corresponding Buka 2.1
and PoL 2.0 implementations.  Buka 2.1 is the primary design donor.

The retail accesses establish the HoMM1 layout used by the implementation:
the active flag is at `inputManager+0x2E`, the 16-byte event ring begins at
`+0x30`, the read and write indices are at `+0x230` and `+0x232`, the mouse
handler guard is at `+0x234`, and the modifier word is at `+0x344`.

The code references four TU-owned initialized-data records directly.  Retail
bytes at VA `0x004A1A40` contain the little-endian short 137, which the call
adds to 50 before passing it to `ProcessAssert`.  It is followed by two
separate 24-byte slots at VAs `0x004A1A44` and `0x004A1A5C`; each contains
`ReleaseCapture Failed` plus its terminator and alignment.  The 28-byte slot
at VA `0x004A1A74` contains `D:\\Heroes\\Base\\INPUTMGR.CPP` and its
terminator.  Separate arrays are required because the two call sites have
distinct retail addresses even though their text is identical.

The handler's repeated absolute pointer accesses identify `gpInputManager` at
VA `0x004C517C`.  Its guarded cursor call identifies `gpMouseManager` at VA
`0x004C6714`; that call resolves to the already matched
`mouseManager::SetPointer` function.

Two adjacent methods retain the donor implementation without adaptation.
`inputManager::Flush` at VA `0x0047C200` writes zero to the queue indices at
`+0x232` and `+0x230`; its body ends at the `ret` after 17 bytes.  At VA
`0x0047C300`, `inputManager::SetKeyCodeType` stores the low word of its integer
argument at `+0x340`, then emits the same two queue-index stores because VC4
inlines `Flush`.  Its body is 31 bytes through `ret 4`.  The following `int 3`
bytes belong to linker padding and are excluded from both function claims.
