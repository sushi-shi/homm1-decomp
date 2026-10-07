# Constants work list

`homm1 verify constants` parses every unit with libclang and lists each numeric
literal in `src/` and `include/` (enumerator values excluded). The editor-only
units (`src/EDITOR`) are parsed with the editor image's compile commands, so
one census and one floor cover both programs; `homm1 verify enum-reuse` reads
every unit of each image (a shared unit once per image). A literal is
open until it is spelled as a name (an enumerator, a named macro, `NULL`,
`TRUE`/`FALSE`) or a row in `config/constants.tsv` keeps it numeric with a
reason. The committed `#floor` is the open count and only goes down.

```sh
homm1 verify constants                 # census, build/gen/bare_constants.tsv
homm1 verify constants --list KB.cpp   # open sites whose file/owner contains KB.cpp
homm1 verify constants -v              # proven replacements and retail-view units
homm1 verify constants --gate          # fail on proven replacements, a risen
                                       # floor or stale/malformed rows
homm1 verify constants --update-floor  # lower the floor after a batch
```

The open list is also written to `build/gen/constants_open.tsv` (file, line,
owner, spelling, group, detail, replacement).

Rows are tab-separated fnmatch globs over file, owner, spelling, review group
and detail, then a reason; the first matching row wins, so put narrow rows
before broad ones. A row that keeps nothing is stale and fails the gate. The
detail column records the context: `argument N` of the callee, `switch on X`
for case labels, `store to X`, `compound assignment to X`, `initializer of X`
and the other operand of comparisons and arithmetic, so a row can name the
field or call it covers. Keep rows for quantities that have no name in the
game: pixel geometry of the retail layouts, icon frame numbers, random bounds,
byte widths of saved fields, delays. A value that a domain names is not kept;
name it.

Each site also carries `context_key`/`context_label`: the declaration identity
of its destination (callee parameter, field, variable, comparison operand,
switch subject, array or function return; `verify/constant_context.py`). `build/gen/constant_contexts.tsv` groups the counted
sites by that key, so one destination's literals are reviewed together. The
keys are review leads: one `Read` length can receive unrelated sizes.

## Booleans

The Buka target is VC6 (`config/units.toml` `compiler = "vc6"`), which has
`bool`, `true` and `false`. A `0`/`1` stored to, passed as, returned as or
compared with a C++ `bool` is proven `false`/`true`; condition and logical
operand truthiness (`while (1)`, `!0`, `x && 1`) is not a boolean
destination. Win32 `BOOL` stays `TRUE`/`FALSE`. The rules below apply to VC4
targets.

A flag stored in an integer keeps the integer's width: `b8` and `b32`
(`include/H1/Ints.h`) are the 8- and 32-bit storage of a genuine boolean
(only 0/1, tested for truth, set from comparisons). The retail view is the
plain integer, so codegen is that of the integer flag (C++ `bool` would
normalize stores); the strict view is a wrapper that converts only to and
from `bool`. A `0`/`1` stored to, passed as, returned as or compared with a
`b8`/`b32` declaration is proven `false`/`true`. Fields, globals, locals,
parameters and returns are flags when every store is 0/1, a comparison, a
logical expression or another flag and every read is a truth test; a flag
of one width converts to the other. A flag that only an `i16` (or a Win32
`BOOL`) carries keeps its integer and its 0/1 literals.

A row in `config/constants.tsv` never keeps a proven site: retyping a
destination makes its rows stale, and `--fix` spells the sites.

### VC4 booleans

VC4 has no `bool`, `true` or `false` (C2065). Clang's C++ `bool` contexts
(conditions, logical operands) are int truthiness in the retail compiler and
prove nothing. Only Win32 `BOOL` is a boolean domain, and its proven spelling
is `TRUE`/`FALSE`, offered only where the macros are visible. The gate also
fails on any `true`/`false` spelling.

## Strict-domain view

Each unit is parsed first with `/std:c++20 /Zc:__cplusplus`, which selects the
strict view in `include/Domains.h`. There, typed storage, parameters and
returns carry their enum, so a literal compared with or switched on
an unscoped (`FLAGS`/`CONST`) domain is proven as its unique enumerator. A
literal that meets an `enum class` domain is a strict-view error; such units
fall back to the retail view, are counted in the output (`-v` lists them), and
the error is itself the site to name.

`homm1 verify strict-view` (run by `homm1 build verify`) parses every unit of
every image's compile database in the strict view with `clang-cl /Zs` and
fails when the distinct error count rises above `config/strict_view.floor`
(`--update-floor` lowers it, `--unit TEXT --list` shows one unit's errors).

## Typed enum arrays

An array indexed by one domain is declared with
`H1_ENUM_ARRAY(type, name, Domain, COUNT)` (`include/Domains.h`); two
domain-indexed dimensions use `H1_ENUM_ARRAY2(type, name, Domain1, COUNT1,
Domain2, COUNT2)`. The retail view expands to the plain `type name[COUNT]`,
so VC6 sees no class, no inline `operator[]` and no change in frame slots or
C1 state. The strict view expands to `H1EnumArray<type, Domain, COUNT>`, whose
subscript and `+` accept only `Domain` (or its `H1_ENUM_STORAGE`); a raw
integer or another domain's value selects a deleted overload, and the array
still converts to its element pointer as the retail array decays.
`H1_ENUM_STEPPED(Domain)` gives a domain used as a loop variable its `++`,
`--`, `+ n` and `- n` in the strict view (nothing in the retail view).

Do not add an `IDX(x)`-style integer escape: a domain-indexed array takes the
domain's extent, its index is typed storage or a typed loop variable, and a
genuine conversion at a domain boundary is a narrow, commented one. `enum-domains`
rejects an `H1_ENUM_ARRAY`/`STEPPED` domain that is undeclared, a constant
group or a flag set; `scripts/homm1/verify/test_domains.py` checks both views.

