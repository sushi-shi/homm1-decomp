# VC6 string literal or named array: `.data` order decides

Measured with the pinned VC6 SP5 (C2 12.00.8966) under `/Od /Ob1 /GX /MT /G5`
and `/Od /Ob2`, on probe TUs and on `BASE/EXEC`, `BASE/WINDOW`,
`EDITOR/EDITMGR` and `SOURCE/kbwin` of both linked programs.

A message passed once, `ShutDown("...")`, compiles to the same instruction
whether its text is a literal or a named `char[]`: both are a `push offset`
with one DIR32 relocation. The identity is settled by where the text sits in
the unit's data, which follows these rules:

- **Initialized `.data`.** Named objects with a nonzero initializer come first,
  in parse order: external and file-static arrays, and function-local statics
  at their function's position. The unit's `$SG` literals follow, in function
  order and use order within a function. A `const` array goes to `.rdata`.
- **No pooling.** Without `/GF` every literal occurrence is its own `$SG`:
  `ShutDown("x")` written six times gives six copies of `x`.
- **Zero storage.** `char name[4] = ""` is a zero-initialized object in
  `.bss` (definition order, after the uninitialized ones), and a `""` literal
  is a one-byte `.bss` member after all of them
  ([empty strings](vc6-empty-string-bss.md)).
- `/Ob2` gives the same order for arrays and function literals. It only moves
  the literal of a file-scope pointer initializer
  ([/Ob2 literal order](vc6-ob2-literal-order.md)).

## Reading retail

A string is a named array when retail shows one of these:

- a named variable after it in the same unit's `.data`;
- a literal of an earlier function before it;
- two or more uses that reach one address.

A string with one use that sits among its unit's literals, at its user's
position, is a literal. When the unit has no other initialized data, both
forms give the same image; the literal at the call site is the source form.

## Measured cases

| Unit | Strings | Result |
| --- | --- | --- |
| `BASE/EXEC` | 12 start-up and manager errors (six copies of one text) | the unit's `.data` is exactly these texts in `InitSystem`, `DoDialog`, `CallManager`, `Terminate` order: inlined, both programs identical |
| `BASE/WINDOW` | the two constructor names | the unit's only `.data`, in constructor order: inlined |
| `EDITOR/EDITMGR` | `Main`'s file name for `PickMap` (0x00451b84) | last `.bss` item after the zero-initialized globals: a `""` literal |
| `SOURCE/kbwin` | window class name and title | before `gUnusedWindowValue` and `gCDTrackName`, and the class name has two users: named. Inlining the title changes 117 `.data` bytes and 7 `.text` bytes of the linked game |

Pointer variables initialized from a literal (`char* gFRDummy = "";`) are not
in this class. Their users load the pointer from memory, so the instruction
bytes prove the variable.
