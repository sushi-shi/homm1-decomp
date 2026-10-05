# VC6 helper forms: which spellings stay byte-neutral

Measured on the Buka VC6 SP5 profiles (`/Od /Ob1 /GX /MT /G5` for the combat
and AI units, `/O2` for FINDPATH and SEARCH) while recovering the combat
helpers listed in [the combat common-code ledger](../common-code-combat.tsv).
"Identical" means every non-debug section and its ordered relocations match
the previous object, with compiler counter names (`$L`, `$T`, `$SG`, the
`$name$NNN` local statics) normalized.

## Expression macros are neutral

Comma and `&&`/`||` expression macros expand to the text they replace, so the
objects are identical: `SET_NEXT_COMBAT_MOVE`, `HEX_HAS_OCCUPANT`,
`CLEAR_HEX_OCCUPANT`, `UPDATE_INCLUSIVE_REGION`, `CELL_HAS_NON_SHADOW_OBJECT`
(also under `/O2`), and the negated `!ARMY_IGNORES_SPELLS(a) && ...`, which
gives the same branch chain as `a->type != X && a->effect != Y && ...`.

A macro still fixes one evaluation order. Sites that compare index before side,
or effect before creature type, are different source and stay explicit.

## Statement macros are not

`do { ... } while (0)` costs code under `/Od`: wrapping army::Walk's four-if
extent clamp grows `.text` from 20850 to 20859 bytes (Walk 95.62%). HoMM2 Buka
measured the same cost on the same compiler. Multi-statement operations (the
extent clamps, the spell-icon cache reload) therefore stay written out.

## Inline accessors: value versus reference

`hero::IsEmbarked()` returns `m_eventFlags & HERO_EVENT_EMBARKED` as `i32`;
replacing the ten open-coded PHILAI tests leaves the bytes identical (only the
`$L`/`$T` counters move).

Reference-returning accessors change the function:

| Accessor | Function | Score |
| --- | --- | ---: |
| `game::GetPlayerHero(player, i)` | `philAI::DetermineHeroToMove` | 93.39% |
| `searchNode& searchArray::GetNode(x, y)` | `philAI::CheckReload` | 90.81% |

This agrees with HoMM2's inline-accessor return-temporary observation: the
expanded call is materialized before use. Retail read those members directly.

## Header inlines move counters, not bytes

Adding an inline function to a header every unit includes
(`OppositeMapDirection` in `cursorTypes.h`) renumbers `$SG`, `$L` and local
static names in each including unit. The code and data bytes are unchanged and
`homm1 compare --baseline` is unaffected.

## Conditional spellings are distinct

`m_facing ? -1 : 1` tests the byte; `m_facing == ARMY_FACING_LEFT ? -1 : 1`
compares it with 1. Retail uses both forms: the first in army::Walk, PowEffect
and MoveAttack, the second in FLY, SpecialAttack and KeepAttack. Rewriting one
Walk site in the compare form drops Walk to 99.79%. A single rear-hex-offset
macro cannot reproduce both forms, so these sites stay explicit.

## Adventure and BASE measurements

Measured while recovering the [adventure common-code ledger](../common-code-adventure.tsv):

- `hero::IsEmbarked()` returning `i32` is byte-identical at all 22 adventure
  sites. Declaring it `i8` drops ten functions (1047/1057): the byte return is
  stored to a frame temporary before the test, as Buka 2.1 measured for
  playerData's accessors (inline-accessor-return-width). The int-valued
  accessor is the evidenced form.
- `game::GetPlayerHero`/`GetPlayerTown` are byte-identical where the pointer is
  stored straight into a local (townManager::GetCategoryStats,
  CheckEndGame) but change philAI::DetermineHeroToMove (above). The accessor is
  a per-site form, not a header-wide rewrite.
- An inline `FontGlyphIndex(i32)` for FONT's CP1251 glyph fold drops
  font::DrawString to 99.79%; retail expanded no inline there.
- Default arguments added to a declaration (`TransmitRemoteData`) or dropped
  at call sites (`NormalDialog`) never change code: the compiler pushes the
  same constants.

