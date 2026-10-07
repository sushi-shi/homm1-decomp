# Heroes of Might and Magic TE — change catalogue

This catalogue records every difference between the community edition
"Heroes of Might and Magic TE" (HeroesWorld TE, version TE 1.04, build
2025-01-15, packages `H1TE250115EN.zip` and `H1TE250115RU.zip`) and the retail
Buka 2003 `HEROES.EXE` this tree reconstructs. It is the input for a future
`source-te` branch that expresses the edition as readable source changes on
top of the clean generated source. Nothing here changes game source.

The mod's binaries are never committed. Addresses are virtual addresses in
the retail image (base `0x400000`); function names come from
`config/retail/buka-function-map.tsv` and original semantics from the matched
source under `src/`. `changes.tsv` (next to this file) holds one row per
change with the columns `id, component, kind, va_or_key, function, category,
description, recommendation, risk, status, status_note`; `status` records how
this branch implements the row (implemented, differs, deferred,
skipped-out-of-scope, or build choice for a row that belongs to how the
program is built rather than to its source).

## 1. Summary

**How TE is built.** The edition is a binary-patched copy of the Buka 2003
executable (same link timestamp and section layout) plus a runtime plugin
layer. The executable's entry point loads `Loader_xx.dll`, which loads every
`Plugins\xx\*.dll`; each plugin installs further hooks through the
`patcher_x86` hooking library. A WinG replacement (`WING32.dll`, an OpenGL/GDI
renderer wrapper) owns `config.ini` and loads `mods\*.mod`. TE expects the
base installation's `smackw32.dll`, `audiere.dll`, maps, sounds and music
tracks, which it does not ship.

**Counts.**

| Layer | Rows in `changes.tsv` | Content |
|---|---|---|
| Executable `.text` | 33 (X01–X07, X10–X18, X20–X36) | 4,188 bytes in 45 functions plus the CRT entry; 8 code caves |
| Executable `.data`/`.rdata` | 6 (D01–D06) + parts of X05/X18/X20/X35 | 146 + 9 bytes |
| Executable `.rsrc` | 2 (R01–R02) | English menus/about box, 75 colour cursors |
| Runtime layer | 75 (RT-*, TE-*, PL-*) | loader, framework, TE.dll (~90 patch sites, 4 whole-function replacements), 6 enabled plugins, 3 disabled plugins, ini files, data |

By category over all 116 rows: balance 32, bug fix 26, UI 26, platform 14,
text 10, framework 7, no-op 1.

**Most significant gameplay changes.**
1. *Retreat/surrender and cowardice:* the AI surrenders (keeping its army)
   when it can afford it, refused AI surrenders turn into a damage spell, a
   re-hired retreated hero loses that day's movement (option), and
   `hero::m_cowardice` becomes a live morale penalty (−1 per non-surrender
   loss, recovering on wins). Killed or dismissed heroes return to the pool
   reset to level 1 with class starting stats (X24, TE-RS-*, TE-MOR-*).
2. *Deterministic random artifacts* from map-cell index and per-hero seeds:
   no reroll on reload, no 1000-gold fallback (X22).
3. *Combat rules:* ghosts no longer gain twice when retaliating (X10), bad
   luck as likely as good luck (X11), Fizbin of Misfortune is −3 luck
   instead of −2 morale (X20), Elves need ammunition for their second shot
   (TE-FIX-3), duplicate-type stacks keep correct survivor counts (X13).
4. *Adventure/economy:* tavern heroes reserved so no hero is offered twice
   (X30), obelisk puzzle completes for any obelisk count (X32), starting
   heroes begin at 0 experience (X25), random castles lose their free
   dwelling and garrison (TE-BAL-1), map-placed heroes get correct movement
   (X27).
5. *AI:* auto-resolve thresholds (X35) and copy-paste fixes in
   `QuickCombat` (TE-FIX-4..6), float army valuation (X36), boat purchase
   fix (TE-FIX-1), plus the optional `SlightlyHarderAI` block (TE-AI-*).
6. *New content and UI:* damage forecast in the combat status bar (X16),
   movement in hero quick view (X29), remaining hit points in combat creature
   view (X31), F5/F9 quicksave/quickload, "(Visited)" quick info, k/m number
   formatting, the scripted "Paladin stronghold" on Megalith objects,
   extended cheat codes (`CheatMode=2`, shipped default).
7. *Platform:* no CD check (X02), asserts compiled out (X03), idle-CPU
   message pump (X04, PL-CPU-1), windowed defaults and a separate registry
   key (X05), videos off by default (X06), lossless/OGG music from the game
   directory (TE-OPT-6).

**Static vs effective behaviour.** Several executable patches are overridden
by plugins in the shipped configuration: `fsfix.dll` restores the retail
full-screen preference read (X05), `ReturnBWCursors.dll` restores the retail
cursor code (X07), `CPUPatchAdv.dll` turns `Sleep(0)` into `Sleep(1)` (X04),
and `TE.dll` disables the executable's own "left the map today" mobility flag
in favour of its retreat/surrender state (X24) and replaces `GetMorale`
(X20). The entries below state both.

## Method

- Section-by-section byte comparison of retail and both TE executables;
  runs mapped to functions through `buka-function-map.tsv`.
- Capstone disassembly of retail and TE bytes for every patched function,
  following each jump into its cave and back; cave entry points cross-checked
  by scanning all `call`/`jmp`/`jcc` and absolute references in TE `.text`.
- Data globals resolved through the retail claim files in `config/retail`;
  strings decoded as CP1251.
- Resource directory walked for both images; menus and dialog decoded.
- DLL patch tables recovered from `patcher_x86` call shapes (vtable slots
  `WriteByte`, `WriteWord`, `WriteDword`, `WriteJmp`, `WriteHexPatch`,
  `WriteCodePatch`, `WriteLoHook`, `WriteHiHook`) and their address
  immediates.

## 2. Executable patches (`H1TE_xx.exe`)

### 2.1 Image-level facts

| Property | Retail Buka 2003 `HEROES.EXE` | TE `H1TE_EN.exe` / `H1TE_RU.exe` |
|---|---|---|
| SHA-256 | `34233110eff3c568…a654db` | EN `10142a44…e1269b9187e9`, RU `0ac3d3ae…0513c3f101` |
| Link timestamp | `0x3e96d447` | unchanged |
| Sections | `.text .rdata .data .rsrc` | same four, same RVAs and raw offsets; no section added |
| `.rsrc` virtual / raw size | `0x1870` / `0x2000` | `0x2c4dc` / `0x2d000` |
| SizeOfImage | `0xdc000` | `0x107000` |
| Import table | 10 DLLs (incl. `audiere.dll`, `smackw32.DLL`) | byte-identical; the loader DLL is **not** an import |
| Debug directory | CodeView `NB10` at file `0xa9000` | same record, `PointerToRawData` moved to `0xd4000` (the only `.rdata` change besides the double below) |

Byte totals (identical for EN and RU): `.text` 4,188 bytes in 142 raw runs
(65 runs after merging gaps under 8 bytes) touching 45 functions plus the CRT
entry; `.data` 146 bytes in 14 runs; `.rdata` 9 bytes in 2 runs; `.rsrc`
rebuilt. The EN and RU executables differ from each other in exactly three
places: the loader name inside the entry stub (`0x416c19`, `EN`/`RU`) and the
two data paths (`0x490e37`, `0x490f97`).

### 2.2 Patch mechanics and code caves

All new code lives inside `.text`, in bodies that the patch set first makes
unreachable or shortens. Every patched call site uses a plain `jmp`/`call`
into one of these caves and jumps back; no `.text` patch jumps into a DLL.
The only bridge to the runtime layer is the entry stub (X01), which
`LoadLibraryA`s `Loader_xx.dll`; the plugins then hook further functions at
run time (section 4).

| Cave host (retail function) | Why it is free | Cave contents (entered from) |
|---|---|---|
| `army::DoAttack` `0x416beb–0x416c27` | the duplicated ghost-retaliation block (X10) is removed | entry loader stub + `"Loader_xx.dll"` string (from CRT entry) |
| `EarlySetup` `0x43cff1–0x43d0c3` | CD check and its four error boxes removed (X02) | ViewArmy hit-point line (X31); hero quick-view format + `"%s: %d (%d)"`, `"\n%s%d (%d)"` |
| `IsCDDrive` `0x442254–0x44228c` | unreachable after X02 | hero "redeployed today" flag set (X24) and the `CalcMobility` test (X24) |
| `DriveSupportsFreeSpaceQuery` + `SetupCDDrive` `0x444702–0x444ae2` | unreachable after X02 | PerDay flag reset, RandomizeHeroPool tail, QuickCombat win clamp, Fizbin luck/morale code, Dimension Door bounds check, obelisk piece share, float morale/luck scaler, experience clamp, random-artifact wrappers and seed formula, monster-count saturation, ProcessOnMapHeroes mobility, Deallocate reset and class stat table, damage-forecast routine, siege-wall test, pointer table to the combat-status strings |
| `ProcessAssert` `0x444b5a–0x444bad` | body replaced by `ret` (X03) | tail of the siege-wall test; strings `"Атаковать"`, `"Стрелять в"`, `"%s %s.  Урон:  %d - %d."` and a 3-pointer table to them at `0x4448bd` |
| `combatManager::UpdateArmyGroup` `0x419808–0x419864` | rewritten more compactly (X13) | middle part of the siege-wall test |
| `hero::HeroView` `0x439c7d–0x439cd7` | post-dialog mobility recalculation removed (X26) | `RoundDamage(count, multiplier)` helper used by the damage forecast |
| `mouseManager::SetPointer` `0x46b98f–0x46bf4b` | runtime cursor synthesis replaced (X36) | resource cursor loader (X07) |

### 2.3 Change list

Each entry gives the TE id, VA range(s), the reconstructed function (unit),
original behaviour from our matched source, TE behaviour, class, and the
proposed `source-te` expression. `changes.tsv` carries the same rows.

#### Combat

