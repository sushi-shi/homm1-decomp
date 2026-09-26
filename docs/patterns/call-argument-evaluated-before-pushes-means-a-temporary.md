> Imported from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
> This is donor evidence, not a validated HoMM1 VC4 rule. Commands and source
> examples describe that donor. Re-prove applicable mechanisms with VC4.

# A call argument evaluated before the pushes means a temporary

Signature: retail calls an inner function first and only then pushes the
outer call's other arguments (constants included), e.g.

    call  ReadBranchTarget        ; inner call first
    push  1                       ; then the constant
    push  eax
    call  ScriptJumpUnless

Written as one expression, `ScriptJumpUnless(ReadBranchTarget(), 1)`, cl 5.0
/Ox emits the pushes right to left and evaluates the inner call where its
argument slot falls, so the constant is pushed before the call. Every
retail instance of the call-first order recovered in this project matched
once the inner result went through a local first:

    target = ReadBranchTarget();
    ScriptJumpUnless(target, 1);

Observed in ExecScriptOpcode (the branch-skip opcodes), RunFieldEncounter,
AddCondition, and lane 2's field-object step code (`direction =
TurnDirection(...)`, `id = GetPanelRowId(...)`).

What it does not establish: the local's name or type, or that every
temporary in the original looked like this. cl may also keep the value in a
register without a stack slot, so the local's presence is inferred from
evaluation order alone.
