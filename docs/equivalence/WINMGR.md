# WINMGR equivalence survey

Manual survey of the non-exact functions of `src/BASE/WINMGR.cpp` against retail
`HEROES.EXE` (master d1fdb30, strict comparison). Every differing
instruction pair was classified; "ours" is the recompiled object, "retail"
the delinked target. Offsets are function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `heroWindowManager::FizzleForward` | 95.008 | IDENTICAL | register renaming; independent reorder; alignment padding | 0x57-0x9c: the two short parameters at [esp+0x58]/[esp+0x54] live in si/di vs di/si; the bitmap constructor arguments are pushed in the same order. 0x17b-0x1ea (blend loop): srcA=`A->data + A->width*(row-[esp+0x14])` in esi vs edi, srcB=`B->data+[esp+0x30]` in ecx vs esi, dst=`screen->data+[esp+0x18]+[esp+0x2c]` in eax vs ecx, count `[esp+0x38]-[esp+0x18]` in edi vs eax, `this` reloaded into edx vs ecx; the loop body is the same instruction sequence modulo registers. Ours has an extra `nop` at 0x199; the 3-byte size difference shifts later displacements. 0x2e7-0x304: `this` in edx vs ecx. |

1 functions: all IDENTICAL.
