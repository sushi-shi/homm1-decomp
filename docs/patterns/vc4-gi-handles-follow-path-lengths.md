# /Gi: C1 handles shift with the source and object path lengths (HoMM1 VC4, measured)

Measured with the pinned VC4.0 `C1XX` on `SOURCE/PHILAI`. For each compile,
the C1 handle of the first initialized datum is read from the IL `in` stream
(the 2-byte value after its record tag: `b[1] & 0x7f | b[2] << 7`). C2's
operand sort and register tie order are functions of these handles. A
uniform shift of all handles therefore changes code. Two of the 60 PHILAI
functions changed when only the source directory name grew by 15
characters.

## Rule

Under `/Zi /Gi` (fresh `.pdb`/`.idb` each compile), every C1 symbol handle
grows by one per character of:

- the source path as passed to C1 (`-f`, the path CL was given): 143 to 206
  characters gave +1 per character;
- the object path as passed to C1 (`-Fo`, the whole path including its
  directory): a 24-character longer directory gave +24, and a longer
  basename +1 per character.

These do not move handles: case (`OZ.OBJ` vs `oz.obj`), the include
directories (`-I` roots 10 and 50 characters longer gave the same handles
and byte-identical PHILAI code), the `-Fd` PDB path, and the `-il` IL temp
path.

So, apart from boundary effects, only the sum `len(source path) +
len(object path)` matters. One boundary effect seen: when handles cross
0x4000, the shift for the 64-character case was +128 rather than +64. A
relative source path (`sy/PHILAI.cpp`) moved the handles by twice its length
change. Without `/Gi`, handles do not depend on these paths.

## An existing .idb freezes handles

C1 reuses the previous compile's handle assignment when the `-Fd`
`.pdb`/`.idb` already exist, even for a different path or source text. This
is incremental compilation. A sweep that reused one `-Fd` measured no path
effect at all. Any `/Gi` build or measurement must start each unit's compile
without a stale `.pdb`/`.idb`, or results depend on what was compiled before
it.

## Consequence

Retail's handles reflect the length of its own source path plus object path
for each unit. That is a single unknown integer per unit (or one per build
layout, such as `D:\Heroes\Source\X.CPP` with its `.obj` directory). Padding
the object directory sets it without touching sources, so the retail length
can be found by sweeping that sum and scoring each unit.
