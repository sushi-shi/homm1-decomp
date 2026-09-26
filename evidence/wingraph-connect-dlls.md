# ConnectToDLLs (retail RVA 0x53bc)

The Buka 2.1 `SOURCE/wingraph.cpp` loader identifies the DirectDraw DLL,
`DirectDrawCreate` factory lookup, and `gbDDrawAttached` flag. HoMM1 retail
at RVA 0x53bc confirms the 0x6c-byte function, the Win32 `LoadLibraryA` and
`GetProcAddress` calls, and a failed lookup calling `ShutDown` with the retail
string `Error loading DDRAW.DLL`. Its code writes a 32-bit flag at VA
0x48e17c and a 32-bit factory pointer at VA 0x48e5ac. The typed pointer
follows the donor's `DirectDrawCreateProc` signature; VC4's available Windows
headers spell the `HRESULT` return as its underlying `long`.

The code-mode match is 100%. The two DATA rows claim code-use identities and
storage width only, not initial values or data-byte coverage.
