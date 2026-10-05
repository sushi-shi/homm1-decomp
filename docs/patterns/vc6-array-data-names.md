# VC6 array data names

The pinned VC6 SP5 compiler and Clang with `-fms-compatibility-version=12`
emit the same external data names for the following eleven declarations.
The control used `/Od /Ob1 /GX /MT /G5`:

```cpp
int writableInts[3] = {1, 2, 3};
char writableChars[3] = {1, 2, 3};
extern const int constantInts[3] = {1, 2, 3};
extern const char constantChars[3] = {1, 2, 3};
const int* pointersToConst[2] = {constantInts, constantInts};
extern int* const constantPointers[2] = {writableInts, writableInts};
int writableMatrix[2][3] = {{1, 2, 3}, {4, 5, 6}};
extern const int constantMatrix[2][3] = {{1, 2, 3}, {4, 5, 6}};
volatile int volatileInts[3] = {1, 2, 3};
extern const volatile int constantVolatileInts[3] = {1, 2, 3};
extern const float constantFloats[2] = {1.0f, 2.0f};
```

| Declaration | VC6 and Clang name |
| --- | --- |
| `constantChars` | `?constantChars@@3QBDB` |
| `constantFloats` | `?constantFloats@@3QBMB` |
| `constantInts` | `?constantInts@@3QBHB` |
| `constantMatrix` | `?constantMatrix@@3QAY02$$CBHA` |
| `constantPointers` | `?constantPointers@@3QBQAHB` |
| `constantVolatileInts` | `?constantVolatileInts@@3SDHD` |
| `pointersToConst` | `?pointersToConst@@3PAPBHA` |
| `volatileInts` | `?volatileInts@@3RCHC` |
| `writableChars` | `?writableChars@@3PADA` |
| `writableInts` | `?writableInts@@3PAHA` |
| `writableMatrix` | `?writableMatrix@@3PAY02HA` |

In particular, `const int[]` uses `QBHB`, and a const multidimensional array
retains its `$$CB` element qualifier. The existing VC4 transformation changed
`Q` to `P` and stripped the matrix qualifier. Applying it to VC6 produced a
claim for a symbol the compiled object did not define.

`core/msvc_names.py:data` now preserves the VC6-compatible front end's array
spelling. It retains the previous transformations for the VC4 profiles and
keeps the existing compiler-specific linkage and scope handling. This changes
source-derived identities; it adds no relocation aliases or comparison masks.

The retail-backed case is `gEnvironmentVolume`: both independently checked
ambient-audio users refer to the five signed integers at Buka RVA `0x8a36c`.
The reconstructed object defines `?gEnvironmentVolume@@3QBHB`. The same fix
restores `gTownObjectType`'s const-matrix name. Across the current 71 object
units, all 698 source data claims resolve to their own unit's definitions:
two repaired joins, zero regressions and no remaining name-join failures.
This checks spelling, not the still-incomplete retail address migration.

The controls are captured by `core/test_msvc_names.py`; legacy VC4 rewrite
expectations remain separate.
