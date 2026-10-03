# BUTTON equivalence survey

Manual survey of the non-exact functions of `src/BASE/BUTTON.cpp` against retail
`HEROES.EXE` (master d1fdb30, strict comparison). Every differing
instruction pair was classified; "ours" is the recompiled object, "retail"
the delinked target. Offsets are function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `button::Main` | 98.57 | IDENTICAL | mirrored compare; register renaming; equivalent instruction selection; alignment padding | 0x1fa/0x26d/0x308: `cmp cx,dx; jl` vs `cmp dx,cx; jg`. 0x2e8-0x329: hit test with ax/cx and ebx/ecx swapped, same comparisons (`jl`, `jle`). 0x33d-0x366: flag clear `and cl,0xfe` vs `and al,0xfe` (1 byte shorter), vtable in eax vs ebx, `UpdateScreenRegion` arguments pushed in the same order ([esi+0x1e], [esi+0x1c], ...) from swapped registers; the high halves of those i16 arguments are unspecified in both. The switch tables move from +0x4f4/+0x50c to +0x4f0/+0x508 (1-byte shift plus 4-byte alignment). |

1 functions: all IDENTICAL.
