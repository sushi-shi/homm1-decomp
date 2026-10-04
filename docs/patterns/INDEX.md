> Imported from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
> This is donor evidence, not a validated HoMM1 VC4 rule. Commands and source
> examples describe that donor. Re-prove applicable mechanisms with VC4.

# Donor compiler-pattern reference

Short observations, not a wall database. [Scope and admission rules](README.md).

## Compiler output

- [Template inline eligibility](vc5-template-members-inline-without-inline-keyword.md) — an unmarked template member can expand under /Ob1.
- [Explicit-only template arguments](vc5-explicit-only-template-arguments-collapse.md) — `F<X>()` with `X` absent from the parameters collapses instantiations within a TU.
- [Mixed inline and out-of-line calls](inline-budget-emits-ool-comdat.md) — inspect each call site; symbol presence is not an expansion census.
- [EH frames and lifetimes](eh-frame-presence-is-a-source-fact.md) — unwind records are evidence, not an object counter.
- [Local-static guards](function-local-static-dynamic-init-guard.md) — recognize dynamic initialization without inventing flag globals.
- [Scopes and stack slots](switch-arm-locals-overlay-only-when-scoped.md) — sibling scopes can change stack reuse.
- [Store scheduling](emitted-store-order-is-not-the-source-order.md) — emitted order need not be source order.
- [Call arguments](call-argument-evaluated-before-pushes-means-a-temporary.md) — an inner call evaluated before the other pushes went through a local.
- [Translation-unit context](tu-state-probe-family-decides-reachability.md) — unchanged function text can emit different code.
- [Operand sort keys (HoMM1 VC4, measured)](vc4-operand-sort-key-is-the-symbol-handle.md) — C2 orders commutative/compare operands by a hash of their C1 symbol handles; local declaration order is the lever.
- [Sortnode replay (HoMM1 VC4, measured)](vc4-sortnode-is-a-replayable-function-of-handles.md) — the C2 operand sort is a pure function of C1 handles; a traced compile replays and predicts every sorted tree, so declaration orders and handle shifts can be planned instead of searched.
- [/O2 register allocation (HoMM1 VC4, measured)](vc4-global-register-allocation-is-chaitin-briggs.md) — Chaitin-Briggs colouring; ties follow declaration order (C1 handle & 31); reference counts weigh 5^loop depth.
- [/O2 register ties (HoMM1 VC4, measured)](vc4-register-tie-order-is-the-range-id.md) — equal-degree ranges colour in C2 range-id order: C1 handle & 31 bucket walk, newer first; C2-created symbols (reversed loop counters) sit in fixed buckets.
- [Control flow consumes handles (HoMM1 VC4, measured)](vc4-control-flow-consumes-c1-handles.md) — labels, `&&`/nested `if`, loops, switches and casts take C1 handles, so a code-identical rewrite shifts every later function's handle state.
- [Incremental compilation /Gi (HoMM1 VC4, measured)](vc4-gi-incremental-compilation.md) — retail's line words are `__LINE__Var`, its per-function literal sections and leading `_$E2` are `/Gi` output; in the pinned C2 the speed flag also 16-aligns `/Gi` functions, which retail SOURCE does not show.
- [/Gi with a listing (HoMM1 VC4, measured)](vc4-gi-listing-packs-functions.md) — `/Gi /Zi` plus `/Fa`/`/Fc`/`/Fl` keeps every C1 `/Gi` effect (`_$E2` first, `__LINE__Var`, handle shift) but one packed `.text`: retail SOURCE's layout. BASE is `/Gi /Gy`.
- [/Gi data placement (HoMM1 VC4, measured)](vc4-gi-static-placement.md) — C2 emits the whole IL `init` stream before any function; under `/Gi` function-local statics move into the function stream (head of their function's data), but `__LINE__Var` stays in `init` (object top), unlike retail's line words.
- [/Gi handles follow path strings (HoMM1 VC4, measured)](vc4-gi-handles-follow-path-lengths.md) — under `/Gi` the source path, the object path and the opened header paths all move C1 handles (by content, not just length); fixedroot gives every compile a fresh `.pdb`/`.idb`; a whole-tree object-directory fit finds no retail spelling above the per-directory default.
- [Data emission order (HoMM1 VC4, measured)](vc4-data-emission-order.md) — `.data` variables in definition order ahead of literals; `.bss` by name hash % 1024; dynamic initializers at the definition point.
- [Signed remainder](signed-modulo-pow2-abs-restore.md) — sign correction around a power-of-two mask.
- [VC4 float expression shape](vc4-float-expression-shape.md) — store/compare order follows declarations; parentheses stop product reassociation; C-style float casts add a temporary (HoMM1-measured).
- [Conditional with an enumerator arm (HoMM1 VC4, measured)](vc4-conditional-enum-narrowing.md) — `c ? <char field> : ENUM` takes the char type (byte temporary + `movsx`); an `int` literal arm keeps it `int`. A function that no TU-state trial moves after an enum edit is the tell.


- [VC4.1 controls for the 1997 build](vc41-win95-1997.md).

- [VC6 local-static data names (HoMM1 Buka, measured)](vc6-static-data-names.md).
- [VC6 array data names (HoMM1 Buka, measured)](vc6-array-data-names.md).
- [VC6 locale startup and COMMON guard (HoMM1 Buka, measured)](vc6-ctype-startup.md).
- [VC6 CPU profile and narrow arguments (HoMM1 Buka, measured)](vc6-short-arguments.md).
- [VC6 inline CP1251 case folding (HoMM1 Buka, measured)](vc6-cp1251-fold.md).
- [VC6 empty strings in BSS (HoMM1 Buka, measured)](vc6-empty-string-bss.md).
- [VC6 word-sized compound assignments (HoMM1 Buka, measured)](vc6-short-compound-assignments.md).
