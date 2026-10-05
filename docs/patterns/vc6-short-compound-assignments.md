# VC6 word-sized compound assignments

Measured in the complete HoMM1 Buka `SOURCE/CMBTMGR` unit with pinned VC6 SP5
`/Od /Ob1 /GX /MT /G5`. For signed-word locals, the expanded expression
`x = dx + x` loads and adds promoted 32-bit values. The compound assignment
`x += dx` loads, adds and stores words:

```asm
; x = dx + x
movsx edx, word ptr [dx]
movsx eax, word ptr [x]
add edx, eax
mov word ptr [x], dx

; x += dx — retail form
mov dx, word ptr [x]
add dx, word ptr [dx]
mov word ptr [x], dx
```

Here `[x]` and `[dx]` denote the respective stack locations. Multiplicative
increments still compute the product at promoted width, then add its low word
to the destination word. The descending projectile step evaluates `i * dy`
in that order before `y += ...`.

The catapult controls isolate two conditional-value restorations before this
arithmetic change. After the column and target choices, the function has all
109 retail CFG blocks but is six bytes longer. Restoring the six coordinate
compound assignments reduces it to retail's 3,314 bytes. Every instruction,
branch and reference agrees under a separately verified bijection of local
stack slots. No branch or operand is masked in the actual matching pipeline.

This preserves the stored signed-word values for the routine's bounded
arithmetic. It is evidence about these measured expressions, not a rule to
rewrite arbitrary arithmetic or to change operand types. Complete controls
and references are in [`buka-catapult.json`](../../config/retail/buka-catapult.json).


The same distinction occurs for a signed-byte member in `SOURCE/COMMAND`
with pinned VC6 SP5 and the then-current `/G6` profile. Both `hex += 1` and
`hex++` emit a byte
load and byte addition. `hex = hex + 1` emits `movsx` and a 32-bit addition,
then stores the low byte. The corresponding subtraction behaves likewise.
HoMM1 Buka's mouse-direction fallback uses the promoted form; an increment
spelling does not reproduce it in this translation unit. These
controls explain the two-byte size difference without changing the member type.
The final explicit assignments also reproduce these retail instructions under
the subsequently established `/G5` unit profile.
See [`buka-combat-direction.json`](../../config/retail/buka-combat-direction.json).
