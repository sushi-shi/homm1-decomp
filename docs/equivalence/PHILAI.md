# PHILAI equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/PHILAI.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?CheckBerserk@philAI@@QAEXPAVhero@@@Z` | 99.654 | IDENTICAL | mirrored compare | 0x14c: `cmp [ebp-0x18],[ebp-4]; jge` vs `cmp [ebp-4],[ebp-0x18]; jle` (assign when [ebp-4] > [ebp-0x18]). |
| `?DetermineTargetPosition@philAI@@QAEXPAVhero@@AAC1F@Z` | 99.891 | IDENTICAL | commutative operand order; mirrored compare | 0x6ba-0x71c: the two Manhattan distances `abs(dx)+abs(dy)` are computed in the opposite order (ebx/esi swap, abs pure) and compared with `jle` vs `jge`. |
| `?DoAI@philAI@@QAEXH@Z` | 99.023 | DIFFERENT, fixed | wrong global referent; equality/mirrored compares | 0x1d6: ours tested `giCurWatchPlayerBit`, retail `gCurWatchPlayerHighBit` (hero-visibility bit, as in CURSOR/ADVMGR hero checks). Source fixed. Remaining: `cmp` operand swaps before `jne` (0x36e, 0x386, 0x727, 0x73f) and `jle`/`jge` mirror at 0x5aa. |
| `?GetTurnAIVars@philAI@@QAEXH@Z` | 99.990 | IDENTICAL | commutative operand order | 0x4ae/0x4bf: `abs(a)+abs(b)` with the two pure `abs` calls in the opposite order. |
| `?HeroInteractionAtTown@philAI@@QAEXPAVhero@@PAVtown@@HPAH@Z` | 99.387 | DIFFERENT, fixed (now IDENTICAL) | float reassociation; equality compare | 0x4cc: ours computed `((c*c-1)*weight)*n`, retail `((c*c-1)*n)*weight` (different x87 rounding before `__ftol`). VC4 reassociates the float product chain; parenthesising `((curveTerm*curveTerm-1)*(heroStrength+garrisonFV))` restores retail order (CUR 99.39 -> 99.99). 0x761: equality compare operand swap. |
| `?TurnsToBuy@philAI@@QAEMQAH@Z` | 99.965 | IDENTICAL | commutative max (domain-restricted) | 0x9c: `__max(fTurns,maxT)` with the two x87 loads swapped; both pick the larger value; equal/NaN cases cannot differ because both operands come from `fild` of positive ints, 99.0f or 0.0f. |
| `?ValueOfEventAtPosition@philAI@@QAEHPAVhero@@FFHPAH@Z` | 99.760 | DIFFERENT, fixed (now IDENTICAL) | float reassociation; commutative operand order | 0x1143 (DAEMON_CAVE): ours summed `((h*100+fd)+h*300)+h*300+gold*2500+gold*-750`; retail `((h*300+gold*2500)+(h*100+fd))+h*300+gold*-750`. Parenthesising the first pair restores retail association; the residue is a single commuted `+` of two values (exact in IEEE). CUR 99.76 -> 99.71. |
