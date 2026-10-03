> Donor evidence from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
> Named closures below are historical examples from other projects. VC5/VC6
> claims are not validated VC4 rules. Use HoMM2 correspondence for HoMM1;
> strict scoring checks data-reference names and addends.

# Exact-match lever catalog

A reasoning aid, not a plan template or a menu of blind mutations: use an entry
only when retail/source evidence licenses it, and mark the family checked when
it does not apply. Derived from checked-in exact closures, hard-wall history,
and the HoMM3/LithTech source-shape campaigns. Compiler mechanisms live in
`docs/patterns/INDEX.md`.

## 1. Recover source before steering the compiler

- Adopt a complete surviving LithTech/game/sample owner as a composition:
  declaration, hierarchy, nested types, helpers/macros, locals, statement
  order, loops, and storage. A lower first score is not rejection. Exact
  precedents include Rez, Bute, Blowfish, Crypt, DIB/imagepool, dprintf, RegMgr,
  and zPTree families.
- Search repeated sibling implementations and retail sibling binaries. Use
  them positively for family, topology, widths, names, and abstraction—not as
  negative proof when revisions differ.
- Search paired Debug/Release objects for CodeView locals, scopes, types, and
  source paths. Compile recovered facts in the real HoMM1 VC4 TU.
- For lost headroom, recover the old exact/high source hash before proposing a
  new one. Historical exact recoveries found spurious out-param initializers,
  a deleted frame local, and deliberate structural trades.
- Recheck function/TU ownership, class membership, calling convention, and
  static receiver type. Exact callee bytes can hide a dropped `this`; a dead
  ECX load before a call proved missing members. Wrong virtual-slot
  displacement proved a wrong static receiver type.

## 2. Types, layout, aggregates, and semantic identity

- Propagate parameter/local width through the whole call family: `u8` versus
  `bool`/`char`, signed versus unsigned, 16-bit destination context, enum
  domain, dirty upper bytes, pointer versus integer, and real SDK types.
- Test cv/ref and overload boundaries: const forwarding wrappers, `T&`
  accessors, const-reference assignment/results, and by-value adapters may
  disappear while changing caller temporaries and evaluation order.
- Restore real aggregates only from complete-object evidence: `RECT`/`CRect`,
  `Coord`/`POINT`, ranges, palettes, wire records, and paired clock fields.
  Exact closures came from by-value RECT parameters, per-call by-value return
  slots, whole-aggregate clears/copies, and typed PALETTEENTRY byte reads.
- Treat repeated same-width fields as candidates, not proof. Prefer a surviving
  typed union/class family to a layout-compatible `void*` or overlay.
- Audit base/subobject/member identity. Same masked bytes with a different
  displacement, vptr target, or relocation name are correctness defects.
- Check forward-declaration `class` versus `struct`; it changes the mangled
  referent even when code bytes are otherwise identical.

## 3. Inline/helper/macro boundaries — mandatory search

For every function, search headers, sibling sites, surviving source, COMDATs,
and repeated expressions for an authored boundary. Do not assume emitted
arithmetic was handwritten.

- Restore tiny inline accessors, setters, getters, array subscripts, range
  guards, conversions, constructors, assignment operators, and member
  predicates. Exact closures came from `operator[]`, `SetFrame`/`SetEnabled`,
  range-guarded `GetAt`, point-containment, min/max/clamp-style helpers, and
  value-returning coordinate accessors.
- Preserve the helper's receiver form and complete signature. A member versus
  free helper changes operand order; object/point versus scalar arguments
  changes load timing; u8-by-value pixel-pack helpers preserve narrow values.
- Preserve the helper body's authored assignment/store order. The expansion
  can recolor caller locals before the first visible helper instruction.
- An inline helper can own an out-param local/reset. Retail's zero store between
  argument setup and call, or a different `lea` home, repeatedly proved the
  local belongs inside the helper. The WellGoo lookup-helper restoration moved
  three sites together and enabled exact closure.
- Repeat a pure inline expression when source identity matters. Replacing it by
  a cache can enable CSE and change register lifetimes; conversely remove an
  unjustified one-use result/local when retail keeps the cursor or receiver.
