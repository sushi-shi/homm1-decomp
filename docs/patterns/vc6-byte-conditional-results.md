# VC6 byte-valued conditional results

Measured in the complete HoMM1 Buka `SOURCE/COMMAND` translation unit with
pinned VC6 SP5 `/Od /Ob1 /GX /MT /G5`.

The type of a conditional's branches matters even when its eventual destination
is a signed byte. In `GetCommand`, the enum-valued move/fly conditional emits
`AND; NEG; SBB; NEG; ADD`. Giving both branches their actual byte representation
emits retail's `AND; TEST; SETNE AL; INC EAX`:

```cpp
return flying
    ? static_cast<i8>(COMBAT_MESSAGE_COMMAND_FLY)
    : static_cast<i8>(COMBAT_MESSAGE_COMMAND_MOVE);
```

The HoMM2 Buka donor uses the same branch casts for this operation. The casts
preserve the byte-valued command result while retaining the named domain values;
they do not change the monster flags or the underlying field types.

`CheckWin` provides a second control. Its retreat outcome is stored in a byte.
An int-valued `condition ? 0 : 1` emits `NEG; SBB; INC`; logical-not and an
explicit comparison with zero produced that same form in this TU. A conditional
with both branches cast to `i8` emits retail's `TEST; SETE` and byte store.
This also restores the retail branch structure, replacing the inherited
separate stores of the two outcomes.

Full-body checks cover the surrounding instructions, branch destinations and
reference identities. Stack-local placement remains non-exact; no comparison
normalization was added. See
[`buka-combat-actions.json`](../../config/retail/buka-combat-actions.json).

The Buka `SOURCE/ARMY` callers provide signed-byte argument controls as well.
`SetGridMode(i8)` receives a conditional with both arms cast to `i8`; this emits
the retail byte Boolean sequence in `DoAttack` and `SpecialAttack`. Casting the
completed integer comparison to `i8` was byte-flat and did not recover it.
The two retaliation calls to `GetAdjacentCellIndex` likewise require each
direction arm to retain its signed-byte type. Full-body evidence and remaining
stack-placement differences are in
[`buka-combat-movement-controls.json`](../../config/retail/buka-combat-movement-controls.json).

`combatManager::LoadIcons` is an exact array-index control: the castle-side
conditional has signed-byte arms (`COMBAT_ATTACKER_SIDE` and
`COMBAT_DEFENDER_SIDE`). It emits retail's `CMP; SETE CL; MOVSX EDX,CL`.
Casting the completed integer comparison instead emits the arithmetic Boolean
sequence. The complete body and references are exact; see
[`buka-seed-attention-controls.json`](../../config/retail/buka-seed-attention-controls.json).
