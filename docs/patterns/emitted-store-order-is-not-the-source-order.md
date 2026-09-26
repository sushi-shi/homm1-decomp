> Imported from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
> This is donor evidence, not a validated HoMM1 VC4 rule. Commands and source
> examples describe that donor. Re-prove applicable mechanisms with VC4.

# Emitted stores need not follow source order

The optimizer can move independent loads and stores. Transcribing a retail
store sequence into C++ and recompiling need not reproduce that sequence.

The [recorded ActionOptionsMenuBar::Init A/B](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/emitted-store-order-is-not-the-source-order.md)
compares two six-member assignment orders. Assigning in member declaration
order reproduced a different, retail store order.

Use data flow to identify each stored value, especially stores interleaved with
the next call's argument setup. Check helper expansion and real aggregate-copy
boundaries before permuting individual stores. Compare from the first
divergence, since changed uses can alter earlier allocation.

Declaration order is a source hypothesis, not a compiler fixed-point rule.
Nor does one sequence emitted unchanged prove that it was the original source.
Aliasing, volatile access, calls, construction order, and dependencies constrain
which reorderings are semantically valid. Matching counts or store offsets alone
do not prove equal values, source identity, or correctness.
