# Incremental compilation (/Gi) and the retail objects (HoMM1 VC4, measured)

Measured with the pinned VC4.0 `CL`/`C1XX`/`C2` on small test TUs and on the
current units. Retail is the reference. These are research observations, not
build profiles: no unit uses `/Gi` yet.

## What /Gi changes

`/Gi` requires `/Zi`; `/Z7` and `/Gi` together give D2016. CL passes `-Gi` to
both C1XX and C2.

- **`__LINE__` is relative.** Each function that uses `__LINE__` gets a static
  `short ?__LINE__Var@?1??<function>@4FA` holding its first source line. Each
  use emits `movsx eax, word ptr [__LINE__Var]` and then `add eax, <offset>`.
  Retail's "assert line words" are these variables. Examples are TOWNMGR's
  word at 0x0048ed8c (value 1483, read by `BuyBuild` with offsets 42, 43, 57
  and 58), PATH, EVENTS, RESMGR, MOUSEMGR, soundmgr, INPUTMGR, EXEC and the
  wingraph/netwin `*LineBase` words. Every use of `__FILE__` emits its own
  literal, which matches retail's repeated `D:\Heroes\...` strings (TOWNMGR
  has four after its line word).
- **One `.text` section per function, plus one `.data` section per function
  for its literals.** Without `/Gy` these sections are not COMDATs. The
  literal sections are 4-aligned and unpooled, and they follow their
  function in the object's section order. File-scope variables, together
  with the `__LINE__Var` words, stay in the first `.data` section. With
  `/Gy` the sections become COMDATs (literals as associative `$SG`
  COMDATs). With `/Gf` (implied by `/O2`) literals become named, pooled
  `??_C@` COMDATs.
- **The dynamic-initializer wrapper comes first.** For a global object with
  a constructor, `_$E2` (the `.CRT$XCU` wrapper, 0x15 bytes at `/Od`) is the
  object's first function. `_$E1` (the constructor call, 0x1a bytes) is
  emitted at the definition. Without `/Gi`, both are emitted together at the
  definition. Retail's Misc+PHILAI object has exactly the `/Gi` layout:
  `_$E2` at 0x00419990, the start of the object, and `_$E1` at 0x0041f2a9,
  between `RVOfPosition` and `StrategicValueOfPosition`.
- **C1 handle state shifts.** Code-identical functions change operand order.
  A survey compiled each unit with its profile, with `/Z7` replaced by
  `/Zi /Gi`, and measured each function's code distance to retail (relocated
  operands relaxed). The exact count rises from 838 to 915 over 1,018
  functions. Large gains: GAME (+19/-3), ADVMGR (+10/-2), PHILAI (+9/-3), AI
  (+8), COMMAND, SPELLS and soundmgr (+5 each). TOWNMGR's three CUR dips
  `SetArmyCommand`, `SetupCastle` and `RecruitHero` reach distance 0; its
  only loss is `BuyBuild`, whose source spells the line word out instead of
  using `__LINE__`. Losses, such as ARMY (-7) and CMBTMGR (-2), are
  functions tuned under the non-`/Gi` handle state.

## Code alignment under /Gi

C2 sets each code section's alignment where it creates the section
(0x0041acef). If the function's favour-speed flag ([0x47ea04], IL option
bit 0x800000, decoded at 0x00422948) is set, the alignment is [0x47ea34]
(16 for `-G4` and up, otherwise 4). If the flag is clear, the alignment is
1 (2 with [0x47e96c]). The same flag drives speed-versus-size code
selection. Clearing only that bit in a patched C2 reproduces the `-Os`
code: 24 of TOWNMGR's 32 functions change. So in the pinned C2, `/Gi` with
`-Ot` code gives 16-aligned functions, and packed functions require `-Os`
code.

- **BASE** fits `/Gi` with `-Ot`. Every function is 16-aligned with LINK's
  `CC` fill, and literals are 4-aligned and unpooled.
  - The `/O2` units need `/Gy`. Without it, C2 pads each function with
    `lea` no-ops, and retail has `CC` there.
  - The duplicate `stop CD` literals in soundmgr rule out `/Gf`. The
    candidate spelling is `/Ox /Gy /Gi /Zi`.
  - The `/Od` BASE units need no `/Gy`, because `/Gi` alone gives the
    per-function 16-byte alignment.
- **SOURCE** has the `/Gi` data, line-word and `_$E2` signatures and the
  operand-order gains, but its functions are packed, with odd starts. No
  flag set of the pinned C2 gives packed `/Gi` functions with `-Ot` code
  (tried: `-G3`/`-G4`/`-G5`, `/Os`, `/O1`, C2 `-Os`, both C1 orders of
  `-Os -Ot`, and no favour flag). Either retail's C2 build differed in this
  rule, or something else packed the sections. This is open.

## Not established

- Which retail units, if any, were compiled without `/Gi`.
- Why retail keeps literals that no code references (WINMGR's first
  `CCYCLE%02d.BIN` and `wb`, WINDOW's `Default Construct`). Under `/Gi /Gy`,
  a dead function's literals are discarded with it. Literals that only dead
  code inside a live function references are kept.
- Adopting `/Gi` in `config/units.toml` would change every unit's handle
  state, and in the pinned C2 it 16-aligns the SOURCE functions in the
  linked image.
