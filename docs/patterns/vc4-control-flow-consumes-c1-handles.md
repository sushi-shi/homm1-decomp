# VC4 control flow consumes C1 symbol handles

HoMM1 VC4 (`c1xx.exe` of the pinned `vc40` toolchain), measured. This entry extends
[operand sort keys](vc4-operand-sort-key-is-the-symbol-handle.md) and
[/O2 register allocation](vc4-global-register-allocation-is-chaitin-briggs.md).
Both read C1 symbol handles. C1 numbers handles sequentially over the whole TU,
and a function body also allocates handles for its compiler labels and for some
temporaries. Two bodies that compile to the same code can therefore leave
different handle state for every later function.

Signature: a later function's operand order, 2-D index order or `/O2` register
permutation flips when an earlier function is rewritten into a code-identical
form, such as `&&` into a nested `if`.

## Measurements

Probe: `void f(int a, int b, int c) { int x; BODY }` followed by
`void probe(int p1)`. C1 runs with `-il`, and `p1`'s handle is read from the `sy`
stream. The values below are handles consumed by `BODY`. The empty body consumes 0.

| body | handles |
|---|---|
| `if (a) g();` | 1 |
| `if (a) g(); else h();` | 2 |
| `if (a && b) g();`, `if (a && b && c) g();` | 1 |
| `if (a) if (b) g();` | 2 |
| `if (a \|\| b) g();`, `if (a \|\| b \|\| c) g();` | 2 |
| `if (a && (b \|\| c)) g();` | 2 |
| `if (a) if (b \|\| c) g();` | 3 |
| `if (a && b) g(); else h();` | 2 |
| `if (a) { if (b) g(); else h(); } else h();` | 4 |
| `if (!(a && b)) g();` | 2 |
| `while`, `for (;;)`, `do ... while` | 3 |
| `switch` with one case (with or without `break`) | 5; each further `case` or `default` +1 |
| a statement label | 1; `goto L; L:` 2 |
| `x = (short)(a + 1);` | 1; `static_cast<short>` is the same |
| `x = a ? b : c;`, `x = a && b;` | 0 |

Real TUs agree:

- In `SOURCE/SEARCH`, nesting `FindNearestObject`'s found test
  (`A && (B || C)` into `if (A) if (B || C)`) leaves its code unchanged and moves
  every later handle by +1. Removing one `static_cast<short>` moves them by -1.
- In `SOURCE/EVENTS`, the nested-with-else form of an `&& ... else` in `DoEvent`
  moves `EraseObj`'s parameter handles by +2.

Locals that are never referenced still take a `/Od` stack slot. They are not a
slot-neutral way to add handles.

## Using it

1. Measure a target's handle-window first. Insert `N` throwaway typedefs before
   it in a scratch copy and record which `N` reproduce retail. A window that
   flips with `N` is handle state. A flat sweep points to rank or to source
   structure.
2. If the required shift is small and lies before the target, look for the
   control flow of preceding functions (in the same TU) that retail could have
   written in a code-identical spelling. Typical candidates are `&&` versus nested
   `if`, `if/else` versus separate `if`s, casts, labels and loop forms.
   Rebuild the edited function and confirm its bytes are unchanged.
3. Prefer a spelling with donor support (HoMM2 Buka 2.1/PoL). A spelling picked
   only for its handle count is a reconstruction choice, like a local
   declaration order. Comment it at the edit.

## Not established

- The handle cost of every statement kind and of temporaries other than casts.
- Whether C2 temporaries (ids `0x8000+`) depend on these counts. The `/O2` ties
  among C2 temporaries looked independent of the TU prefix in
  `FindNearestObject`.
- Whether retail's source used any particular spelling. Equal code does not
  identify the spelling.
