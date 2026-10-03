# TEXTWDGT equivalence survey

Manual survey of the non-exact functions of `src/BASE/TEXTWDGT.cpp` against retail
`HEROES.EXE` (master d1fdb30, strict comparison). Every differing
instruction pair was classified; "ours" is the recompiled object, "retail"
the delinked target. Offsets are function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `textWidget::Main` | 99.207 | IDENTICAL | mirrored compare | 0x83: `cmp ax,bp; jl` vs `cmp bp,ax; jg`; 0x88: `cmp si,[ebx+0x1a]; jl` vs `cmp [ebx+0x1a],si; jg`. |

1 functions: all IDENTICAL.
