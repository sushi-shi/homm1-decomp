# FLY equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/FLY.cpp` against retail
`HEROES.EXE` (master d1fdb30, strict comparison). Every differing
instruction pair was classified; "ours" is the recompiled object, "retail"
the delinked target. Offsets are function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `army::FlyTo` | 99.234 | IDENTICAL | mirrored compare; branch displacement | 0x535/0x54c: `mov eax,[ebp-0x6c]; cmp [giMinExtentX],eax; jge skip` vs `mov eax,[giMinExtentX]; cmp [ebp-0x6c],eax; jle skip` (both assign when the local is greater than the global); the same for giMinExtentY/[ebp-0x78]. The 1-byte encoding difference at each site shifts the later branch/call displacements (all other hunks). |

1 functions: all IDENTICAL.
