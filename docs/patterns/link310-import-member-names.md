# LINK 3.10 names import members after the DLL name it records

Measured with the pinned VC4.1 `LINK.EXE` (3.10) and `LIB.EXE` under wine,
building import libraries for HoMM1 Buka's `mss32.dll` and `smackw32.DLL`.

## Why it matters

The consuming linker groups each DLL's `.idata$4`/`.idata$5` contributions
by archive member name, so a long-format import library's member names reach
the image's ILT/IAT layout. Retail's three LINK 3.10 libraries (mss32,
smackw32, and VC4.1's own `netapi32.lib`, whose members are `NETAPI32.dll/`)
are named exactly after the DLL as the image's import directory spells it.

## Observations

Every import member and the descriptor member carry one name, and the import
descriptor's DLL string is that same name. It is chosen as follows:

| invocation | member name |
| --- | --- |
| `LINK /DLL /IMPLIB`, no `.def`, `/OUT:mss32.dll` | `mss32.dll` |
| same, `/OUT:smackw32.DLL`, `/OUT:MSS32.DLL`, `/OUT:foo.dll` | the `/OUT` file name verbatim |
| `.def` `LIBRARY smackw32`, `/OUT:smackw32.DLL` | `smackw32.DLL` |
| `.def` `LIBRARY smackw32`, `/OUT:foo.dll` / `/OUT:foo.DLL` | `smackw32.dll` / `smackw32.DLL` |
| `.def` `LIBRARY SMACKW32`, `/OUT:smackw32.DLL` | `SMACKW32.DLL` |
| `.def` `LIBRARY smackw32.DLL` or `"mss32.dll"`, any `/OUT` | the `LIBRARY` name verbatim |
| `LIB /DEF` with `LIBRARY smackw32` | `smackw32.dll` |
| `LIB /DEF` with `LIBRARY smackw32.DLL`, or `/NAME:smackw32.DLL` | that name verbatim |

So the name is the `LIBRARY` name when it has an extension; a bare `LIBRARY`
stem takes the `/OUT` file's extension (`LIB /DEF` appends `.dll`); without
`LIBRARY` it is the `/OUT` file name. `homm1.graph.implib` links each stub
DLL as `/OUT:<retail DLL name>` (its `.def`, when one is needed, says
`LIBRARY <stem>`), so LINK writes retail's names itself; `_verify_members`
fails the synthesis on any other name, and nothing edits the library.

## Not established

Because one string serves as both the member name and the imported DLL
name, LINK 3.10 cannot emit a member name that differs from the DLL string
the image imports; a target that needs such a difference needs a library
LINK 3.10 did not write. The VC 2.0 LINK 2.50 (`vc2` format) was not
measured here.
