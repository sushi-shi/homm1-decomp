# Wingraph globals required by code

These six four-byte identities are supported by the pinned February 1996
`HEROES.EXE` and the corresponding declarations in HoMM2 Buka 2.1
`include/SOURCE/wingraph.h`. The latter supplies names and types; retail
addresses and instruction use decide the HoMM1 identities.

| RVA | Name | Retail use |
| --- | --- | --- |
| `0x0008E180` | `giGraphicsType` | Compared with `1` by `RestoreDisplayMode` at `0x5451`, `SetPalette` at `0x547D`, `InitializePalette` at `0x55D3`, `CleanUpWinGraphics` at `0x5639`, and `QueryNewPalette` at `0x5723`. |
| `0x0008E5A8` | `gbWinGraphBusy` | Compared with zero at `0x36A2` in `DDQueryNewPalette`. |
| `0x0008E94C` | `hdcImage` | Tested, passed to `SelectObject` and `DeleteDC`, then cleared in `WGCleanUpWinGraphics` at `0x5344`. |
| `0x0008E950` | `gbmOldMonoBitmap` | Passed to `SelectObject` at `0x535A` in the WinG cleanup path. |
| `0x0008E954` | `hpalApp` | Tested and passed to `SelectPalette` at `0x4BF8`; deleted and cleared at `0x5344`. |
| `0x0008E9FC` | `hDDrawLibrary` | Compared unsigned with `32` and passed to `FreeLibrary` in `DisconnectDLLs` at `0x5428`. |

The `DATA` annotations and census rows record only code-required identities
and four-byte layout. They do not assert initializer or data-byte matching.
