> Unmeasured hypothesis: observed with MSVC 5.0; not measured on this
> target's VC4 or VC6 compilers. Re-prove the mechanism before relying on it.

# A call argument evaluated before the pushes means a temporary

Signature: retail calls an inner function first and only then pushes the
outer call's other arguments (constants included), e.g.

    call  ReadTarget              ; inner call first
    push  1                       ; then the constant
    push  eax
    call  JumpUnless

Written as one expression, `JumpUnless(ReadTarget(), 1)`, MSVC 5.0 /Ox
emits the pushes right to left and evaluates the inner call where its
argument slot falls, so the constant is pushed before the call. In the
observed instances, the call-first order was reproduced once the inner
result went through a local first:

    target = ReadTarget();
    JumpUnless(target, 1);

What it does not establish: the local's name or type, or that every
temporary in the original looked like this. The compiler may also keep the
value in a register without a stack slot, so the local's presence is inferred
from evaluation order alone.
