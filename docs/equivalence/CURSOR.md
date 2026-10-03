# CURSOR equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/CURSOR.cpp` against retail
`HEROES.EXE` (master d1fdb30, strict comparison). Every differing
instruction pair was classified; "ours" is the recompiled object, "retail"
the delinked target. Offsets are function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `advManager::StartCursor` | 90.527 | IDENTICAL | commutative operand order; register renaming | 0x110 and 0x139: `[ebp-0xc]*10 + [ebp-4]*720` computed as `eax=[ebp-0xc]*5; ecx=[ebp-4]*720; lea eax,[ecx+eax*2]` vs `eax=[ebp-4]*720; ecx=[ebp-0xc]*5; lea eax,[eax+ecx*2]`; same slots, same sum. |

1 functions: all IDENTICAL.
