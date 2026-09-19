# `BASE/MOUSEMGR`

## `mouseManager::SetPointer`

HoMM1 retail VA `0x00476940` follows the HoMM2 Buka 2.1 and PoL 2.0
`SetPointer` implementation: it selects one of 75 cursor slots, loads a 32 by
32 cursor bitmap through `resourceManager`, builds the AND and color bitmaps,
creates the Win32 cursor, and caches its handle. The donors establish the
operation and source-unit ownership; HoMM1's instructions establish its exact
types, globals, branch order, and constants.

The function takes a `short`. Its parameter is loaded and stored with 16-bit
instructions, and the `1000` sentinel retrieves the 16-bit cursor-frame member
at `mouseManager + 0x3C`. The active-manager test reads the inherited
`baseManager` state at `+0x2E`, which also proves the class inheritance and the
three pointer-sized members preceding the cursor frame.

The cursor bitmap setup uses the VC4 SDK `BITMAP` and `ICONINFO` layouts. The
hotspot table is unsigned: retail clears `EAX` and then loads each coordinate
into `AL`, producing zero extension. Exact relocation aliases record retail's
interior field addresses as their owning array symbols plus SDK field addends;
these aliases reproduce the compiler object's original relocation spelling
without claiming the data contents.

The 1,161-byte body ends at VA `0x00476DC9`. The following seven `int 3` bytes
are linker fill and are excluded from the function claim.

## `mouseManager::MouseCoords`

HoMM1 retail body VA `0x00476E60` is the HoMM2 `BASE/MOUSEMGR`
`MouseCoords` operation. Both HoMM2 Buka 2.1 and PoL 2.0 call
`GetCursorPos`, convert the point with `ScreenToClient`, then scale the client
coordinates to the 640 by 480 game surface.

HoMM1 differs in two details proved directly by its instructions:

- stores at VAs `0x00476E95` and `0x00476EB1` are 16-bit stores, so the two
  output references are `short&` rather than the HoMM2 donor's `int&`;
- the function uses an 8-byte stack `POINT`, while Buka 2.1 uses a static
  point.

The relocation operands establish the three external data identities needed
by this source body:

| Retail VA | Identity | Use |
| --- | --- | --- |
| `0x0049FE74` | `hwndApp` | first argument to `ScreenToClient` |
| `0x004CA900` | `iMainWinScreenWidth` | divisor after multiplying X by 640 |
| `0x004CA4A8` | `iMainWinScreenHeight` | divisor after multiplying Y by 480 |

The 90-byte body ends at the `ret 8` at VA `0x00476EB7`. The six following
`int 3` bytes are linker fill and are excluded from the function claim.

The body is frame-pointer-omitted optimized code. The adjacent
`mouseManager::SetPointer` at VA `0x00476940` has the same optimized register
allocation, establishing an optimized compiler profile for the whole TU.
