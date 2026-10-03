# miscwin equivalence survey

Manual survey of the non-exact functions of `src/BASE/miscwin.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?ClipIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHHHHH@Z` | 94.180 | IDENTICAL | register renaming; mirrored compare; independent reorder of global loads | 0x130-0x29d: every clip case (inside, no clip, right clip, left clip, both) performs the same signed/unsigned tests (`Y>B`, `X+run <u left`, `X>R`, `X<left`, `R <u X+run`, `byte+X>R`) and the same copies (dest row+X, counts run / R-X+1 / run-left+X / width); only registers and load order of globals (no intervening stores) differ. |
| `?ClippedMonoIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHHHHHH@Z` | 95.543 | IDENTICAL | register renaming; independent reorder | Prologue computes the same `a+b-1` bounds from swapped registers; 0x12b: left-clipped count `esi-left+run` built in eax vs ecx (same three dead-store sequence to [esp+0x10]); fill dword built from cx only. |
