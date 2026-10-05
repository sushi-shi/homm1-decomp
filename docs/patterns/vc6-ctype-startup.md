# VC6 locale startup from a standard header

With the pinned VC6 SP5 compiler, this complete translation unit is enough to
emit startup code for `std::ctype<wchar_t>::id`:

```cpp
#include <string>
```

The installed `STRING` header includes `ISTREAM`. That instantiates the
`ctype<wchar_t>` template declared in `XLOCALE`; VC6 represents `wchar_t` as
`unsigned short`, encoded `G` in the mangled name. `locale::id` has a `size_t`
member. The compiler also emits the separate one-byte COMMON guard:

```text
??_B?1???id@?$ctype@G@std@@$D@@9@51
```

Three real Buka audio objects independently provide the unoptimized control:
a 39-byte startup body, an 18-byte registration body and a five-byte synthesized
cleanup named `?id@?$ctype@G@std@@$E`. The startup body tests and sets bit zero
of the guard, then calls the registration body. The registration body passes
the cleanup address to `_atexit`. All instructions and all reference fields
are checked, rather than identifying the guard from its zero byte alone.

The header-only `/O2 /Ob2 /GX /MT /G5` control emits one 32-byte body. It combines
the guard and registration sequence, with two guard references, one cleanup
reference and one `_atexit` call. Its synthesized cleanup is a single `ret`;
retail's optimized bodies refer to the same shared cleanup symbol whose
retained unoptimized contribution contains a frame prologue and epilogue.

Header selection matters: `<locale>` instead emits both `collate<char>` and
`ctype<wchar_t>` initialization in a 63-byte body. `<xlocale>` alone emits no
initializer. These controls distinguish actual instantiation from declarations
and explain why adding an arbitrary dummy constructor is not a reconstruction.

HoMM1 Buka contains sixty complete unoptimized initializer/registration pairs
and two optimized bodies, after FINDPATH and SEARCH. Together they account for
all 184 absolute references to the guard and all 62 associated CRT initializer
entries. The guard is one loader-zero byte at RVA `0xd82d8`. The complete
instruction controls, object/header hashes, references and CRT entries are in
[`buka-ctype-guard.json`](../../config/retail/buka-ctype-guard.json).

Use the existing `data_compgen.tsv` COMMON mechanism for this compiler-owned
storage; never introduce a source global to imitate it. The control's volatile
`_$E` ordinal is evidence only, not a stable function identity. The guard's
address and extent come from HoMM1's own verified image and objects.

This establishes the data identity and the compiler mechanism. Most HoMM1
game units still lack the relevant header emission, so matching their startup
bodies remains unfinished. Do not infer source-header completion from successful
enrollment of the shared guard.
