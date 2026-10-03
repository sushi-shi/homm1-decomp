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
- [/O2 register allocation (HoMM1 VC4, measured)](vc4-global-register-allocation-is-chaitin-briggs.md) — Chaitin-Briggs colouring; ties follow declaration order (C1 handle & 31); reference counts weigh 5^loop depth.
- [Control flow consumes handles (HoMM1 VC4, measured)](vc4-control-flow-consumes-c1-handles.md) — labels, `&&`/nested `if`, loops, switches and casts take C1 handles, so a code-identical rewrite shifts every later function's handle state.
- [Data emission order (HoMM1 VC4, measured)](vc4-data-emission-order.md) — `.data` variables in definition order ahead of literals; `.bss` by name hash % 1024; dynamic initializers at the definition point.
- [Signed remainder](signed-modulo-pow2-abs-restore.md) — sign correction around a power-of-two mask.
- [VC4 float expression shape](vc4-float-expression-shape.md) — store/compare order follows declarations; parentheses stop product reassociation; C-style float casts add a temporary (HoMM1-measured).
- [Conditional with an enumerator arm (HoMM1 VC4, measured)](vc4-conditional-enum-narrowing.md) — `c ? <char field> : ENUM` takes the char type (byte temporary + `movsx`); an `int` literal arm keeps it `int`. A function that no TU-state trial moves after an enum edit is the tell.

