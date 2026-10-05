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


## Army drawing control

The final-source `SOURCE/ARMY` control confirms the same mechanism in another
unit. The first five setup/resource functions are unchanged between profiles.
Drawing adds 74 clears under `/G6`, standing and wincing add one each, and walking
adds nine. Every retained instruction, reference, branch and switch-table target
is verified. `/G5` agrees with retail's absence of these clears.

`Walk` also exposes a CPU-dependent equivalent instruction choice:

```asm
; /G5 and retail
and al, 0xfe
add eax, 1
; /G6
and eax, 0xfffffffe
add eax, 1
```

Both AND forms clear only EAX bit zero. They set different width-dependent
flags, but the immediately following ADD overwrites all flags before use.
Two such sites account for two additional bytes under `/G6`. This is an
explicit control result, not a normalization accepted by strict comparison.
See [`buka-army-drawing.json`](../../config/retail/buka-army-drawing.json) for
all nine function controls and hashes. `Wince`'s separate boolean-materialization
residue remains visible after the profile correction.


## Melee retaliation direction controls

The Buka melee routine passes both wide-creature retaliation direction choices
through a signed byte before extending to its word parameter. The choices
remain 0/5 and 2/3. Restoring the missing conversion in the first expression
recovers the byte-to-word extension; the retail callee reads words for both
arguments. See [the complete melee review](../../config/retail/buka-melee-attack.json).

Do not infer full-register equality from a byte argument. Retail's `SETcc`
leaves upper bits untouched, whereas the candidate's `NEG/SBB` idiom can define
them. The per-bit dataflow review checks only the argument bits the retail
callee actually consumes and rejects an artificial 32-bit contract here.
Explicit signed-byte conversion and logical negation at the `SetGridMode`
call both retain the non-retail materialization; neither control was kept.
These observations do not authorize a new CPU profile or comparison mask.


## Army-group control

`SOURCE/ARMYGRP` independently confirms the narrow-argument mechanism.
`CanJoin` is 59 bytes under `/G5` and 61 under `/G6`; `/G6` adds only
`xor eax, eax` immediately before `mov al, byte ptr [ebp + 8]`. Retail has
the `/G5` sequence, and `IsMember` consumes the signed byte at its argument
slot. All ten other explicit routines have identical profile-control bytes
outside their checked references, and every branch destination agrees.
The complete eleven-function control and retail review are in
[`buka-army-group.json`](../../config/retail/buka-army-group.json).


## Combat-manager control

An unchanged-source `/G5` versus `/G6` control of `SOURCE/CMBTMGR` gives the
same narrow-argument result. `CombineGroups` is 280 bytes under `/G5`, matching
retail, versus 296 under `/G6`. `LoadArmies` is 607 bytes under `/G5`, matching
retail, versus 615 under `/G6`. The check covers every currently identified
body in the unit and loses none of the previously matching instruction bodies.
References remain subject to strict identity/addend comparison.

Separately, `SetupCombat`'s three conditional-value assignments account for
three four-byte compiler temporaries. The retail frame is 20 bytes; expanded
branch assignments produce an eight-byte frame and different stores.
Restoring the hero/group choices, the original-town assignment and the
castle-flag value as conditional-value expressions reproduces the complete
retail body without introducing authored temporaries.
`Open`'s extra seven-byte null assignment immediately before `LoadPlaySample`
is also absent from retail. These are source-form corrections, not comparison
normalizations. See [the full controls](../../config/retail/buka-combat-setup.json).


## Adventure-event controls

The complete Buka `DoEvent` operand review establishes `/G5` for EVENTS:
retail omits the narrow argument clears and uses byte flag operations. Four
final-source compiler controls isolate the same clear difference without
source changes: `GiveArtifact` and `GiveRandomArtifact` each omit one clear,
while `HouseEvent` and `HeroLoses` each omit two. Every retained instruction,
reference/addend and branch target agrees between these controls. Their `/G5`
bodies also match retail under strict comparison. The full dispatcher retains
local-frame placement differences; other EVENTS behavior remains under review.
See [`buka-events.json`](../../config/retail/buka-events.json).
