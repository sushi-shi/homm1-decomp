# HoMM1 window layout evidence

The February 1996 retail `WINDOW` bodies retain the HoMM2 window algorithms
but use the earlier 16-bit geometry and state layout.  Direct member accesses
establish the following `heroWindow` offsets:

| Offset | Member |
| ---: | :--- |
| `0x00` | z order (`short`) |
| `0x02`, `0x06` | next and previous window pointers |
| `0x0a` | 20-byte name |
| `0x1e`, `0x20` | flags and state (`short`) |
| `0x22`..`0x28` | x, y, width, height (`short`) |
| `0x2a`, `0x2e` | widget tail and head |
| `0x32` | saved-background bitmap |

The constructor at VA `0x00473f90` and `Open` at VA `0x00473fe0` establish
the `heroWindowManager` prefix inherited from the 0x30-byte `baseManager`, its
window pointers at `0x30`..`0x3c`, and the screen bitmap at `0x42`.
`gpWindowManager` is the pointer stored at VA `0x004c7c9c`; the WINDOW methods
reference that exact slot for screen updates and bitmap access.

The method bodies were ported from HoMM2 Buka 2.1, checked against PoL 2.0,
and adjusted only where the HoMM1 instructions prove the older field widths or
behavior.
