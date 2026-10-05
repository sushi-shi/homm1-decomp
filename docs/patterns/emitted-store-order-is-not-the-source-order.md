> Unmeasured hypothesis: observed with MSVC 5.0; not measured on this
> target's VC4 or VC6 compilers. Re-prove the mechanism before relying on it.

# Emitted stores need not follow source order

The optimizer can move independent loads and stores. Transcribing a retail
store sequence into C++ and recompiling need not reproduce that sequence.

An MSVC 5.0 A/B of a six-member initializer compared two assignment orders.
Assigning in member declaration order reproduced the retail store order, which
differs from that declaration order.

Use data flow to identify each stored value, especially stores interleaved with
the next call's argument setup. Check helper expansion and real aggregate-copy
boundaries before permuting individual stores. Compare from the first
divergence, since changed uses can alter earlier allocation.

Declaration order is a source hypothesis, not a compiler fixed-point rule.
Nor does one sequence emitted unchanged prove that it was the original source.
Aliasing, volatile access, calls, construction order, and dependencies constrain
which reorderings are semantically valid. Matching counts or store offsets alone
do not prove equal values, source identity, or correctness.
