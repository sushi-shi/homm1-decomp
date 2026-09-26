# Adventure reseed code-use identity

Pinned February 1996 `HEROES.EXE` function RVA `0x27CFE` has seven calls from
`advManager` methods. It takes two stack arguments (`ret 8`) and writes a
zero dword to VA `0x004C5170` (RVA `0xC5170`) before returning. HoMM2 Buka
2.1 `src/SOURCE/ADVMGR.cpp` places `advManager::Reseed(i32, i32)` between
`Main` and `ProcessSelect` and its complete body sets `giSeedingValid = false`.
The corresponding HoMM1 function has exactly that position and code shape.

The census row and `DATA` annotation identify only the four-byte global used
by this code. They make no initializer or data-byte coverage claim.
