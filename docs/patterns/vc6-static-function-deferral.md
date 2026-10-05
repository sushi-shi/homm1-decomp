# VC6 defers internal-linkage function bodies

Measured with the pinned HoMM1 Buka VC6 SP5 compiler under
`/Od /Ob1 /GX /MT /G5`. A `static` function is not compiled where it is
defined. VC6 emits it after the first external function that references it,
and its string literals follow that caller's literals:

```cpp
static int A(int x) { printf("%d:", x); return x; }
int B(int x) { printf("%d:", x); return A(x); }
int C(int x) { printf("%d:", x + 1); return x; }
```

| Emitted order | Code | `$SG` literals |
| --- | --- | --- |
| `static A` | B, A, C | B's, A's, C's |
| external `A` | A, B, C | A's, B's, C's |

Retail evidence therefore distinguishes the two linkages even when the helper
has a single caller: a helper placed before its caller in the image, whose
literals also precede the caller's literals, had external linkage. Separate
literals with identical text are not merged in this profile, so the comparison
detects the order through each literal's occurrence index.

`DriveSupportsFreeSpaceQuery` in `SOURCE/kbwin` (RVA `0x44702`) precedes its
caller `SetupCDDrive` and owns the first of the unit's two `"%c:"` literals.
With `static`, its literal became the second occurrence; external linkage
gives the retail referent.
