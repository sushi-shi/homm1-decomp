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
