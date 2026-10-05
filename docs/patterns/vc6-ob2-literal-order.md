# VC6 /Ob2 emits file-scope initializer literals in source order

Measured with the pinned VC6 SP5 (C2 12.00.8966) in HoMM1 Buka `SOURCE/REQUEST`.

A file-scope pointer initialized from a string literal (`char* gFRDummy = "";`)
gets its literal storage when the front end processes the initializer. Under
`/Od /Ob1` the literal is emitted before every function literal of the
translation unit. Under `/Od /Ob2` it is emitted at the definition's position
among the function literals, in source order. The instruction bytes do not
change. A scaled-down probe with the definition between `f2` and `f3`:

| Flags | `.bss` literal order |
| --- | --- |
| `/Od /Ob1` (also with `/Ob0`, `/Gy`, `/Z7`, `/Zi`, `/Za`) | `gDummy`, `f1`, `f2`, `f3` |
| `/Od /Ob2` | `f1`, `f2`, `gDummy`, `f3` |

The probe also checked these alternatives under `/Ob1`; none places the literal
in source order while keeping the datum first:

- `#pragma auto_inline`, `inline_depth` and `optimize` leave the order unchanged.
- A function-local static places both its datum and its literal in function
  order.
- A static in an inline function becomes a COMDAT datum with a pooled COMDAT
  `??_C@` literal.
- Named zero objects (`static char e[1]`) are placed among the variables.

Retail REQUEST keeps `gFRDummy` as the first `.data` item. Its `""` sits
between `UpdateMapInfo`'s and `ShowMapInfo`'s `.bss` literals. Compiling only
REQUEST with `/Od /Ob2 /G5` closes the constructor, `Open` and `UpdateMapInfo`
and keeps all 13 REQUEST bodies exact. Applying `/Ob2` to every `/Od` unit
regresses six KB bodies, whose retail literal order is data-first. The inline
expansion setting is therefore a per-unit fact; use it only where retail
literal placement proves it.
