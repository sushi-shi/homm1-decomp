> Imported from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
> This is donor evidence, not a validated HoMM1 VC4 rule. Commands and source
> examples describe that donor. Re-prove applicable mechanisms with VC4.

# EH frames are clues to cleanup lifetimes

Under a C++ profile using /GX, a sequence involving fs:[0], an exception-handler
address, and unwind-state stores can be a compiler-generated EH registration:

```asm
push -1
push <handler>
push <previous registration>
mov  fs:[0], esp
```

Check the [actual TU flags](../../config/units.toml) first; do not add a
per-function flag workaround to explain a source mismatch.

Then inspect construction, destruction, potentially throwing calls, scope, and
inlining together. Frame presence or state-store counts alone do not establish
how many source objects exist. Expanded constructors, repeated cleanup paths,
and state-flow changes can alter those counts without adding an object.

The [historical StepArrivalDrop control](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/eh-frame-presence-is-a-source-fact.md)
records equal state values and constructor/destructor call sets but different
state-store counts. It refutes the use of that count as a direct object census.

Matching an exact pair calibrates detector consistency, not semantic validity.
Do not treat an EH mismatch as an irreducible compiler choice, or fabricate a
destructible local just to obtain a frame.
