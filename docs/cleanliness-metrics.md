# Cleanliness checks

`homm1 verify check` runs the MAX score gate and fast/normal source/model checks.
The registry in `scripts/homm1/verify/tiers.py` is authoritative. Committed debt
floors live under `config/cleanliness`; generated findings belong in `build`.

Direct gate commands expose their evidence. Updating a debt floor is an explicit
reviewed operation, not a routine response to a failing build. Existing source
gate failures remain visible during the tooling port. Code-first relaxation
changes data-reference scoring; it does not license false code, wrong callees,
contradictory identities or silent gate exceptions.

Run `homm1 build` for tooling changes. The imported `verify selftest` is a separate,
currently incomplete whole-donor validation suite; see the inheritance review.

Counters identify candidates, not source truth. Preserve authentic SDK/ABI types,
source-backed ownership and necessary reviewed casts. Do not replace a correct
type with a layout view, hide a cast in a helper, or invent names to lower a
count. Equal numeric values alone do not establish one enum domain
(`homm1 verify enum-reuse` compares declarations with the review ledger).
Every cleanup edit must leave the edited function exact in `homm1 match`.

## Measuring

Run inside `nix develop .#build`:

```sh
homm1 verify board               # text rows, delta vs config/cleanliness floors
homm1 verify board --semantic    # adds the build-derived rows
homm1 verify casts               # reinterpret_cast ledger + self-recursion
homm1 verify casts --nested      # libclang nested static_cast scan
homm1 verify dead-code           # @dead-code markers vs retail reachability
homm1 verify enum-domains        # range tests and enum-domain defects
homm1 verify enum-reuse          # enum members vs the reuse review ledger
homm1 verify constants           # bare numeric spellings work list
homm1 verify label-style | include-order | unique-names | source-encoding | review-claims | bans
```

The committed board floors (`cleanliness-{text,semantic}-baseline.tsv`) predate
the HoMM1 source and are still zero for most rows, so `board --gate` reports
every non-zero row as a ratchet violation. Re-blessing them with `--update` is
a reviewed, manual act and has not been done.

## Reconstruction debt

Manually maintained checklist in the kf1 style. Counts were measured on
2026-10-02 with the commands above; bracketed "was" values are from `32e35b4`,
before the first cleanup pass. Cast, union, goto and name counts cover the
whole tree; the other items cover audited cases.

- [x] Review reinterpret_casts: **168 casts, 0 unexplained** (was 136 / 82):
  68 Win32 ABI types (`LPBYTE`, `LPBITMAPINFO`, `LPLOGPALETTE`, `WNDPROC`, ...)
  and 100 reasoned seams (remote-queue `char*` records, NetBIOS `PUCHAR`
  buffers, Smacker palettes, resource `ReadBlock` signed bytes). Nested
  `static_cast` chains: **0** (was 4 `void*` launderings in `wingraph`).
- [ ] Review written C-style casts. Board row: **1** (was 198); 480
  `static_cast` spellings. The remaining board cast is required by retail: VC4
  narrows `philAI::QuickCombat`'s double through a stack temporary only for
  the C-style form ([float expression shape](patterns/vc4-float-expression-shape.md)).
  A wider census (any `(T*)expr`) still finds **35** pointer casts: 30
  `combatRemoteMessage`/`heroRemoteMessage` views over `char*` packets in
  `EVENTS`, 4 save-transfer buffer writes in `game`, and `NULL_SAMPLE2`.
- [ ] Review unions: **14 definitions**: 7 DirectDraw SDK, 3 per-message word
  views in the flat `tag_message`, 2 remote payload variants, 1 serial/NetBIOS
  node payload, 1 `searchNode` per-phase overlay. Each models record variants;
  none is a layout workaround.
- [ ] Review gotos: **209 statements** in 22 files (KB 42, ADVMGR 22, ARMY 21,
  EVENTS 20, GAME 15, CURSOR 14, SETUP 13, FINDPATH 10, others ≤ 9). In exact
  functions they are byte evidence: replacing one of `PollRemote`'s
  `goto done` jumps to its closing label with `return` drops it to 98.97
  (two fewer `jmp`s than retail; all four drop it to 96.86). Per-site review
  remains open.
- [x] Review artificial address arithmetic: **0 cases**; board offset-cast
  macros 0, no `(char*)this + n`.
- [x] Review owner recovery from member pointers: **0 sites** (no `offsetof`
  or containing-record arithmetic).
- [ ] Review out-of-object pointers: **6 sites**: 4 `game` save-transfer
  writes through `(int*)`/`(short*)` packet buffers and the 2 one-byte
  trailing payload arrays in `tag_Node`.
- [x] Review manual varargs: **0 functions**; `nb_sess` reads its arguments
  with `va_arg`, `com_sess` is a zero-reference stub.
