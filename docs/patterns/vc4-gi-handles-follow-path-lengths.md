# /Gi: C1 handles depend on the compile's path strings (HoMM1 VC4, measured)

Measured with the pinned VC4.0 `C1XX` and `C2`, first on `SOURCE/PHILAI` and
then on the whole tree through `homm1.tool.fixedroot`. C2's operand sort and
its register tie order are functions of C1 symbol handles. So anything that
shifts handles can change code.

## Which paths matter

Under `/Zi /Gi`, with a fresh `.pdb`/`.idb` for each compile, three paths move
the handles:

- **The source path** as passed to C1 (`-f`). In split compiles, each extra
  character added one to every handle. This held for paths of 143 to 206
  characters.
- **The object path** (`-Fo`, directory included). Through fixedroot, PHILAI
  changed in 2 to 6 functions when only the object directory changed.
- **The paths of the opened headers.** Fixedroot's `HOMM1_FIXEDROOT_INCLUDE`
  knob moves `include/` under a subdirectory of `D:\Heroes`. That changed 6
  (`I`) or 1 (`INCLUDE`) of PHILAI's 60 functions. An earlier test reported
  that include roots made no difference. It used a symlinked `-I` root, which
  wine resolves to its target, so the opened paths never changed. That test
  was wrong.

**The strings matter, not only their lengths.** For GAME, an object directory
of `Heroes\Source` (the source's own directory) gave the same code as the
default. Same-length variants did not: `Heroes\Sourcf`, `Xeroes\Source` and a
13-character placeholder each changed 3 to 5 functions, and differently from
each other. Across the whole tree, the default layout (objects beside their
sources) does not equal the length-equivalent sweep points either: SOURCE at
13 characters with BASE at 11 differs in 23 functions. So no length-only model
predicts the result.

Without `/Gi`, handles do not depend on any of these paths.

## An existing .idb freezes handles

If the `-Fd` `.pdb`/`.idb` already exist, C1 reuses the previous compile's
handles, even for a different path or source text. One split-compile sweep
that reused a single `-Fd` measured no path effect at all. Fixedroot runs
every compile in a private tmpfs working directory, so `vc40.pdb` and
`vc40.idb` are always fresh. This covers match, build, `tu_state_noise` and
`batch_source_variants`, which all compile through `homm1.graph.cc`, then
`homm1.tool.cl`, then fixedroot.

## Fitting the object directory (whole tree, relaxed comparison)

The tree is n2-cur's `/Zi /Gi` profiles, before the master merge. Every C++
object was rebuilt for each run, and exact functions were counted out of
1,012. The object directory was set with `HOMM1_FIXEDROOT_OBJDIR`:

| Object directory under `D:\` | Exact |
| --- | --- |
| default: beside the source (`Heroes\Source`, `Heroes\Base`) | 954 |
| `Heroes\Debug` / `Release` / `WinDebug` / `WinRel` | 946 / 949 / 950 / 948 |
| `Heroes\Source\Debug` / `Release` / `WinDebug` / `WinRel` | 954 / 949 / 947 / 951 |
| `Heroes\Obj`, `Heroes`, `Heroes\Source`, `Heroes\Base` (all units) | 949, 951, 950, 951 |
| synthetic `Ox…x`, 1 to 30 characters | 942 to 957 (best: 25 characters, 957) |

Over all 31 sweep points, 923 functions are exact everywhere, 39 nowhere, and
only 50 depend on the object path. No directory stands out: the spread is ±6
around the default. The best synthetic point gains 3 over the default, and its
name is not retail-shaped. The sources were tuned under the default layout,
which biases the fit towards it. A clear peak would need source that was not
tuned under any `/Gi` path.

The build keeps objects beside their sources (`fixedroot.OBJ_DIR = ""`).
`HOMM1_FIXEDROOT_OBJDIR` remains for further fits. Header path spellings
(`D:\Heroes\SOURCE\army.h` against retail's unknown case and layout) are a
second unfitted parameter of the same kind.
