> Imported from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
> This is donor evidence, not a validated HoMM1 VC4 rule. Commands and source
> examples describe that donor. Re-prove applicable mechanisms with VC4.

# An inline can have expanded and out-of-line uses

A caller may contain both an expanded helper body and calls to its out-of-line
copy. That is compatible with an ordinary inline definition; it does not by
itself require separate APIs or manually expanded source.

The [recorded VC5 probes](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/inline-budget-emits-ool-comdat.md)
vary caller content and repeated eligible call sites, observing different
expansion counts. Caller context and nested expansion matter. The repository's
`inline model` (historical donor reference) is a conditional
predictor derived from a sibling compiler and calibrated on selected VC5 probes,
not a proof of every VC5 inlining decision.

Check resolved call targets and ordered sites, including tail jumps. Confirm
eligibility separately; [template members](vc5-template-members-inline-without-inline-keyword.md)
are an exception to a keyword-only /Ob1 rule. An /Ob0 comparison helps expose
boundaries, but optimization can still merge or remove sites.

A locally defined COMDAT supports body availability. An undefined or absent
symbol does **not** prove the body was unavailable: another site may expand,
a nested site may remain external, or delinking may obscure the provider.

An all-expanded harness is saturated, not proof of a zero-cost helper.
Calibrate a partially rejecting case before inferring a budget; do not equate
machine-code bytes with the compiler's internal size estimate. A missing call
can also be tail merging or dead-code elimination.

Member construction spends the same budget. Giving a member type a user-declared
constructor, even an empty inline `T() {}`, adds a construction site for every
such member of the enclosing class. Observed: with ctors on `Coord` and
`DoubleVector3`, `CMotionState::InitBounds` stopped expanding into the
`CProjectile` constructor, and `walls diagnose` reported an inline/call-set gap.
When an exact constructor degrades that way right after a member type gains a
constructor, suspect that retail's type is an aggregate. That is a clue for the
type model, not proof that no constructor exists anywhere.
