# VC6 CPU profile and narrow arguments

The pinned VC6 SP5 `/Od /Ob1 /GX /MT` compiler emits different argument setup
under `/G5` and `/G6`. A final-source A/B control of HoMM1 `SOURCE/AI` isolates
the effect. `/G6` emits self-XOR register clears before loading byte or word
arguments; `/G5` omits them. For example:

```asm
; /G6
xor edx, edx
mov dl, byte ptr [ecx + 0x6b0]
push edx
; /G5 and Buka retail
mov dl, byte ptr [ecx + 0x6b0]
push edx
```

The callee's recovered parameter type consumes the narrow value; this is not
evidence that the argument should be widened in source. The complete AI
instruction review checks the call identities, declarations and operand widths.

| Function | `/G5` bytes | `/G6` bytes | Extra clears |
| --- | --- | --- | --- |
| `DoCompAI` | 2162 | 2268 | 49 |
| `GetClosestArmy` | 258 | 266 | 4 |
| `AttemptAttack` | 387 | 399 | 6 |
| `AttemptAdjacentAttack` | 636 | 646 | 5 |
| `WalkTowardArmyFront` | 490 | 506 | 8 |
| `WalkTowardArmy` | 502 | 512 | 5 |

The other seven functions have identical control-object instruction bytes and
references. The six affected functions add 77 clears in total; `DoCompAI` also
widens two branches, accounting for eight more bytes. The A/B proof checks
every retained instruction, ordered reference/addend and destination, including
branches that enter an added clear before the corresponding narrow load.

A separate complete `/G5` versus Buka review checks all thirteen functions.
The retail sequence excludes those clears. Stack-local placement remains
unmatched in twelve functions and is not hidden by this compiler correction.
Use the existing `cpp_buka_g5` unit profile; do not generalize this observation
to unreviewed units. Full controls, hashes and retail comparisons are in
[`buka-combat-ai.json`](../../config/retail/buka-combat-ai.json).