- Restore authentic macro abstraction for standard algorithms and pixel/math
  families. Exact examples include Blowfish rounds, DIB conversion, pixel
  packing, WWD rectangle helpers, and palette math.
- Distinguish visibility from eligibility. Under `/Ob1`, unmarked `static`
  helpers do not expand; tiny `inline` helpers may expand everywhere; a body
  visible in one TU/site can coexist with calls elsewhere. Inspect actual call
  referents and `llvm-nm` COMDAT evidence.
- Treat per-caller inline budget as a population problem only after candidate
  helpers are independently proved. Missing repeated one-field helpers changed
  constructor call/expansion populations and EH states. Re-prove inline behavior with VC4 controls; never retain cost padding or forcing devices.
- A pure register-colour row may still be missing one inline IL tuple. This
  closed `MidiManager::GetMasterVolume` after in-body and TU probes were flat.

## 4. Local census, lifetime, scope, and creation order

- Recover the exact locals instead of caching every member. Test distinct
  old/new/result variables, parameter reuse, one reused result through setters,
  and removal of one-use call-result locals.
- Check declaration and first-use order independently. Creation order can
  determine register preference, recycled spill-slot reload order, and which
  call-crossing value receives EBX.
- Check initialization placement: declaration initializer versus later store,
  initialization in a `for` header, deliberate uninitialized locals, and a
  late store after the value's first consumer.
- Scope is emitted structure. Sibling block locals may share one escaped home;
  function/case-scope locals cannot. Distinct per-arm address-taken outputs may
  need distinct homes. A second-half local at function scope can deliberately
  prevent frame sharing.
- Compare frame size and every address-taken `lea`. A frame difference may be a
  real local, an unnamed temporary, a dead parameter-home reuse, or a spill;
  classify which before editing.
- Test pointer/base locals where retail folds a subobject offset into one `lea`
  or reuses a receiver across stores. Remove aliases where retail advances the
  parameter itself or reloads instead of caching.
- By-value accessors can require **unnamed** temporaries. Naming the Coord may
  let VC5 delete its dead stores; repeated `obj->LastTilePx().field` calls
  restored otherwise unreachable 8-byte frame homes and several exacts.
- Preserve named result references for stream/operator chains and other
  sequencing boundaries. `ButeGroup_Apply` reached exact through a named
  `ostream&` result and statement sequencing.
- Preserve payload locals around varargs or calls when retail homes surrounding
  loop state; remove copy locals when they alone cause a spill.

## 5. Statement and expression shape

- Test one expression versus sequenced assignments, and direct member stores
  versus a reused result passed through setters. VC5's front end retains
  statement boundaries after the machine body appears folded.
- Put computations in the statement where retail emits them. Naming an outer
  call argument, moving `+K` into an initializer, or grouping constructor
  arguments changed scheduling and closed exact functions.
- Preserve evaluation and operand order where it is observable; do not infer
  source order from commutative expressions that VC5 canonicalizes.
- Test ternary versus split `if`, short-circuit expression versus materialized
  bool local, in-place compound update versus local-copy/writeback, and clamp
  ternary versus init-then-if.
- Reproduce stores in semantic/member order in **both** symmetric arms. Equal
  operations in different source statement groups can decide cross-jumping.
- Keep mathematically authentic idioms: `abs()` rather than hand-expanded sign
  masks, cumulative numerators rather than recomputed products, explicit
  accumulation rather than one reassociable OR tree, and correct parentheses
  around division/interpolation.
- Use the right era library abstraction: `memcpy`, `memset`, `strlen`, `strcmp`,
  MFC conversion/accessor, Win32 helper, or authored wrapper. Their inlined
  forms create masks, REP setup, dead-looking scans, and temporary homes.

## 6. Guards, exits, and block placement

- Count calls, conditional branches, unconditional jumps, and returns before
  calling a wall regalloc. Match the **destinations** and block placement, not
  merely the multiset.
- Test separate early returns, `goto fail`, total `||`/`&&` collapse, and a
  positive success region. They are distinct VC5 tail-merging regimes.
  WellGoo closed only after the final successful region was nested under
  `if (m_fgFrame != NULL)`, leaving the shared failure epilogue last.
