> Unmeasured hypothesis: observed with MSVC 5.0; not measured on this
> target's VC4 or VC6 compilers. Re-prove the mechanism before relying on it.

# Signed remainder can include sign correction around a mask

A signed remainder by a positive power of two cannot generally be replaced by
an unsigned mask: negative dividends require a negative or zero remainder.

An observed MSVC 5.0 sequence computes an unsigned magnitude, masks it, and restores
the original sign:

```asm
cdq
xor eax, edx
sub eax, edx
and eax, 7fh
xor eax, edx
sub eax, edx
```

With EDX retaining the original sign mask, this implements signed remainder
modulo 128. The observed example emits this sequence even after a mask that
made the input nonnegative.

Recognize the data flow before introducing handwritten absolute-value logic.
This does not prove a unique C++ expression or that MSVC 5.0 always uses this
lowering. Check operand width, signedness, preceding range constraints, and
whether the sign mask is actually preserved through the sequence.