- [ ] Review unrelated variable reuse: not measured.
- [ ] Review stack aggregates and unused members: **67** never-used slot locals
  (`junk*`, `unused*`, `spare*`, `notUsed`, `gap*`, `dummy*`, `pad*`) hold
  retail stack slots; whether each is a real unused variable is open.
- [ ] Review buffer bounds: **2** one-element trailing arrays (`tag_Node`).
- [ ] Triage compiler warning families: not measured; HoMM1 has no warning
  harness yet.
- [ ] Review inline helpers: **12** explicit `inline` helpers plus in-class
  accessors (`army::IsAlive`, `playerData::Color`, `game::GetHero`, ...).
- [ ] Review common-code macros: **23** function-like macros (identity/enum
  macros, `REMOTE_PACKET`, `IS_WIDGET_SELECTION_NOTIFICATION`,
  `SET_ADVENTURE_BUTTON_FLAGS`). The 38 `ProcessAssert(cond, file, line + n)`
  sites are an `assert` macro candidate; their per-function line base is data
  and is not yet modelled.

### Names and identity

- [ ] Unknown identifiers: board **71** (was 462). Declared `m_unknown*`
  members **33** (was 54) with **59** spellings (was 444). Most remaining
  members are opaque spans or constructor-only stores; the only remaining
  member read without a proven role is `mouseManager::m_unknown49/4d` (the
  pointer position ComboDraw shifts into map cells).
- [ ] Slot-tuned local names: **28** numeric-suffixed local declarations
  (was 86, not counting `junk*`/`unused*` slot holders). 19 sit in rows
  still below 100 (`ValueOfEventAtPosition`, `DrawCell`, `SpecialAttack`/`DoAttack`);
  the rest are natural names or slot holders (`row1`/`row2`, `t1`-`t3`,
  `extra2`). A rename must keep each
  name's `/Od` identifier-hash bucket (`h = (h << 2) + (h >> 7) + c`,
  bucket `h & 15`); then stack slots and C1 handles do not move.
- [x] Donor bootstrap blocks: **287** `donor PoL RVA ...; preferred Buka symbol`
  blocks remain and each names the function that follows (symbol-checked);
  15 blocks that named a different donor function were corrected or removed.
- [x] Source markers: `@early-stop` **0** (3 stale markers sat on exact
  functions); `@identity-TODO` **0**; `@dead-code` **29** (28 C++, 1 MASM).
  `verify dead-code` reports **1** finding (was 22): `BMAP2.asm`
  `MoveBitmapArea`, whose decorated MASM `PROC` name the gate does not read.

### Other gates

- [ ] `enum-domains` **120** defects, `enum-reuse` **998** coverage findings,
  `constants` **21,320** bare spellings (8,468 nontrivial, 735 proven
  `NULL`/bool replacements); board magic case labels **1,421**, unnamed
  domain compares **606**. Owned by the enum/constant lane.
- [ ] `unique-names` **13** data claims missing from the admitted census
  (data lane).
- [ ] `include-order` **0** files out of order (was 24), 5 manual: `Misc`,
  `KB`, `REQUEST`, `comwin` and `kbwin` define `WIN32_LEAN_AND_MEAN` inside
  the block; canonical order would also move `windows.h` ahead of their
  project headers.
- [ ] Board structural rows: cpp extern decls **3** (was 189), cpp external
  prototypes **0** (was 28), duplicate header externs **0** (was 24),
  `.cpp`-local types **6** (was 9), `.cpp`-local enum **1**, `void*` members
  **4**. Externs and prototypes live in the owner unit's header (Buka's header
  where Buka declares the symbol, else the retail data band's owner; KB-band
  tables in `X_GLOBAL.h`). Seven retail-referent aliases were renamed at their
  uses (`giThisNetPos`, `giSpellAIValue`, `glTimers[0]`, `gArmyNames` twice,
  `NULL_SAMPLE2`, `gbRemoteOn`). The remaining externs: `iMPExtendedType` in
  GAME, which retail reads as `giDebugLevel` but whose user `LoadGame` is not
  exact in the current TU state, so its body is not renamed yet; `comwin`'s
  private `gComPorts`; and `AppAbout`'s `extern "C"` definition, which the
  counter reads as a declaration. The remaining types are unshared (EVENTS
  wire records, `OLDASM` palette, `comwin` port state).
- [x] `label-style` OK, `source-encoding` 0, `bans` OK, `review-claims` 0.
- [ ] `verify casts` self-recursion reports 3 false positives: the `(void)`
  overloads of `WalkTo`, `AttackTo` and `FlyTo` forward to the one-argument
  overload, but the gate counts `void` as an argument.
