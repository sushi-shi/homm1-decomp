# `LogStr`

HoMM1 retail VA `0x00419A1E` is the `BASE/Misc` logging operation found in
both HoMM2 donors. Its 166-byte body uses a 500-byte stack buffer, opens
`KB.LOG` with mode `at+`, appends a newline, writes and closes the stream, and
optionally calls `OutputDebugStringA`.

Unlike the HoMM2 donor, HoMM1 does not test the `fopen` result before using
the stream. Removing that donor-side guard accounts for the complete 18-byte
body-size difference.

The literal addresses at VAs `0x0048F590`, `0x0048F594`, and `0x0048F59C`
also prove the VC4 `.data` layout: `at+`, `KB.LOG` with its alignment byte, and
the newline occupy 4, 8, and 2 bytes. Calls resolve to the same VC4 CRT members
used by the donors: `_fopen`, `_strcpy`, `_strcat`, `_fputs`, and `_fclose`.

Retail directly proves the two logging levels through comparisons with
`giDebugLevel` at VA `0x004C7C94`: values below `2` return before file output,
and value `3` also emits the completed line to the debugger. The latter is the
only behavioral constant that differs from the current HoMM2 source.
