# VC4 conditional with an enumerator arm

Measured with the HoMM1 VC4 `/Od /Z7 /G5 /Ob1 /GX` profile (EVENTS, 2026-10).

In the retail build every H1_ENUM form is a plain `enum name {`. In
`cond ? <char or short operand> : <enumerator>`, VC4 gives the conditional
the narrow operand's type. With an `int` literal arm it gives the conditional
`int`.

`advManager::DoCombat`, where `firstHero->m_owner` is a `signed char`:

```cpp
attackPlayer = firstHero ? firstHero->m_owner : GAME_PLAYER_NONE;  // enum arm
attackPlayer = firstHero ? firstHero->m_owner : -1;                // retail
```

- **Enumerator arm.** Both arms are written to a byte-wide temporary. The
  `-1` arm is stored as `mov dword [tmp], 0xffffffff`. The result is read
  back with `movsx eax, byte [tmp]` and then stored to `attackPlayer`. That
  is one extra stack slot (`sub esp, 0x64` instead of `0x60`) and an extra
  `movsx`/`mov` pair.
- **Literal arm.** Each arm stores straight into `attackPlayer`.
- **Exactness.** Restoring the literal made `DoCombat` exact (99.53 to 100)
  without any other change.
- **When it changes nothing.** When the conditional's value is itself stored
  into a byte field, the two spellings compiled to the same code
  (`SendHeroTownData`'s `buf->firstOwner`).

Replacing a literal with an enumerator is therefore not always behaviour
neutral. It changes the conditional's type, so it can change the instruction
set, not only the handle state.

## Detecting it

- After an enum-naming edit, the function's semdiff shows a changed frame size
  or an extra `movsx` from a stack temporary, not only a register or operand
  swap.
- No TU-state trial moves the function back to its earlier score. In
  `homm1 permute state` (`tu_state_noise`), every trial plateaus at the same
  best value.
- Grep for `? … : NAME` and `? NAME : …` where the other arm is a `char` or
  `short` field, local or parameter. If it is not exact, spell the arm as the
  integer literal and add a keep row.

## Not this mechanism

A function whose code does not change when its own enum spellings are
reverted, but which moves with a header-only change (for example new enum
blocks in a shared header), is responding to header state, not to a type
change; `soundManager::SetMusicQuality` is such a case.