**X10 — Ghost retaliation counts kills twice (bug fix).**
`army::DoAttack` (SOURCE/ARMY) `0x416beb–0x416c27`. Retail, after the target
retaliates, runs `if (targetPtr->m_creatureType == CREATURE_GHOST)
targetPtr->m_quantity += m_ghostKills[...]`; the nested `targetPtr->DoAttack(1)`
already applied the same addition for the retaliating ghost, so retaliating
ghosts grew by twice their kills. TE deletes the outer block (and reuses the
space as the loader cave). *source-te:* delete the outer block in
`army::DoAttack`. *Risk:* changes combat outcome; network peers must run the
same build.

**X11 — Bad luck as likely as good luck (balance/bug fix).**
`army::CheckLuck` (SOURCE/ARMY) `0x417066`: `jge` → `jg`, i.e.
`SRandom(1,12) < -luck` becomes `<= -luck`, matching the good-luck test
`SRandom(1,12) <= luck`. *source-te:* change the comparison. *Risk:* combat
RNG outcome; multiplayer must match.

**X12 — Quick-combat losses: no 1-in-101 survival and a guaranteed survivor
(bug fix).** `armyGroup::DamageGroup` (SOURCE/ARMYGRP) `0x418a2f`,
`0x418a60–0x418a78`: `SRandom(0,100) < killChance` → `SRandom(0,99) <
killChance` (a 100 % kill chance no longer spares 1 in 101 creatures); the
first stack keeps one creature when wiped regardless of
`damagePercent < 0.999` (the later `damagePercent >= 1.0` wipe still applies).
*source-te:* edit `DamageGroup`. *Risk:* AI-vs-AI and auto-resolved results.

**X13 — Survivors written back by stack position (bug fix).**
`combatManager::UpdateArmyGroup` (SOURCE/CMBTMGR) `0x419730–0x419807`.
Retail maps each combat stack to the *first* army slot holding the same
creature type, so an army with two stacks of one creature wrote both results
to the first slot and left the second unchanged (losses vanish or duplicate).
TE walks combat stacks and non-empty slots in lockstep. *source-te:* rewrite
the loop with two indices. *Risk:* none for saves; outcome differs only for
duplicate-type armies.

**X14 — Wandering-monster count saturates at 127 (bug fix).**
`combatManager::Close` `0x419679–0x41968b` → cave `0x444897`: the surviving
monster total stored in the map cell (`m_battlefieldCell->m_objectMetadata`,
a signed byte) is clamped to `0x7f` instead of wrapping. The `0x4195d4–0x4195f1`
run in the same function is a re-encoding of the `UpdateArmyGroup(ii)` loop
(`movsx` for `mov cl`, `inc` for `add 1`) with no behavioural change.
*source-te:* clamp before the store. *Risk:* none.

**X15 — Experience never negative per stack (bug fix).**
`combatManager::ExperienceValueOfStack` `0x41c770` → cave `0x44487a`: a
stack's `(m_initialQuantity - m_quantity) * hitPoints` is added only when
positive, so stacks that grew (ghosts, resurrection) no longer subtract
experience. *source-te:* `if (lost > 0) num += …`.

**X16 — Damage forecast in the combat status bar (UI).**
`combatManager::CombatMessage(i16)` (SOURCE/DRAWING) `0x4238f7–0x423947`,
jump-table entry `0x423a0e` (the shoot case now targets the attack handler).
"Attack %s" and "Shoot %s (%d shots left)" become
`"%s %s.  Урон:  %d - %d."` with "Атаковать"/"Стрелять в" — a min–max damage
estimate computed by the new routine at `0x4449d2`: attacker attack − target
defence clamped to ±20 and looked up in `gBattleStat`, × quantity; −4 to that
difference when a left-side shooter fires across an intact castle wall
(`combatManager+0x6d3` siege flag, wall cells type 8/10 in column 5); halved
for a shooter in melee; rounded and clamped to 1..32000 by the helper at
`0x439c8f`; min=max under bless (damage mode 1) or curse (mode 3). The shot
count is no longer shown. EN text and an alternative format come from the
text plugins, whose `[BATTLE_STATUS_BAR_MESSAGE] Format` selects TE-style
(0, default), Classic+ (1) or Classic (2) messages (section 4). *source-te:* new `combatManager::EstimateDamage`
reusing the real damage formula, plus a `[BATTLE_STATUS_BAR_MESSAGE]`-style
option; strings into localization. *Risk:* UI only.

**X17 — AI spell damage evaluation ignores off-board hexes (bug fix).**
`combatManager::EffectSpellDamage` (SOURCE/SPELLAI) `0x459b1e–0x459b46`: adds
`hex >= 0` before indexing `m_hexCells[hex]`
(`GetAdjacentCellIndexNoArmy` returns −1 at the board edge). *source-te:*
guard in the loop.

**X18 — Resurrect animation (UI).** `.data` `0x49360e`: `gCombatFxNames[4]`
(`COMBAT_EFFECT_RESURRECT`) `"magic01.icn"` → `"magic05.icn"`.
*source-te:* edit the initializer in SOURCE/KB.

#### Morale, luck and artifacts

**X20 — Fizbin of Misfortune: luck −3 instead of morale −2 (balance).**
`armyGroup::GetMorale` `0x4185a2`, `0x4185ac–0x4185c5` drops the
`HasArtifact(ARTIFACT_FIZBIN_OF_MISFORTUNE)` −2 morale; `game::GetLuck`
`0x435f8c` → cave `0x44475a` subtracts 3 luck for it before the luck
artifacts; `game::ShowMoraleInfo` `0x44092f`, `0x440944–0x440968` stops
listing it; `game::ShowLuckInfo` `0x440b4d` → cave `0x44477e` lists it.
Data: `gMoraleInfoText[12]` `0x49c894` "-2" → "-3" (the same text is reused in
the luck list) and the artifact description `0x493da0` "…сильно уменьшает
мораль" → "…удачу". **Runtime:** `TE.dll` replaces `GetMorale` wholesale
(adding `m_cowardice`), consistent with this removal. *source-te:* move the check from `GetMorale` to
`GetLuck`, the text line from `ShowMoraleInfo` to `ShowLuckInfo`, update
texts. *Risk:* balance; multiplayer must match.

**X21 — Spell book no longer corrupts the boat table (bug fix).**
`advManager::GiveArtifact` (SOURCE/EVENTS) `0x426eda–0x426ef9`:
`gpGame->m_randomArtifacts[artifact] = heroId` only when `artifact <= 36`
(the array has 0x25 entries; `ARTIFACT_MAGIC_BOOK` = 37 wrote into
`m_boats[0]`). *source-te:* bounds check.

