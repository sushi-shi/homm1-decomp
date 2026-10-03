# ARMY equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/ARMY.cpp` against retail
`HEROES.EXE` (master d1fdb30, strict comparison). Every differing
instruction pair was classified; "ours" is the recompiled object, "retail"
the delinked target. Offsets are function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `army::DoAttack` | 98.889 | IDENTICAL | commutative operand order; equivalent multiply selection; register renaming | 0x356-0x373: `20*b[+0x48] + 4*b[+0x49]` built as `ecx=b48*20; eax=ecx+b49*4` vs `eax=b48*20; eax=eax+b49*4`. 0x501-0x52a and 0x6f9-0x722: `504*b48 + 84*b49`: ours `b48*63` (`shl 6; sub`), `b49*84` (`*5`, `*21`, `shl 2`), `lea [ecx+eax*8]`; retail `b49*21`, `b48*504` (`shl 6; sub; shl 3`), `lea [ecx+eax*4]`. Same hex indices and field addends (+0x48, +0x49). |
| `army::DrawToBuffer` | 98.852 | IDENTICAL | commutative operand order (equality compare); branch/jump-table displacement | 0x284: `eax=army->+7, ecx=gpCombatManager->+0x6bf` vs swapped loads, followed by `cmp eax,ecx; jne` (equality). The 1-byte shorter retail encoding shifts every later branch and both switch tables (entry targets and index-table bases move by exactly 1; the std/cld/pop noise in the listing is the tables decoded as code). |
| `army::SpecialAttack` | 99.987 | IDENTICAL | commutative operand order | 0x26d: `eax=[ebp-0x70], ecx=[ebp-0x78]; cmp eax,ecx; jg +; mov eax,ecx` vs operands swapped: both store the maximum of the two to [ebp-0x64] (equal values give the same result). 0x439/0x479/0x4aa/0x4b6: `mov eax,a; add eax,b` vs `mov eax,b; add eax,a`. |

3 functions: all IDENTICAL.
