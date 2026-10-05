> The MSVC 5.0 observations below are unmeasured hypotheses on this target.
> The [HoMM1 VC4](#homm1-vc4) section links the measured mechanism.

# Translation-unit context can change an unchanged function

MSVC 5.0 experiments changed unrelated declarations while leaving a target
body unchanged and obtained different allocation or scheduling. Function text
alone is therefore not a complete description of compiler input.

One such experiment used /d1il<prefix> to capture the ex/gl/in/sy streams and
/d2il<prefix> to replay them. The replay reproduced the changed output. This
locates an input difference at the frontend/backend boundary; it does not prove
the downstream decision is made by the frontend.

Another fixed the target body and every other declaration and added k unused
prototypes before the target: k = 1, 3, 4, 5 gave the retail register
assignment and k = 2 an esi/edi swap. The effect is not monotonic in the count,
so it says nothing about which declarations the original TU held.

For a controlled comparison, keep compiler, flags, headers, and target source
fixed; retain both objects and compare ordered referents as well as instructions.
IL byte differences need interpretation, including source-line records.

A flat sweep means those probes did not move that input. Neither uniform nor
mixed probes prove a function unreachable, a TU permanently insensitive, or a
particular backend cause. Measured handle strides belong to those probes; they are
not portable compiler constants.

Do not retain unused declarations, includes, or fake locals to select an output.
Use the bounded [permuter workflow](../permuter.md), not an unbounded hunt for
a favorable score. Source correctness and the current MAX policy still govern
what is kept.

## HoMM1 VC4

For HoMM1 VC4 the operand-order part of this effect has been measured in
[operand sort keys](vc4-operand-sort-key-is-the-symbol-handle.md): the order
follows the operands' C1 symbol handles, which C1 numbers sequentially over the
whole TU. A shared header that gains or loses prototypes therefore moves the
handles of every unit that includes it, and an unedited function in such a unit
can load `a + b` in the other order. Earlier control flow moves them as well
([control flow consumes handles](vc4-control-flow-consumes-c1-handles.md)).
The order in which locals are declared can be searched instead of adding
declarations to the TU.
