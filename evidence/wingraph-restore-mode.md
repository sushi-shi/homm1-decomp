# DDRestoreDisplayMode (retail RVA 0x3640)

Buka 2.1 SOURCE/wingraph.cpp supplies the guarded DirectDraw restore and
error check. PoL 2.0's same function retains `gDDRestoreLineBase + 7`, which
matches HoMM1's signed word load at VA 0x48e5c4. The retail error filename
is `D:\Heroes\Source\wingraph.cpp`; the donor filename is secondary.
The one-word result local and COM call at vtable offset 0x4c reproduce
the 0x59-byte retail function exactly in code mode (100%).

The external IDirectDraw interface is transcribed from the donor Microsoft
SDK DDRAW.H at build/toolchain/msvc/include/DDRAW.H, lines 491–518.
All 23 real interface operations remain in SDK order. RestoreDisplayMode is
slot 19; no game implementation, artificial slots, or object storage is
introduced. Forward-declared descriptor/caps records are pointer-only.

Code-use DATA identities are lpDD at VA 0x48e5b0 (four-byte interface
pointer) and gDDRestoreLineBase at VA 0x48e5c4 (two-byte signed word).
Initializer/data-byte coverage is deferred.
