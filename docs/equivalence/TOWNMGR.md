# TOWNMGR equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/TOWNMGR.cpp` against retail
`HEROES.EXE` (master d1fdb30, strict comparison). Every differing
instruction pair was classified; "ours" is the recompiled object, "retail"
the delinked target. Offsets are function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `townManager::BuyBuild` | 99.48 | DIFFERENT, fixed (now IDENTICAL) | assert line constant; commutative operand order | 0x22b/0x35c: `add eax,0x2d` / `0x3c` vs retail `0x2a` / `0x39`: the neutral-building and dwelling-cost `ProcessAssert` calls reported lines 1528/1543 instead of 1525/1540, because clang-format split each call over four lines after its `#line` and `__LINE__` landed on the fourth. Fixed by keeping each call on its `#line` line (clang-format off/on); both constants now match. Remaining 0x9f5: `16*[ebp-0x5c] - [ebp-0x6c] + 45*[ebp-0x6c]` vs `44*[ebp-0x6c] + 16*[ebp-0x5c]` (same sum). CUR stays 99.48. |
| `townManager::GetCategoryStats` | 99.99 | IDENTICAL | commutative operand order | 0x25a/0x275/0x290: sum of `gpGame` town fields +0x29d, +0x28d, +0x295 vs +0x28d, +0x295, +0x29d (no stores between the loads). |

2 functions: 1 IDENTICAL, 1 DIFFERENT (fixed in source).
