> Unmeasured hypothesis: observed with MSVC 5.0; not measured on this
> target's VC4 or VC6 compilers. Re-prove the mechanism before relying on it.

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

An MSVC 5.0 control had equal state values and constructor/destructor call
sets but different state-store counts. It refutes the use of that count as a
direct object census.

Matching an exact pair calibrates detector consistency, not semantic validity.
Do not treat an EH mismatch as an irreducible compiler choice, or fabricate a
destructible local just to obtain a frame.
