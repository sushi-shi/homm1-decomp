# GAME equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/GAME.cpp` against retail
`HEROES.EXE` (master d1fdb30, strict comparison). Every differing
instruction pair was classified; "ours" is the recompiled object, "retail"
the delinked target. Offsets are function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `advManager::FindAdjacentMonster` | 92.283 | IDENTICAL | mirrored compare; commutative operand order; branch displacement | 0x83/0x21d: `mov eax,[s_adjacentMonsterY]; cmp [s_adjacentMonsterEndY],eax; jle` vs `mov eax,[EndY]; cmp [Y],eax; jge` (both skip when EndY <= Y). 0x9d-0xb9: cell address `base+X*720` then `[eax+Y*5*2+8]` vs `base+Y*10` then `[eax+X*720+8]` (same sum). 0x125/0x151/0x2bf/0x2eb: `cmp [ebp+0x1c],Y; je` vs `cmp Y,[ebp+0x1c]; je` (equality). The other hunks are branch targets shifted by the 6-byte size difference. |
| `game::InitCampaignMap` | 97.416 | IDENTICAL | commutative operand order; register renaming | 0x174-0x185: `eax=x*21, ecx=[ebp-0xc]` vs `eax=[ebp-0xc], ecx=x*21`, consumed only by `mov al,[eax+ecx+gCampaignScenarios+0x13]`; eax upper bits are dead (reloaded at 0x1a3). |
| `game::LoadGame` | 99.806 | IDENTICAL | commutative operand order (equality compare); branch displacement | 0x3e9: `mov eax,[ebp-0x200]; cmp [giThisGamePos],eax; jne` vs `mov eax,[giThisGamePos]; cmp [ebp-0x200],eax; jne`. All other hunks are branch/call displacements from the 1-byte encoding difference. |
| `game::NextPlayer` | 99.964 | IDENTICAL | commutative operand order (equality compare) | 0x30c: `mov eax,[giHostGamePos]; cmp [giThisGamePos],eax` vs operands swapped; followed by `je` in both. |
| `game::Overview` | 99.972 | IDENTICAL | commutative operand order | 10 sites (0x2b4, 0x2dc, 0x325, 0x493, 0x4df, 0x64d, 0x699, 0x7fa, 0x838, 0x884): `movsx eax,[ebp-0x74]; movsx ecx,[ebp-0x90]; imul eax,ecx` vs the two loads swapped (the product commutes). |
| `game::SetRandomHeroArmies` | 95.104 | IDENTICAL | independent reorder across call; mirrored compare; register renaming | Threshold `strongArmy==0 ? 50 : 80` (`cmp [ebp+0xc],1; mov r,0; adc r,-1; and r,0x1e; add r,0x32`) is computed after `Random(0,99)` into ecx in ours, before it into callee-saved ebx in retail. It reads only the by-value parameter `strongArmy`, whose address is never taken, so `Random` cannot change it. Then `cmp eax,ecx; jge` vs `cmp ebx,eax; jle` (same condition). |
| `game::SettleOverlay` | 99.974 | IDENTICAL | commutative operand order (equality of pure calls) | 0x92/0xa5 and 0x12d/0x140: ours calls `GetObjectFamily(cell->trigger)` then `GetObjectFamily(cellEast->trigger)`; retail calls them the other way round. `GetObjectFamily` is a pure switch on its argument (no memory access, no side effect), and the results are compared with `cmp ebx,eax; jne`. |
| `game::TransmitSaveGame` | 99.852 | IDENTICAL | mirrored compare | 0x4fb: `mov eax,[ebp-0x238]; cmp [ebp-0x218],eax; jle` vs `mov eax,[ebp-0x218]; cmp [ebp-0x238],eax; jge`. |

8 functions: all IDENTICAL.