**X22 — Deterministic random artifacts per map cell (balance / anti
save-scum).** `advManager::GiveRandomArtifact` `0x426f25–0x426f64` →
cave `0x4448f3`; callers rerouted through wrappers: `advManager::DoEvent`
`0x42535e`, `0x4262ff` and `advManager::DoAIEvent` `0x428204`, `0x428aab`
call `0x444888` (passes the caller's `cell` = first argument);
`advManager::GhostEvent` `0x427347` and `philAI::FightEvent` `0x44ec2a` call
`0x4448ca` (cell = second argument). The artifact becomes
`4 + seed[h] % 33` with `h = (cellIndex + seed[0]) % 31`, `cellIndex = (cell −
advManager+0x9f map base) / sizeof(mapCell)` and `seed[i]` the low byte of
`m_heroRecs[i].m_randomSeed`. `GetRandomArtifactId` is still called (its
result discarded) so the RNG stream is unchanged. Consequences: reloading
does not reroll; the "no artifact left → 1000 gold" fallback is gone and an
artifact already owned can be granted again. *source-te:* new
`game::CellRandomArtifactId(mapCell*)` and an extra `cell` parameter on
`GiveRandomArtifact`. *Risk:* deterministic per save; peers stay in sync as
long as all run TE.

**X23 — Experience gain ignores non-positive amounts (robustness).**
`advManager::GiveExperience` `0x426f91–0x426fd4`: the add is skipped when
`experience <= 0` and both `H1_ASSERT`s (EVENTS.CPP lines 1093/1094) are
removed. *source-te:* early-out instead of assert.

#### Heroes and movement

**X24 — Retreat/rehire exploit closed; dismissed or defeated heroes reset
(balance).** `hero::Deallocate` (SOURCE/HERO) `0x43a33a–0x43a345` → cave
`0x44494c`; `hero::CalcMobility` `0x439017` → cave `0x442266`;
`game::PerDay` `0x433a55–0x433a5b` → cave `0x444702`;
`game::RandomizeHeroPool` `0x4354f4` → cave `0x444726`. TE uses the last
byte of `hero::m_name` (offset `0x12`, normally the terminator of a
16-character name) as a "left the map today" flag:
- every `Deallocate` sets it; `CalcMobility` returns 1 while it is set, so a
  hero rehired from the tavern on the same day can barely move;
- `PerDay` clears it on all 36 heroes at each day change and
  `RandomizeHeroPool` clears it at game start;
- when the hero did **not** retreat or surrender (`!gbRetreatWin`: killed or
  dismissed), `Deallocate` also resets him: experience 0, level 1, primary
  stats from a per-class table at `0x4449c2` (Knight 1/2/1/1, Barbarian
  2/1/1/1, Sorceress 0/0/2/3, Warlock 0/0/3/2), morale and luck 0, all
  artifact slots empty except a spell book for Sorceress/Warlock, all spells
  and charges cleared.
*source-te:* name the byte (e.g. an accessor over `m_name[16]`, keeping the
0xb6-byte record), add the reset to `Deallocate`, the test to `CalcMobility`,
clears to `PerDay`/`RandomizeHeroPool`. **Runtime:** with the shipped
plugins this flag half is inert — `TE.dll` NOPs the flag store
(`0x442254`, 9 bytes) and the `CalcMobility` test (`0x442266`, 18 bytes) and
implements its own retreat/surrender state in `hero+0x34` (section 4, the
`SoftRetreatSurrender` option); the reset half stays active and `TE.dll`
extends it to clear `m_cowardice` and `gbCombatSurrender`. On `source-te`
implement only the `TE.dll` design and the reset. *Risk:* the flag is saved inside the
hero record — retail reading a TE save sees an unterminated 16-character
name only for 16-character names (none in the stock roster); rules differ
from retail, so multiplayer peers must match.

**X25 — Starting heroes begin with 0 experience (balance).**
`game::RandomizeHeroPool` `0x4354ba–0x4354bc`: `Random(0,50) + 40` becomes
`0` (the `Random` call is kept for RNG parity). *source-te:* constant change.

**X26 — Hero screen no longer recomputes movement (behaviour change, side
effect).** `hero::HeroView` `0x439c7d–0x439cd7`: after the dialog TE returns
1 immediately: the `m_mobility = CalcMobility()` refresh and the clamp of
`m_remainingMobility` are gone, and `gHeroWindShowing` is never cleared (the
only reader, `NormalDialog`, then centres adventure-map dialogs instead of
placing them at the map-side position). Return value is ignored by all five
callers. The freed bytes host the rounding helper of X16.
*source-te:* if kept, express as an explicit option; the `gHeroWindShowing`
leak looks accidental — recommend keeping the reset. *Risk:* movement-point
recalculation timing differs from retail (artifact swaps mid-turn).

**X27 — Map-placed heroes start with correct movement (bug fix).**
`game::ProcessOnMapHeroes` `0x436bab–0x436bb8` → cave `0x4448d9`: after
granting the hero's pre-placed artifacts, `m_mobility` and
`m_remainingMobility` are set from `CalcMobility()`. *source-te:* two lines in
`ProcessOnMapHeroes`.

**X28 — Dimension Door outside the map (bug fix).**
`advManager::DimensionDoor` `0x40f969–0x40f970` → cave `0x4447ad`: target x/y
outside 0..71 now takes the "failed" path instead of `GetCell` out of bounds.
*source-te:* bounds check.

**X29 — Hero quick view shows movement (UI).**
`advManager::HeroQuickView` `0x4097c9`, `0x4097d6`, `0x4097e6` → cave
`0x43d078`: the name line `sprintf(gText, "%s", m_name)` becomes
`"%s: %d (%d)"` with `m_remainingMobility` and `m_mobility`, for own and
enemy heroes alike in the static patch; at run time `TE.dll` hooks the cave
(`0x43d07d`) and prints only the name for foreign heroes unless
`ShowEnemyMobility=1` (default 0). *source-te:* format change behind the option.

#### Adventure map and economy

**X30 — Tavern heroes reserved per week (bug fix).**
`game::PerWeek` `0x433ba1`, `0x433dfa–0x433e9b`: retail draws each tavern
hero with `GetNewHeroId` without reserving him, so the same hero can appear
in two taverns. TE marks each drawn hero `HERO_AVAILABILITY_RETREATED` (0x40,
"reserved"), then frees the two heroes of the previous week once both slots
are redrawn; slots of dead players are released immediately (`m_playerDead`).
Frame grows by 8 bytes for the saved ids. *source-te:* rewrite the tavern
loop. *Risk:* availability values are saved; old saves load fine.

**X31 — Remaining hit points in combat creature view (UI).**
`game::ViewArmy` (SOURCE/GAME) `0x431bf3–0x431c0b` → cave `0x43cff1`: while
`gInCombat`, the "Hit Points" line reads `"\n%s%d (%d)"` with the remaining
hit points of the top creature (`m_stats.hitPoints − m_hitPointsLost`).
*source-te:* extra `sprintf` branch. *Risk:* dereferences the army pointer
whenever `gInCombat` is set.

**X32 — Obelisk puzzle completes for any obelisk count (bug fix).**
`game::VisitObelisk` `0x42bdfa–0x42be00` → cave `0x4447d8`/`0x4447f0`: pieces
per visit become `48 / m_obeliskCount` plus one extra for the first
`48 % m_obeliskCount` obelisks visited by the player (counted from
`m_obeliskVisitors`), so visiting all obelisks reveals all 48 pieces.
*source-te:* new helper counting visited obelisks.

**X33 — Ultimate-artifact hint chance as unsigned (bug fix).**
`ComputeUALoc` `0x42bb87`, `philAI::DoAI` `0x448c9b`, `0x449013`,
`philAI::ValueOfEventAtPosition` `0x450a5e`, `0x450a8e`:
`playerData::m_ultimateArtifactHintChance` (offset `0x52`) is read with
`movzx`; values above 127 % no longer turn negative. *source-te:* make the
field `u8`. *Risk:* none (same byte in saves).

**X34 — Shipwreck reads its own cell (bug fix).**
`advManager::DoEvent` `0x4265d8–0x426601`: the shipwreck branch passed
`GetCell(x - normalDirTable[dir].x, y - normalDirTable[dir].y)` (the tile the
hero came from) to `GhostEvent`, which reads the guard size from that cell's
metadata; TE passes `GetCell(x, y)`. With X22 the artifact also keys on the
shipwreck cell. *source-te:* argument change.

#### AI

**X35 — AI quick combat (balance).** `philAI::QuickCombat` `0x44d5bc–0x44d5c3`
→ cave `0x444734`: `Random(0,100)` → `Random(0,99)` and a win chance above
0.75 is raised to 1.0 before the roll. `0x44d784`, `0x44d7ab` (`test ah,0x41`
→ `test ah,1`) with `.rdata` `0x48a620` `0.99` → `1.0` turn `> 0.99` into
`>= 1.0` at both `TransferArtifacts` sites; the shared constant also makes
`QuickCombat`'s `attackerDamage >= 0.99` and `philAI::FightEvent`'s
`dmg < 0.99` compare against 1.0. `TE.dll` additionally fixes the defender-wins branch of
`QuickCombat` (section 4). *source-te:* edit `QuickCombat`/`FightEvent`
literals. *Risk:* AI-vs-AI outcomes; artifacts now transfer only on total
destruction.

**X36 — AI army valuation without integer overflow (bug fix).**
`philAI::FightValueOfStack` `0x44d0ed–0x44d151` → helper `0x444852`: the
morale and luck scalings `worth * (m + 48) / 48`, `(m + 24) / 24`,
`(l + 16) / 16` are computed as `worth * (1 + m / d)` in floating point.
*source-te:* float arithmetic in those three statements.

#### Platform and presentation

**X01 — Plugin loader stub (framework).** `_WinMainCRTStartup` `0x479a38`
(4 bytes) → cave `0x416bed`: save flags/registers, `LoadLibraryA
("Loader_EN.dll")` (RU: `Loader_RU.dll`), `ExitProcess(-1)` on failure,
restore, re-run the overwritten prologue and continue at `0x479a3d`.
*source-te:* out of scope as a byte patch; if plugins are kept, an explicit
loader call in `WinMain` behind an option.

**X02 — No CD check (platform).** `EarlySetup` (SOURCE/KB)
`0x43cfcf–0x43d0c3`: the static re-entry guard, the `InterpretCommandLine`
early return, `SetupCDDrive()` and its four fatal message boxes are removed;
`InitVars()` always runs. Also drops the `HEROES.AGG` presence/`_chdir`
check. *source-te:* option in `EarlySetup`. *Risk:* none.

**X03 — Assertions disabled (robustness).** `ProcessAssert` (SOURCE/kbwin)
`0x444b59` is a bare `ret` (22 callers). *source-te:* compile `H1_ASSERT`
out in the TE configuration.

**X04 — Idle CPU (platform).** `Process1WindowsMessage` `0x44377c–0x4437c1`:
`Sleep(0)` before every `PeekMessage` pass, `GetTickCount` instead of
`KBTickCount`, `GetMessage` throttle 150 → 127 ms. **Runtime:**
`CPUPatchAdv.dll` turns it into `Sleep(1)` and sets `timeBeginPeriod(1)`.
*source-te:* source edit with `Sleep(1)` and the timer period.

**X05 — Windowed defaults (platform).** `SetGameDefaults` `0x443eed`,
`0x443f1f`: default `fullScreen` 1 → 0; `ReadPrefs` `0x4441e7–0x444204`,
`0x44427d–0x44429a`: `gfx[0].showMenu = 1` and `gfx[0].fullScreen = 0` are
forced instead of reading `HMM1 GameShowMenu`/`HMM1 GameFullScreen`; the
registry key (`ReadPrefs`, `WritePrefs`, dead `SetupCDDrive` copies at
`0x49e7c0`, `0x49ea60`, `0x49ed18`) becomes
`SOFTWARE\Buka\3DO\Heroes of Might and Magic\HeroesWorld TE`. Full-screen and
scaling belong to the renderer wrapper. **Runtime:** `fsfix.dll` writes the
retail bytes back at `0x44427d` (30 bytes), so `HMM1 GameFullScreen` is read
again; the forced `showMenu` and the default stay. *source-te:* options; key name as a
build constant. *Risk:* a separate registry key keeps TE and retail
preferences apart.

**X06 — Videos disabled (platform).** `PlaySmacker` (SOURCE/SMACKMGR)
`0x458d56`: the `SmackMain()` call is `nop`ped (the shipped but disabled
`ReturnSMKVideos` plugin restores it). *source-te:* option.

**X07 — Colour cursors from resources (UI/platform).**
`mouseManager::SetPointer` (BASE/MOUSEMGR) `0x46b98f–0x46bf4b`: retail builds
each cursor at first use from the AGG bitmaps (`advmice`/`spelmice`/
`cmbtmice`, colour or monochrome) with `CreateBitmapIndirect`/
`CreateIconIndirect`; TE instead calls `LoadCursorA(hInstance, id)` for ids
94..1 once and stores them so that `hMouseCursor[i]` = resource `i + 20`
(75 cursors, ids 20–94, in `.rsrc`). Hot spots come from the cursor resources.
**Runtime:** the enabled `ReturnBWCursors.dll` writes the exact retail
1,469 bytes back, so the shipped configuration uses retail cursors; the
75 resource cursors are only used when that plugin is removed.
*source-te:* keep runtime synthesis; ship cursor resources only behind an
option, or generate them at build time from the AGG.

## 3. Data, `.rdata` and `.rsrc` changes

### 3.1 `.rdata` (9 bytes)

| Id | VA | Change | Meaning |
|---|---|---|---|
| D01 | `0x48a368–0x48a36b` | `0x000a9000` → `0x000d4000` | Debug directory `PointerToRawData`: the CodeView `NB10` record (PDB path `…\release\game\heroes.pdb`) moved behind the grown `.rsrc`. Linker artefact. |
| X35 | `0x48a620` | double `0.99` → `1.0` | PHILAI literal-pool constant shared by four comparisons in `QuickCombat` and `FightEvent` (see X35). |

### 3.2 `.data` (146 bytes, CP1251)

| Id | VA (string start) | Owner | Retail → TE |
|---|---|---|---|
| D02 | `0x490e30` | `gDataPath` | `.\DATA\` → `.\DATA\EN\` (RU: `.\DATA\RU\`); also the base for saves and high scores (`SaveGame`, `LoadGame`, `highScoreManager::Update`, …) |
| D02 | `0x490f90` | `gAnimPath` | `.\ANIM\` → `.\ANIM\EN\` (RU: `.\ANIM\RU\`) |
| D03 | `0x49124f` | byte before the `"\TRACKS\"` literal used by `PlayMusic` | `0` → `.`; no static reader of the dotted string exists in the executable or any DLL. `TE.dll` rebuilds the music path in `PlayMusic` itself (TE-OPT-6), so the byte is inert. |
| X18 | `0x493608` | `gCombatFxNames[COMBAT_EFFECT_RESURRECT]` | `magic01.icn` → `magic05.icn` |
| X20 | `0x493da0` | Fizbin of Misfortune description | `Символ неудачи\n\nСимвол неудачи сильно уменьшает мораль.` → `…сильно уменьшает удачу.` ("greatly reduces morale" → "luck") |
| D05 | `0x49679c` | demon-cave reward event text | typo `поучаете` → `получаете` ("you receive"); the tail moves one byte into the existing padding |
| D06 | `0x499f60` | "no modifiers" line of the morale/luck panels | `\nнет` → `\nНет` (capitalised) |
| X20 | `0x49c894` | `gMoraleInfoText[12]` | `\nСимвол неудачи -2` → `\nСимвол неудачи -3` (now listed in the luck panel) |
| D04 | `0x49de28` | loading-window title (`oldmain`) | `Загрузка Героев Меча и Магии (версия 1.1)` → `# Загрузка Героев Меча и Магии TE 1.04 #` |
| X05 | `0x49e7c0`, `0x49ea60`, `0x49ed18` | registry key used by `ReadPrefs`, `WritePrefs` and (dead) `SetupCDDrive` | `SOFTWARE\Buka\3DO\Heroes of Might and Magic Platinum\1.000` → `SOFTWARE\Buka\3DO\Heroes of Might and Magic\HeroesWorld TE` |

New strings are also embedded in `.text` caves (Russian in both builds):
`"%s: %d (%d)"` and `"\n%s%d (%d)"` in the `EarlySetup` cave,
`"Loader_xx.dll"` in the `DoAttack` cave, `"Атаковать"`, `"Стрелять в"`,
`"%s %s.  Урон:  %d - %d."` in the `ProcessAssert` cave. The text plugins
replace them for English.

### 3.3 `.rsrc`

Retail: one Russian (1049) set — `ICON 1`, `GROUP_ICON 109`, `MENU`
`MNUADV`/`MNUCMBT`/`MNUDFLT`/`MNUTOWN`, `DIALOG HEROES`. TE (identical in EN and
RU executables):

| Id | Resources | Content |
|---|---|---|
| R01 | `MENU` ×4, `DIALOG HEROES`, `ICON 1`, `GROUP_ICON 109`, language 1033 | Menus are the original English NWC menus (File/New Game/…, Control Panel, Adventure, Display, Help) with the same command ids; the RU build therefore also shows English Win32 menus. The about box reads "Heroes of Might and Magic / Tournament Edition 1.05 f3 / with plugins support / Copyright 1995-6, New World Computing" (the version differs from the "TE 1.04" loading banner). Icon bytes are unchanged. |
| R02 | `CURSOR` ×75 + `GROUP_CURSOR` ×75, ids 20–94, language neutral | 32×32, 8-bpp colour cursors (2,220 bytes each) for X07; id *n* fills `hMouseCursor[n-20]`. |

*source-te:* menus and the about text as per-language resource scripts with
one version constant; cursors either generated at build time from the AGG
mouse bitmaps or omitted (the shipped configuration does not use them).


## 4. Runtime layer

Scope: everything the edition loads besides the patched executable. Addresses
are HEROES.EXE virtual addresses (image base 0x400000); function names come
from `config/retail/buka-function-map.tsv`. "Cave" means a region of the TE
executable that the in-exe patch rewrote (section 2).
Patcher vtable slots were identified from call shapes: +0x00 WriteByte,
+0x04 WriteWord, +0x08 WriteDword, +0x0C WriteJmp, +0x10 WriteHexPatch,
+0x14 WriteCodePatchVA, +0x18 WriteLoHook, +0x1C WriteHiHook. Every
WriteHiHook uses (SPLICE, DIRECT, THISCALL).

### 4.1 Load chain

| Step | Component | Evidence |
|---|---|---|
| 1 | `H1TE_xx.bat` runs `start /AFFINITY 1 H1TE_xx.exe` (pins the game to one CPU core) | bat text |
| 2 | Static imports resolve. The TE executable still imports `smackw32.DLL` and `audiere.dll`, which the archive does not ship, so the package is an overlay on an existing installation that supplies them, along with `Maps\`, `Sound\` and `Tracks\` (inferred). `WINMM.dll` and `WING32.dll` in the game directory take precedence over the system copies. | import table; archive listing |
| 3 | The entry point (0x479a38) jumps to a cave in `army::DoAttack` (0x416bed) that calls `LoadLibraryA("Loader_xx.dll")` and `ExitProcess(-1)` on failure, then resumes the CRT entry | exe patch |
| 4 | `Loader_xx.dll` (DllMain 0x10001350) enumerates `.\Plugins\xx\*` and loads each file whose extension is `dll` with `LoadLibraryW`. A failure raises the message box "Cannot load "…"" titled "Plugin Loader". Files named `*.dl_` are therefore disabled. | Loader disassembly 0x10001000-0x100015cf |
| 5 | Each plugin calls `LoadLibraryA("patcher_x86.dll")` + `_GetPatcherX86@0`, creates an instance named `HD.Plugin.…`, and installs its patches inside DllMain | each plugin |
| 6 | `WING32.dll` (renderer wrapper) loads `mods\*.mod` (`cdda.mod`, `hi_combat.mod`) and reads `config.ini` | WING32 strings `mods\`, `*.mod`, `config.ini` |

The executable itself only differs between EN and RU in the Loader name
(0x416c19) and in the `DATA\xx` / `ANIM\xx` paths. All other language
differences live in this runtime layer.

### 4.2 Third-party framework and platform components (not edition behaviour)

| File | Identity | Role | source-te recommendation |
|---|---|---|---|
| `patcher_x86.dll` + `patcher_x86.ini` (`Logging = 0`) | patcher_x86 4.18.2.0, a hooking library (exports `_GetPatcherX86@0`, `_GetPatcherX86Version@0`) | runtime hook engine for every plugin | out of scope: a source build compiles the changes in |
| `WING32.dll` | "HoMM I & II - OpenGL wrapper" (HeroesGL 1.7.0, 2022-01-05). It exports the 10 WinG entry points and imports OPENGL32. | Replaces WinG with a GDI/OpenGL renderer that provides windowed and fullscreen modes, scaling filters, a colour pipeline and menus. It loads `mods\*.mod` and owns `config.ini` `[Wrapper]`, `[Colors]` and `[FunktionKeys]`. It contains the `Application` section name plus `RegOpenKeyExA`/`RegQueryValueExA`/`RegSetValueExA`/`RegCreateKeyA` name strings, consistent with redirecting the game's registry preferences into `config.ini` `[Application]` (inferred; the redirect code was not traced). | out of scope (renderer/platform). If kept, it remains a drop-in DLL beside a source build. |
| `WINMM.dll` | "WinMM Cleaner" 2021.01.15.004 (original name dplayx.dll) | pure pass-through proxy: DllMain resolves about 190 exports from `%SYSTEM%\WINMM.dll` and every stub jumps through that table (0x10001006…, resolver 0x10001847). It adds no behaviour of its own. | out of scope |
| `MSS32.DLL` | Miles Sound System V3.6C (1997) | replaces the retail Buka MSS V3.50F (1996, 144,384 B) with a newer build of the same API | out of scope (redistributable) |
| `mods\cdda.mod` | "HoMM I & II - Music Mod" (HeroesCDDA 1.1.0) | CD-audio emulation: intercepts `mciSendStringA`/`_AIL_redbook_*` and plays `.mp3/.flac/.ogg/.wav` from `.\Tracks`, `.\Tracks2` or `.\MUSIC`. The Buka executable already plays OGG through audiere, so this mod may be inert for this build (unresolved). | out of scope |
| `mods\hi_combat.mod` | "HoMM I - Combat Mod" (Heroes1Combat 1.0.2) | Version-keyed address tables. The first record matches this build: `AppInit+145`, `combatManager::DrawBackground+1ae`, `combatManager::ProcessCombatMsg+ff/+11f`, `combatManager::DrawFrame` (+0, +bcd, +be0), `combatManager::Main+250/+260`, `AppWndProc+694`, `gpCombatManager` 0x4a7828. It adds a combat hex grid, a shadow of the movement range and a shadow cursor, plus a speed control and a menu. `config.ini` `[HI_COMBAT]` holds `ShadowMovement`/`EnableGrid`/`ShadowCursor` (0/1) and `Speed`. | Optional UI feature. It could be re-expressed as source options in `combatManager::DrawFrame`/`DrawBackground`, but it is outside TE edition semantics, so keep it out of scope initially. |
| `Help\winhlp32.exe` | Microsoft WinHelp 4.90.3000 | viewer for `HEROES.HLP` on systems without WinHelp | out of scope |

`config.ini` keys:
- `[Wrapper]`: `UseOpenGL`, `Language` (wrapper UI language: 1033 in EN, 1049 in RU, the only difference between the two `config.ini` files), `Title` (window title "Heroes of Might and Magic TE"), `Renderer`, `ColdCPU`, `PointerFix`, `ImageAspect`, `ImageVSync`, and filter parameter words (`Interpolation`, `Upscaling`, `ScaleNx`, `XSal`, `Eagle`, `ScaleHQ`, `XBRZ`).
- `[FunktionKeys]`: `ImageFilter=3` and `WindowedMode=4` (F3/F4 hotkeys), `AspectRatio`, `VSync`.
- `[Colors]`: hue/saturation plus RGB input, gamma and output levels.
- `[Application]`: the game's own preference values (`HMM1 MusicVolume` … `HMM1 GameFullScreen`, `HMM1 CDDrive`, `AppPath`). The names match the registry values read and written by `ReadPrefs`/`WritePrefs` (key now `…\HeroesWorld TE`). The wrapper is the presumed store (see above).

### 4.3 Edition plugins (Plugins\xx\*.dll, enabled)

#### 4.3.1 TE.dll (`HD.Plugin.H1TE.TE`): core edition behaviour

It reads `.\Plugins\xx\TE.ini` (the path is built from `GetCurrentDirectoryA`). Options:

| Key | Default (code / shipped ini) | Effect |
|---|---|---|
| `<ShowEnemyMobility>` | 0 / 0 | 0 limits the movement-points line added to the hero quick view (exe cave reached from `advManager::HeroQuickView`) to own heroes. Hook `EarlySetup+b2` (0x43d07d, in a cave) prints only the name when the hero's owner is not `giCurPlayer` and `advManager+0x25c` is clear. 1 shows mobility for all heroes. |
| `<SoftRetreatSurrender>` | 0 / 0 | Controls the movement of a hero re-hired after retreating or surrendering (see the retreat/surrender block). 0 means the hero has no movement left that day. 1 means the hero keeps his leftover movement (at least 1). |
| `<SlightlyHarderAI>` | 0 / 0 | Installs the AI block below (0x100041f5 skips it when 0). |
| `<CheatMode>` | 1 / **2** | 0 disables the digit cheat sequence (hook `advManager::Main+5cd` jumps past it). 1 keeps the original 101495 reveal-map cheat. 2 adds the extended cheat codes (hook `advManager::Main+5e6`). |
| `<OriginalCheatKeys>` | 0 / 0 | 1 writes the dword 1 at `InterpretCommandLine+13` (0x43de25), so `giDebugLevel` starts at 1. That enables the F3/F5/F6/F7/F8/F9/F11/F12 debug cheats in `advManager::Main`, and also bypasses the quicksave hook below, which sits after the debug-level branch. |
| `<LosslessAudio>` | 0 / 0 | Music file path used by `PlayMusic` (see the audio hook). 1 selects `<cwd>\Audio\Track NN.flac`; 0 selects `<cwd>\Tracks\NN-AudioTrack NN.ogg`. |
| `[Texts] <Visited>`, `<SavedSuccessfully>`, `<LoadQuicksave>`, `<FileNotFound>` | English defaults compiled in | strings for the quick-info "(Visited)" suffix and the quicksave dialogs (the RU ini translates them) |

Hooks (EN; RU is identical except where noted in 4.5):

**Gameplay bug fixes**

| VA | Function | Kind | Effect |
|---|---|---|---|
| 0x447fc5 | philAI::CheckBuyStuff+1fe | LoHook | The AI buys a boat only if the dock cell `m_map[x-1][y+1]` has no trigger and no hero flag (`m_flags&0x40`). Otherwise the boat order stays reserved (jumps to +285). |
| 0x461311 | townManager::Main+b36 | LoHook | The shipyard "build boat" check now tests only that the dock cell trigger is empty. It drops the original test `cursorMapX != x-1 && cursorMapY != y+1`, which refused a boat whenever the selected hero shared the dock's row **or** column. |
| 0x415490 | army::SpecialAttack+c3c | LoHook | Elves (creature 14) fire their second shot only if `m_stats.shots >= 1` (army+0x1e). |
| 0x44d72e, 0x44d764 | philAI::QuickCombat+20d/+243 | WriteByte 0x14 / 0xEC | AI auto-resolve, defender wins: `ApplyBattleWinTemps` now runs on `defenderHero` instead of `attackerHero`, and `GiveExperience(defenderHero, …)` uses the attacker-side experience (`[ebp-0x14]`) instead of the defender's own (`[ebp-0x10]`). These are copy-paste bugs in the original. |
| 0x44d708 | philAI::QuickCombat+1e7 | LoHook | Attacker wins: the losing defender hero also gets the loss temps and the cowardice penalty (see the morale block), which the original never applied. |
| 0x44d6df | philAI::QuickCombat+1be | LoHook | Auto-resolved town battle (`townBattle`): the winning attacker gets +500 experience, mirroring the castle bonus of interactive combat. |
| 0x451077, 0x451084 | recruitUnit::Open+17e/+18b | LoHook | The maximum recruitable count is kept as 32 bits (a dword store at +0x50 and the full value in eax) instead of being truncated to `i16` (more than 32,767 affordable). The dword store also overwrites the following field (risk). |
| 0x40c91a, 0x40cba5, 0x40c9e6, 0x40cc71 | advManager::ViewWorld+6d7/+962/+7a3/+a2e | LoHook | View-World handling for heroes standing on a mine. The case tests are rewritten (+6d7 requires the event bit: 0x81/0x99/0xa0), and the mine-type letter is drawn from a hero-record byte instead of the hero index used as a mine index. The handler reads hero+0x23 (`m_locationType`), which looks like a slip for +0x24 (`m_occupiedTown`, the mine index); verify in game. |
| 0x434a8a, 0x434a9d | game::RandomizeTown+46c/+47f | WriteByte 0x40, WriteJmp →+4aa | A random town flagged as castle gets only `BUILDING_SLOT_CASTLE`. It no longer gets a free `BUILDING_SLOT_DWELLING_1` (0xc0→0x40) or the initial `m_garrison[0] = growth`. This is a balance change. |
| 0x4314d4 | ViewSpellsHandler+436 | WriteByte 0xEB | Disables the trailing `if (message.id == WIDGET_COMMAND_DIALOG_SELECT) forward` path in the spell book, so widget id 10 no longer acts as "select". The motivation is unresolved (likely a stray close/select). |
| 0x434d89 | game::SetupTown+202 | LoHook | Before the original memset, writes the town index into `m_objectMetadata` for the 4×3 cells x-2..x+1 by y-2..y (bounds 0..71) of each town footprint. |

**Retreat / surrender / morale (balance)**

TE.dll reuses the unused fifth primary-stat byte (`hero::m_primaryStats[4]`,
hero+0x34) as a per-day "fled" state: 1 = normal, 0xFE = retreated,
0xFF = surrendered. It also activates `hero::m_cowardice` (hero+0x37).

| VA | Function | Kind | Effect |
|---|---|---|---|
| 0x4213af | combatManager::ProcessNextAction+2ef | LoHook | On retreat, marks the retreating side's hero +0x34 = 0xFE. |
| 0x4213b4 | combatManager::ProcessNextAction+2f4 | LoHook | Surrender branch. A human side marks the hero 0xFF and copies the AI-computed cost (0x4a6ab4) into the surrender cost (0x4a6a88). For a computer side the hook calls `combatManager::DoSurrender`. If the offer is refused and the hero has not cast this round, the AI casts its best damage spell among Turn Undead, Armageddon, Storm, Meteor Shower, Fireball and Lightning Bolt (11/15/16/17/0/1) instead, and the hook resumes at +2d3. |
| 0x411d6f | combatManager::AICheckRetreat+70f | LoHook | When the AI decides to flee, it computes its surrender cost (sum of cost/2 × count over its stacks) into 0x4a6ab4. If its gold is at least that cost + 2500, `giNextAction = 5` (surrender, keeping the army) instead of 4 (retreat). |
| 0x44e559 | philAI::BuildHero+ac | LoHook | The AI re-hiring a surrendered hero (0xFF) skips `game::SetRandomHeroArmies`, so the hero keeps his army. |
| 0x44e593, 0x464e7a | philAI::BuildHero+e6, townManager::RecruitHero+433 | LoHook | Re-hire of a retreated or surrendered hero (+0x34 < 0): the state resets to 1. With `SoftRetreatSurrender` the hero gets `m_mobility = max(remaining,1)`. Otherwise remaining = 0 and full mobility is recomputed. |
| 0x433a56 | game::PerDay+388 | LoHook | Each day resets +0x34 = 1 for all 36 heroes, then continues into the exe cave (`DriveSupportsFreeSpaceQuery+1a`). |
| 0x442254 (+9 B), 0x442266 (+18 B) | cave inside IsCDDrive | WriteCodePatch "%n" (NOPs) | Removes the exe's own version of this feature: the hero+0x12 flag set during deallocation and the `CalcMobility` "flag ⇒ mobility 1" check. TE.dll supersedes it. |
| 0x44495b, 0x44495e | cave inside SetupCDDrive (hero reset path from `hero::Deallocate`) | LoHook | The reset also clears cowardice (+0x37 = 0), and `gbCombatSurrender` is cleared. |
| 0x43a89e | hero::ApplyBattleLossTemps | HiHook (replace) | Unless the battle ended by surrender, `m_cowardice -= 1` (floor -3). Then the original temp-event cleanup runs (re-implemented at 0x10001020). |
| 0x43a725 | hero::ApplyBattleWinTemps | HiHook (replace) | `m_cowardice = min(m_cowardice+1, 0)`, then the same cleanup. |
| 0x4184ee | armyGroup::GetMorale | HiHook (replace) | Re-implementation: alignment + knight bonus + `m_morale` + four medals + **`m_cowardice`** + tavern, clamped to ±3. The Fizbin of Misfortune −2 morale term is **dropped**; the exe patch moves that artifact to luck, see X20. |

**SlightlyHarderAI block (option)**

| VA | Function | Kind | Effect |
|---|---|---|---|
| 0x44e302 | philAI::ChooseEvaluateBattle+8d | LoHook | Battles with a win chance ≤ 0.75 are rejected (jumps to +79). |
| 0x44f66e, 0x44fe98, 0x44fb48 | philAI::ValueOfEventAtPosition+6ba/+ee4/+b94 | LoHook | If `gWinChance` ≤ 0.75, the event value is −32000. |
| 0x44f261 | philAI::ValueOfEventAtPosition+2ad | LoHook | The own-object case returns −1000 instead of 0 and exits. |
| 18 sites: 0x44f3c6, 0x44f628, 0x44f6d8, 0x44f729, 0x44fedf, 0x44ff84, 0x4501dd, 0x450209, 0x45024a, 0x4503ce, 0x450421, 0x4505e2, 0x4507b5, 0x45086a, 0x4509d4, 0x450a3f, 0x450a4e, 0x450ad6 | philAI::ValueOfEventAtPosition | WriteDword −1000 | The neutral or "nothing to gain" valuations (0, −100 at +fd0, −200 at +1a20) become −1000, so the AI stops wandering to worthless or visited objects. |
| 0x448be8 | philAI::DoAI+179 | LoHook | Planning horizon `mobility + 0x7E` instead of `+ 0x2A`. |
| 0x44b644 | philAI::CreaturesToBuy | HiHook (replace) | `min(arg, int(MaxBuyableCreatures × 0.8))` when the maximum is above 1 (keeps a 20 % reserve), and 0 if the result is below 1. |
| 0x427d22 | advManager::DoAIEvent+45 | LoHook | Skips `hero->m_remainingMobility -= 1` per AI event visit. |

**Cheats**

| VA | Function | Kind | Effect |
|---|---|---|---|
| 0x402f58 | advManager::Main+5cd | LoHook | With `CheatMode=0` the digit sequence is ignored. |
| 0x402f71 | advManager::Main+5e6 | LoHook | With `CheatMode=2`, after 101495, the extended codes work. **101496**: wood/ore +300, other rares +100, gold +100,000. **101497**: current town becomes a castle with every building, dwellings filled. **101498**: unlimited movement for the current hero. **101499**: all artifacts and spells, unlimited spell charges. **101500**: all of these combined. **101501–101510**: experience to a level. **101600–101627**: add creature N. **101700–101737**: give artifact N. The individual code assignments are approximate (read from the handler's compare chain). |
| 0x404650, 0x404a0c | advManager::ProcessSearch+2f/+3eb | LoHook | The unlimited-movement flag (table 0x100073a8) skips the search mobility checks and costs. |
| 0x424b43, 0x424c68 | advManager::DoEvent+f3/+218 | LoHook | The flag skips the mobility zeroing on boarding or leaving a boat. |
| 0x4331ee | game::NextPlayer+262 | LoHook | The flag sets mobility and remaining mobility to 32000 each turn. |
| 0x439566, 0x439629 | hero::AddSpell+c7/+18a | LoHook | The unlimited-spells flag (table 0x10007548) keeps the higher of the old and new charges. |
| 0x43d27f | oldmain+1bb | LoHook | Clears both cheat flag tables at startup. Note that they are not saved. |

**UI / quality of life**

| VA | Function | Kind | Effect |
|---|---|---|---|
| 0x402caa | advManager::Main+31f | LoHook | **Quicksave/quickload.** F5 (scan 0x3F) saves `QUICKSAVE` and shows "Game saved successfully.". F9 (scan 0x43), outside network play, asks a yes/no question, then loads `Games\QUICKSAVE.GM<n>` (n = human-player count) or `QUICKSAVE.CGM` (campaign), restores hero or town context, or reports "A quicksave not found.". |
| 0x4075d5 | advManager::QuickInfo+35f | LoHook | Right-click on visitable objects shows "%s\n(Visited)" when the current hero or player has used them. It covers hero event flags (`+0xAE` bits 2/4/8/0x10/0x100), the visited-site bitmask (+0x39) and obelisks (game+0x145a5). |
| 0x408ebb | advManager::UpdBottomViewKingdom+245 | LoHook | Kingdom resources are shown as `%dk` (gold ≥ 100,000; others ≥ 10,000) or `%dm` (≥ 1,000,000). |
| 0x4092ce, 0x40930e, 0x40941c, 0x4094ae; WriteDword 0x409292=4, WriteByte 0x40929c=0xE8, 0x4092a4=0x00, 0x4092a6=0x8C | advManager::UpdBottomViewHero | mixed | The hero panel draws army slots 4→0 instead of 0→4 (the loop counts down), shows counts ≥ 10,000 as `%dk`, and adjusts label offsets. |
| 0x413c52, 0x413c7f | army::DrawToBuffer+41e/+44b | LoHook | The battlefield stack count is shown as `%dk` when ≥ 1,000, with the text shifted by one pixel. |
| 0x41af66, 0x41b452, 0x41b60b, 0x41b249, 0x41b835, 0x41b9aa, 0x41bac3 | combatManager::CatAttack | LoHook | Each catapult-animation redraw extent is forced to the full screen (`giMinExtentX/Y=0`, max 639×479). |
| 0x41bb88 | combatManager::CatAttack+cec | LoHook | Extra `DrawFrame(1)` at the end, so no catapult residue remains. |
| 0x43b294 | HeroHandler+1df | LoHook | At level ≥ 12 the next-level experience value returned by `hero::GetExperience` is increased by 1 before display. The purpose is inferred: avoid showing a threshold that has already been reached. |
| 0x459020, 0x4592bb | combatManager::DetermineEffectOfSpell+aa/+345 | LoHook | Spell-effect evaluation is replaced with a TE routine (0x10001a20). Exact semantics unresolved (resist/target computation for two spell classes). |
| 0x469418 + WriteDword 0x469426 | PlayMusic+11b/+129 | LoHook + immediate | The music path is built from the current directory instead of `gcRegCDRomPath` (CD) + `\TRACKS\`. The format follows the `LosslessAudio` option. |

**Scripted content**

| VA | Function | Kind | Effect |
|---|---|---|---|
| 0x424b0d | advManager::DoEvent+bd | LoHook | Adds behaviour to `MAP_OBJECT_MEGALITH` (47), which has no event in the original: the "Paladin stronghold". The player confirms entry, fights 10 Paladins through `CombatMonsterEvent`, and on victory 10 Ghosts join (`armyGroup::Add(26,10)`). If there is no room, the ghosts wait (cell metadata = 2) and join on a later visit. Afterwards the stronghold is "abandoned". The texts are English in **both** EN and RU builds. `Games\PaladinStronghold.GM1` is a bundled save (77,663 B). |
| 0x41a373, 0x420214 | combatManager::LoadIcons+7b, DoVictory+3a3 | LoHook | When `m_campaignScenario == 8` and the current hero stands at (41,37), the battle uses `dirt.xtl` and the victory music track 0x2D instead of 0x2C. This is the stronghold location in the last campaign map (inferred). |

#### 4.3.2 fsfix.dll (`HD.Plugin.H1TE.FullscreenFix`)
WriteHexPatch at `ReadPrefs+2ee` (0x44427d), 30 bytes. The exe patch replaced
the registry read of `HMM1 GameFullScreen` with `gConfig.fullScreen = 0`
plus NOPs. This plugin writes back the **retail bytes exactly**, so the
preference is honoured again. Net runtime effect equals retail.
Recommendation: no source change (keep retail `ReadPrefs`); the pair exists
only to keep the wrapper in control when the plugin is removed.

#### 4.3.3 ReturnBWCursors.dll (`HD.Plugin.H1TE.ReturnBWCursors`)
WriteHexPatch at `mouseManager::SetPointer+dd` (0x46b98f), 1,469 bytes. These
are the **retail bytes exactly** (verified), which undoes the exe's
SetPointer rewrite (alternative cursor rendering) and restores the original
black-and-white cursors. Net runtime effect equals retail.
Recommendation: model the exe cursor rewrite, if at all, as an option that
defaults off.

#### 4.3.4 CPUPatchAdv.dll (`HD.Plugin.CPUPatchAdv`)
WriteByte 1 at `Process1WindowsMessage+7` (0x44377c). The exe patch inserted
`Sleep(0)` at the top of the message pump, and this makes it `Sleep(1)`. The
plugin also calls `timeBeginPeriod(1)` on attach and `timeEndPeriod(1)` on
detach. Effect: lower CPU use with 1 ms timer resolution. Recommendation:
platform source edit in `Process1WindowsMessage` (`Sleep(1)`) plus the timer
period in startup/shutdown.

#### 4.3.5 Text plugins: EnglishTexts.dll (EN) / MinerTexts.dll (RU)

Both carry the same 275-entry table of code addresses, which are the
`push offset string` sites that they repoint. Both read an ini with the same
section families:

ART 38, SPL 29, CMP 9, CLASS 4, HERO 36, MON 28, TOWN 36, OBJ 63, OBJTXT 77,
BLD 31, SIZE 6, RES 7, MINE 7, TRN 7, WEEK 15, MONTH 10, MISCA 139, MISCB 81,
MISCC 29, MISCD 94, MISCE 55, MISCF 10, STR 268, XSTR 10, and
`[BATTLE_STATUS_BAR_MESSAGE] Format=0`. The Format values are 0 TE-style,
1 Classic+, 2 Classic.

The keys are `Name`, `Desc`, `Text`, `Plural`, `Short` and `Alt`. Each
family overwrites a pointer table, as documented by comments in
EnglishTexts.ini:

| Family | Table | Symbol |
|---|---|---|
| CLASS | 0x49257c | `gClassNames` |
| HERO | 0x492f50 | `gHeroNames` |
| MON | 0x49258c | — |
| TOWN | 0x49291c | — |
| OBJ | 0x492820 | `gObjectNames` |
| OBJTXT | 0x4929ac | — |
| BLD | 0x492750 | `gNeutralBuildingNames` |
| BLD | 0x4931b4 | `gNeutralBuildingDescriptions` |
| BLD | 0x492c94 | `gDwellingDescriptions` |
| SIZE | 0x492cf4 | — |
| RES | 0x4927e8 | — |
| MINE | 0x492804 | — |
| TRN | 0x4927cc | — |
| WEEK | 0x492c58 | `gWeekNames` |
| MONTH | 0x492c30 | `gMonthNames` |
| MISCA | 0x492d24 | `gHeroScreen` |
| MISCB | 0x493070 | — |
| MISCC | 0x4931d0 | `gMoraleInfoText` |
| MISCD | 0x4932b0 | `gDifficultyNames` |
| MISCE | 0x492ae0 | — |
| MISCF | 0x492554 | — |

STR*n* entries each name an instruction address. The most frequent sites are
in oldmain, ProcessSearch, DoAttack, DoHydraAttack, CheckEndGame,
SpecialAttack and KeepAttack.

Grammar hooks (both plugins unless noted):
- `oldmain+93e`: end-sequence congratulations text.
- `combatManager::CombatMessage+af/+153/+217`: battle status-bar damage and kill messages with plural forms, in the selected Format.
- `ViewSpellsHandler+b0/+1c3`: spell descriptions.
- `MageGuildHandler+e3`.

EN only:
- `NormalDialog+a70`.
- `army::SpecialAttack+a76/+ac4`, `army::DoAttack+926/+968` and `army::DoHydraAttack+30c/+34e`: "%d creatures perish" plural forms.
- `game::DoNewTurn+425/+539`: week/month announcements.
- WriteByte 0x74 at `inputManager::AsciiConvert+273` (0x46f32c): the Cyrillic keyboard translation (`TranslateInputCharacterCp1251`) runs only when a modifier is held, so plain keys type Latin.

RU only:
- `font::DrawString+61`, `font::DrawBoundedString+115` and a HiHook (fastcall) on the CP1251 glyph-mapping routine at 0x471ef2. These handle extra glyphs (Ё/ё).
- `UpdateHeroScreenStatusBar+413`, `swapManager::Open+10f`, `swapManager::SplitMons+f7`, `hero::HeroView+c0` and `combatManager::ViewGeneral+17a`: text formatting.
- WriteByte at `game::ShowCampaignInfo+50/+52`, which moves the campaign-info window from (0x69,0x60) to (0x3d,0x31).

Recommendation: use data/text changes. Put the strings in the localisation
tables (`config/retail/buka-localization.tsv` and the source string tables)
as a per-language text set, and move the grammar logic (plural selection,
status-bar Format) into the respective functions as source.

#### 4.3.6 Yo.dll (RU only, `HD.Plugin.H1TE.Yo`)
- LoHooks at `textEntryWidget::Main+560/+56f` (0x476161/0x476170) handle the Ё/ё key (scan 0x29, the backtick key, is remapped to 0x35 around `AsciiConvert`).
- WriteDword at `fileRequester::Main+4fb` (0x454d88) repoints the allowed-filename-punctuation string `$%'-_@~`!(){}^#&+,;=[].` to a copy that adds `«»№`.

Recommendation: a text-input source edit in `textEntryWidget::Main` and in the `fileRequester` character set (RU build option).

### 4.4 Disabled plugins (`*.dl_`, shipped but not loaded)

| File | Hooks | Effect |
|---|---|---|
| `fixBalance.dl_` (`HD.Plugin.H1TE.fixBalance`, VC2019) | WriteByte 0xF5 at `game::RandomizePlayerCrests+67/+80/+a3`, so crests are read from `gCampaignScenarios[k]+0x15` instead of +0x17 (shifted one player). Data writes into `gCampaignScenarios` (0x491ce0, 9 × 0x55): `playerTypes[1]=4`, the human `resources[0][0..6]=0`, and for scenarios 0–3 and 8 also `kingOfTheHill=1` and `playerTypes[2..3]=4`. | A harder campaign: opponents at difficulty 4, no starting resources, all-against-the-player, and the crest-to-player mapping shifted to opponents. The exact crest semantics for scenarios 4–7 (human crest read from `playerTypes[2..3]`) are unresolved. |
| `ReturnSMKVideos.dl_` | WriteHexPatch `E8 14 F8 FF FF` at `PlaySmacker+62` (0x458d56) | Restores the retail `call SmackMain`, which the exe patch NOPs (TE disables all Smacker videos by default). |
| `ScheduledMessages.dl_` + `ScheduledMessages.ini` | LoHooks at `game::NextPlayer+517` (0x4334a3) and `oldmain+88e` (0x43d952) | Shows ini-defined messages at the start of a human player's day. The ini has sections keyed by map name (`[Claw ( Easy )]`, `[Dragon Rider]`) or by campaign (`[~CAMP-X-n]`, `[~CAMP-c-s]`), with `day=text` keys and `\n` escapes. The shipped ini is a sample in English for both languages. |

### 4.5 EN vs RU (runtime files)

| Item | EN | RU |
|---|---|---|
| Loader | `Loader_EN.dll` loads `.\Plugins\EN` | `Loader_RU.dll` loads `.\Plugins\RU`. Otherwise identical code; separate builds. |
| TE.dll | 109 patch calls | Same set plus one extra LoHook at `oldmain+954` (0x43da18) that re-implements the `gEndSequence==2` path (`PlaySmacker(4)`, `PlaySmacker(5)`, resume at +97e). It is behaviourally identical to the original bytes; purpose unresolved. The ini path is `Plugins\RU\TE.ini` and the Paladin texts stay English. |
| TE.ini | English `[Texts]` | Russian `[Texts]`. Options are identical. |
| Text plugin | EnglishTexts.dll/.ini (English translation of the Russian executable, Latin keyboard) | MinerTexts.dll/.ini (revised Russian texts, glyph hooks) + Yo.dll |
| config.ini | `Language=1033` | `Language=1049` |
| Shared, byte-identical | WING32, WINMM, MSS32, patcher_x86(+ini), cdda.mod, hi_combat.mod, fsfix, ReturnBWCursors, CPUPatchAdv, all `.dl_`, ScheduledMessages.ini, Help\winhlp32.exe, Games\PaladinStronghold.GM1, NETLODR.DAT, ORIGDATA.BIN, REMOTE.GAM | — |
| `Data\xx\HEROES.AGG` | 876 entries. Adds `RCRTHER1.BIN` and changes 82 entries vs retail Buka, mostly English art: buttons, panels, screens, fonts, plus the common set below. | 879 entries. Adds `RCRTHER1.BIN` and three stray backups (`CMBTWIN.BIN.BAK`, `REQEXTRA.BIN.BAK`, `SCENINFO.BIN.BAK`, names truncated to 14 chars), and changes 41 entries. |
| Common AGG changes (identical in EN and RU) | `BOAT.XTL`, `DESERT/DGRASS/DIRT/GRASS/LAVA/SNOW/SWAMP.XTL` (combat terrain), `CASTLE00–03.ICN`, `TENT.ICN`, `FONT.ICN`, `SMALFONT.ICN`, `RCRTHER1.BIN` (658 B; the exe still names `rcrthero.bin`, so the user is unresolved) | — |
| `Data\xx\STANDARD.HS`/`CAMPAIGN.HS` | English default high-score names (NWC originals) | Retail Buka files |
| `Help\xx\HEROES.HLP/.CNT` | Original English help (1996) | Retail Buka Russian help |
| `Anim\xx\INTRO.SMK` | English intro (2,648,124 B, 1996) | Retail Buka intro. The other five SMK files are identical to retail. |


## 5. EN vs RU

**Executables.** `H1TE_EN.exe` and `H1TE_RU.exe` differ in three places only:
the loader name in the entry stub (`0x416c19`, `EN`/`RU`) and `gDataPath`/
`gAnimPath` (`DATA\xx\`, `ANIM\xx\`). Every code patch, every `.data` text fix
(all in the Russian string set of the Buka executable) and the whole `.rsrc`
(English menus, English about box, cursors) are identical. The English
edition is therefore the Russian executable with its strings replaced at run
time by `EnglishTexts.dll`; the Russian edition uses `MinerTexts.dll` (revised
Russian texts) and `Yo.dll`.

**Runtime files.** See 4.5: the loaders differ only in the plugin path,
`config.ini` only in the wrapper `Language`, `TE.ini` only in `[Texts]`; RU
`TE.dll` has one extra behaviour-neutral hook (TE-RU-1); the Paladin
stronghold texts are English in both; `HEROES.AGG` changes 82 entries (EN,
English art) vs 41 (RU) with 16 shared changes (combat terrain, castle/tent
icons, fonts) and an added `RCRTHER1.BIN`; EN ships the original English help,
intro video and high-score tables.

*source-te:* one code base; the language split belongs to the localisation
layer (string tables, resource scripts, data paths) and to two optional RU
input features (Ё/ё input, `«»№` in file names). AGG art, help and video are
media packs outside the tree.

## 6. Recommendations for `source-te`

**Structure.**
- One TE configuration (build flag) that switches the default behaviour, plus
  a runtime options block mirroring `TE.ini` (`ShowEnemyMobility`,
  `SoftRetreatSurrender`, `SlightlyHarderAI`, `CheatMode`,
  `OriginalCheatKeys`, `LosslessAudio`) and the text plugins'
  `BATTLE_STATUS_BAR_MESSAGE Format`.
- Express every hook and cave as ordinary edits in the function named in
  `changes.tsv`; new logic becomes named helpers (`EstimateDamage`,
  `CellRandomArtifactId`, an obelisk piece counter, a hero "fled" state
  accessor, surrender cost, k/m number formatting, quicksave).
- Implement the *effective* behaviour: skip the executable's
  `hero+0x12` mobility flag (superseded by `TE.dll`), keep retail cursor
  synthesis and the full-screen preference read (restored by plugins), use
  `Sleep(1)` with a 1 ms timer period.
- Texts go to the localisation tables per language; menus/about box to
  per-language resource scripts with one version constant (the edition calls
  itself both "TE 1.04" and "Tournament Edition 1.05 f3").

**Out of scope** (document only): `patcher_x86`, `Loader_xx.dll` and the
entry stub (a source build compiles the changes in), the `WING32.dll`
renderer wrapper and its `config.ini` sections and registry redirection, the
`WINMM.dll` proxy, the Miles `MSS32.DLL` upgrade, `cdda.mod`,
`hi_combat.mod` (possible later combat-grid option), `winhlp32.exe`, the
single-core `start /AFFINITY 1` launcher, AGG art, help and video media.
Disabled plugins (`fixBalance`, `ReturnSMKVideos`, `ScheduledMessages`)
become off-by-default options at most.

**Save-game compatibility.**
- TE keeps the record layouts but changes the meaning of spare bytes that are
  saved: `hero::m_primaryStats[4]` (`+0x34`, fled state), `hero::m_cowardice`
  (`+0x37`, now live), `hero::m_name[16]` (`+0x12`, only in the static patch),
  `m_availableHeroes` value `0x40` used as "reserved for a tavern".
  Retail saves load into TE; TE saves read by retail carry nonzero
  cowardice and possibly unterminated 16-character names. Give these fields
  names and add a save-format marker on `source-te`.
- Random artifacts become a function of the save (X22): reloading no longer
  rerolls; existing saves get different artifacts than retail would give.
- Cheat state is not saved.

**Multiplayer.** Nearly every gameplay row (combat rules, AI, map logic,
tavern draws, artifact choice, retreat/surrender) changes simulation results;
TE and retail peers desync. TE keeps some RNG call sequences unchanged
(`RandomizeHeroPool`, `GiveRandomArtifact`, `DamageGroup` draw counts), but
outcomes still differ. Bump the network protocol version on `source-te`.
Quickload is already refused in network games.

**Verify before porting.**
- X26 leaves `gHeroWindShowing` set after the hero screen; likely accidental.
- TE-FIX-8 (View World mine letters) reads `hero+0x23`
  (`m_locationType`) where `+0x24` (`m_occupiedTown`) looks intended.
- TE-FIX-7 stores a dword into a 16-bit field and overwrites its neighbour.
- TE-UNR-1 (`DetermineEffectOfSpell` replacement) and TE-UI-1
  (`ViewSpellsHandler` byte patch) are not fully decoded.
- X22 can grant an artifact the player already owns and drops the gold
  fallback when all artifacts are taken.
- The `fixBalance` crest mapping for campaign scenarios 4–7 is unresolved.
- D03 (`.\TRACKS\` byte) and the users of `RCRTHER1.BIN` are unresolved; both
  look inert.

## 7. source-te decisions

Where the Tournament Edition binary has a bug, an ambiguity or a design that
does not fit source, this branch implements the intended behaviour:

- **Fled state (TE-RS-\*, X24).** The edition kept "retreated/surrendered
  today" in `hero::m_primaryStats[4]`, the byte the Ballista of Quickness
  raises by 3, so the artifact corrupted the state. The state lives in its
  own byte, `hero::m_fledState` (the former `m_unused38`, zero in
  `ORIGDATA.BIN` and in every original save): none, retreated, surrendered.
  `m_cowardice` is a live field (floor `HERO_COWARDICE_MIN` = -3). The
  executable's own `m_name[16]` day flag is not reproduced (the edition's
  runtime disabled it). The reset of a killed or dismissed hero restores all
  five stat bytes.
- **Tavern marker (X30).** `0x40` already meant "sitting in a tavern" in the
  original game, not "retreated"; it is renamed `HERO_AVAILABILITY_IN_TAVERN`.
  Last week's pair is released only if still marked and not redrawn.
- **Surrender (TE-RS-2).** The surrender cost is copied only on the computer's
  path (the edition also overwrote it for a human, which desynchronises
  network peers); the computer's offer has its own text and portrait; it is
  offered only against a hero (the edition crashed without one); refused, the
  computer picks its spell with the existing spell evaluator.
  `gbCombatSurrender` is cleared after every combat.
- **Hero screen (X26).** The edition's early return left `gHeroWindShowing`
  set and skipped the movement refresh only to free bytes; both are kept.
- **View World (TE-FIX-8).** The mine letter comes from
  `m_mines[hero.m_occupiedTown]` (+0x24); the edition read `m_locationType`
  (+0x23), an out-of-range index.
- **Recruit maximum (TE-FIX-7).** The count is clamped to the available
  creatures before it is stored in the 16-bit field; the edition's dword store
  overwrote the next field.
- **Random artifacts (X22).** The per-cell choice is kept, but only artifacts
  not yet in play are drawn and the 1000-gold fallback stays.
- **Town footprint (TE-MAP-1).** Event objects inside a footprint keep their
  metadata.
- **Quick load (TE-QOL-1).** F9 reloads through the main loop, like the load
  command; the edition's guard (`gFreshSave`) is replaced by a network-game
  guard, and the file is checked where it is saved.
- **Cheats (TE-CH-2).** Level codes add to the level computed from experience
  (the edition trusted a possibly stale `m_level`).
- **Damage forecast (X16).** The estimate uses the real damage formula
  (shared helpers `army::ScaleDamage` and `army::ShotCrossesCastleWall`); the
  status bar shows target hit points, damage and kills in words.
- **DetermineEffectOfSpell (TE-UNR-1).** Decoded: the computer no longer casts
  Bless or Curse on stacks whose damage range is one value (Peasants).
- **Final campaign battle (TE-SCR-2).** Scenario 8, cell (41,37) is the last
  campaign's Dragon City, not the Paladin stronghold; the test requires a
  campaign and uses the attacking hero.
- **Fonts and keyboard (PL-TXT-4, PL-YO-1, PL-TXT-3).** The new glyphs are
  drawn only by fonts that have them (`catalog.py` lists them in the Russian
  glyph set); the keyboard mapping and the registry key live in the locale
  descriptors (`locales/<lang>.json`) and all new text in the catalogs under
  ids of the game's scheme, so the source has one code path for every
  language.
- **Save format.** The reserved header block of a save starts with
  `SaveFormatTag` {"H1TE", version}. Version 1 (`SAVE_FORMAT_FLED_STATE`)
  adds the heroes' fled state, live cowardice and reserved tavern heroes;
  version 2 (`SAVE_FORMAT_TOWN_FLAGS`) stores a "built today" bit for each of
  the 36 towns (BUG-TWN-3) in five bytes instead of four. Older versions
  load: version 0 (original game) has its tavern heroes reserved, and
  versions 0 and 1 read the four flag bytes and restore the first hero's id,
  which towns 32-35 flagged.
- **Multiplayer.** `REMOTE_PROTOCOL_VERSION` 1 changes the serial handshake tag
  (`TE`), the NetBIOS group (`Empire TE1 `) and the packet checksum seed, so
  the edition never pairs with the original game.

Catalogue corrections found while implementing: X14's metadata byte is
unsigned; X23 is not an early-out; X28's retail `GetCell` already clamps; X35's
second 0.99 user is `philAI::DamageGroup`; TE-BAL-1 affects only customized
random castles; TE-CH-2's 101499 grants the spell book and spells, 101500
excludes the castle code; the F9 guard is `gFreshSave`; MinerTexts has no
general plural rule and its glyph hooks add « » — № (Ё/ё were already
supported).

## 8. The editor

The edition shipped no editor; `EDITOR.EXE` is built from this tree
(`build.py --target editor`) and shares `kbwin`, `REQUEST`, `Audio` and the
BASE library with the game. It shares their behaviour as one release would:
the per-language registry key and the windowed default, the message pump
with its idle sleep and 1 ms timer period, the CD music from the game folder,
the Russian file-name punctuation, the keyboard mapping and the revised
texts, and the original assertions. Only the `HOMM1_EDITOR` checks of the
original source separate the two programs.

The edition does not change the map format. A megalith
(`MAP_OBJECT_MEGALITH`) placed in the editor becomes a Paladin stronghold in
the game; its cell metadata starts at 0 (guarded) as the editor writes it,
and only the game sets 1 (abandoned) and 2 (ghosts waiting). The save-format
tag, the fled-state byte, reserved tavern heroes, per-cell random artifacts
and the town footprint metadata exist only in saved games and at game start,
which the editor never reads.

## 9. Retail gameplay bugs fixed on this branch

Besides the edition's own fixes, this branch fixes the retail gameplay bugs
that the native port documents and keeps (`docs/port/divergences.md` on
`port`, "Retail gameplay bugs (kept)"); plain `port` stays faithful to the
original game. They are not part of the edition's patch, so they are listed
in `changes.tsv` as `BUG-*` rows (component `source`), outside the counts of
section 1, each with the reproduction it was checked against.

- *Computer player (BUG-AI-1–8):* the replaced stack's value, the affordable
  creature count, sites visited by other players, stale route nodes, hero
  distances, artifacts taken without a free slot, the first player's
  ultimate artifact hint, and the obelisks' value without an artifact or
  obelisks.
- *Combat (BUG-CMB-1–3):* berserk stacks that loop or act without an
  action, commander skills that wrap the stack's attack and defense, and
  the computer's adjacent attack test off the battlefield.
- *Adventure map (BUG-ADV-1–9):* the skeleton's unpaid gold, Summon Boat at
  the map's edge, the campfire's sound, the puzzle's off-map cells, heroes
  and towns without records under other objects, Town Gate without a town,
  the path search and mine flags beyond the map's edges, and the ultimate
  artifact's placement for a human player without a hero.
- *Campaign (BUG-CAM-1):* the enemy lords' crests, read one entry late.
- *Towns (BUG-TWN-1–3):* the Thieves' Guild's resource groups, the owner
  the recruit window gave both tavern heroes, and the "built today" flags of
  towns 32-35, which lived in the first hero's id. The port's "weekly monster
  growth" row is not a defect: `game::PerWeek` grows a site's stock only
  below 100 and by at most 10, so the byte never exceeds 109.
- *Random map generator (BUG-GEN-1–5):* region ranking, region bounds,
  diagonal chain lengths, roads between castles, and desert stone liths.
- *Editor (BUG-EDT-1–4):* the mine records' empty test, the vertical scroll
  knob, ground painting's redraw, and the extra records of erased towns and
  heroes.

The computer's auto-resolved defeat as attacker (the winner's effects on the
attacker, the defender's own experience) is the edition's TE-FIX-4, and bad
luck at luck -1 its X11 (`SRandom(1, 12) <= -luck`).

Kept as designed:

- *Morale chances.* Good morale gives an extra action with chance
  morale/24 (`SRandom(1, 24)`), bad morale loses one with chance -morale/12
  (`SRandom(1, 12)`, `combatManager`): the asymmetry stays.