- A state/result carrier plus one shared exit is distinct from duplicated
  returns. Conversely, a fabricated `default: return` or `else` can pin the
  prologue and add an exit retail lacks.
- Preserve staged failure ownership around allocation/initialization/insertion.
  `CNetMgr::AddPlayer` became exact when early init failure partitioned later
  delete tails naturally.
- Check guard polarity, compare operand order, and positive versus negative
  nesting. Cross-jumped call arms expose source arm order in their differing
  pushed constants.
- A scan may null its cursor rather than `break`; a retry may reuse one variable
  and one return; branch-local null checks may share only the success tail.
- Constructor/destructor/EH arms are special: unwind targets, called base ctor,
  cleanup owner, and EH-state count can prove the authored layer even when the
  normal body looks equivalent.

## 7. Loops and cursor ownership

- Test `while`, guarded `do/while`, `for`, infinite loop plus `break`, and
  duplicated `continue` paths from retail's entry/latch shape.
- Test post-decrement/count-down guards. `while (n-- > 0)` materializes `n-1`;
  a shared counter declared above sibling arms can preserve that form in both.
- Reproduce counter and pointer update order, including comma increments.
  `CGrunt::LoadStateRecord` reached exact only after counter-before-pointer in
  both nested loops.
- Decide whether the parameter itself is the cursor, an interior pointer local
  is carried, an array cursor owns the member bias, or each access is indexed.
- Treat an entry trampoline as a readout of value ownership/pressure, not a
  request to respell the loop blindly. Check hoisted invariants, one-use result
  locals, and cross-arm aliases first.
- Check loop-invariant caches versus repeated inline conversions, `CString` to
  `LPCTSTR`, zero-store spelling, and fixed clear loops that share a later REP
  zero.

## 8. Allocation, destruction, and storage operations

- Prefer source `new`, `new[]`, `delete`, and `delete[]` when object ownership
  supports them; explicit allocator/destructor calls require evidence. Their
  cleanup/EH and scratch-register choices differ even when final frees match.
- Audit constructor chain depth and per-site inline cuts using EH states and
  call referents. Do not infer source form from MSVC 5's allocator symbol alone.
- Test guarded delete plus null store, direct member out-param versus local plus
  copy, and `memset`/fixed loop/manual stores based on target setup and sharing.
- Preserve complete typed teardown adapters and heterogeneous container seams
  when surviving source proves them; do not erase them merely to obtain a clean
  `delete` spelling.

## 9. Data, constants, referents, and TU state

- Audit ordered relocations, not just masked instructions. Wrong callee, pooled
  literal, member/global identity, FP slot, or DIR32 addend can be the entire
  residue.
- Use bare string/FP literals when automatic oracles cover them. `/Gf` pooled
  literals invalidate duplicate file-static char arrays.
- Check local static versus TU global versus header COMMON ownership, and
  source/TU order for first-function EH or dynamic-init effects.
- A masked diff can hide wrong branch destinations; inspect raw displacement
  targets and normalized CFG.
- Only after all structural families are disposed may a classified
  register/schedule wall use deterministic source variants or disposable
  declaration/TU-state probes. Retain the diagnostic evidence, remove the probes, and verify authored
  source before banking; disposable states cannot bank MAX in HoMM1.

## 10. Proven stop signals and negative controls

- Pure destination-register colour with identical IL and no source entity may
  be cursor phase. Do not manufacture a local solely to request a register.
- Commutative integer term order, local names under `/O2`, some declaration
  order changes, and some cursor spellings are proven flat in their specific
  signatures. Treat them as controls, not universal laws.
- Inline `strcmp` can choose a two-instruction-longer byte-register form solely
  from register availability. Fix the pressure source elsewhere.
- REP counts and branch totals can be downstream of tail merging or allocation;
  confirm the earlier divergence before altering object counts or CFG.
- A current dip below a banked MAX is not a reason to distort correct source.
  A source-hash change that lost historical exactness is, however, a mandatory
  history investigation.

An exact closure does not require a new pattern or catalog entry. Consolidate
only distinct reusable mechanisms under `docs/patterns/README.md`; keep
function-specific experiment history in the handoff and Git history.
