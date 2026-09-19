"""homm1.core.msvc_names - one spelling for one cl 5.0 symbol.

Two directions over the same vocabulary:

  * FORWARD (labelling). clang proposes a mangled name; cl 5.0's own spelling
    for the same declaration is a DETERMINISTIC function of that name plus the
    declaration's linkage, so `func`/`data` derive it from SOURCE alone - no
    object is read, and a stale build artifact can no longer answer for source
    that has changed. Three rules, each measured over the whole claim corpus
    (`homm1 verify selftest -k SourceNameRewrite` re-proves them per build):
      1. the i386 COFF global prefix - LLVM adds `_` to every name that is not
         already MSVC-mangled (`?...`); the IR value name lacks it, libclang's
         `mangledName` already carries it;
      2. array storage class - clang spells a top-level array `@@<d>Q` (const
         pointer), cl 5.0 `@@<d>P`;
      3. TU-local storage - cl's `outdname` wraps a datum with no external
         linkage as `_<mangled>$S<n>`; the `<n>` is dropped (see MASK).

  * MASK (joining). cl stamps a per-object CodeView counter onto every TU-local
    datum (`name$S<n>`) and numbers a function-local static's enclosing lexical
    scope (`@?<n>??`, `<n>` counting every scope c1 opened). Both renumber on
    any edit to the translation unit, so neither is ever stored: `mask` reduces
    EITHER side of a name join to the ordinal-free form, which is what makes a
    canonical claim and cl's own object symbol meet.

`discriminate` is the one sanctioned way back to a unique spelling: several
translation units can hold a same-named TU-local static, and the delink data
manifest needs one name per address image-wide. The retail rva is the
discriminator - the convention cl-generated data already uses (`$T<rva>` FP
pool slots), and `mask` folds it straight back onto the family.
"""

from __future__ import annotations

import re

#: clang's top-level-array storage class, at the mangled storage-class digit.
ARRAY_STORAGE = re.compile(r"@@([0-9])Q")
#: cl's per-object CodeView counter on a TU-local datum. Every occurrence is
#: volatile, not just the trailing one (a function-local static's guard byte is
#: spelled `?$S<n>@?<scope>??<fn>@4EA$S<n>`). A name that is NOTHING BUT `$S<n>`
#: is not this: that is the rva-keyed spelling `discriminate` produces.
STATIC_ORDINAL = re.compile(r"(?<=.)\$S[0-9]+(?=@|$)")
#: cl's lexical-scope number in a function-local static's mangled name. MSVC
#: spells 1..10 as the digits `0`..`9` and larger values in hex as `A..P@`.
LOCAL_STATIC_SCOPE = re.compile(r"@\?(?:[0-9]|[A-P]+@)\?\?")
#: the scope spelling both sides agree on - clang's, for the one scope we model.
CANONICAL_SCOPE = "@?1??"


def decorate(name: str) -> str:
    """The i386 COFF global prefix LLVM applies to a name it did not mangle."""
    return name if name.startswith("?") else "_" + name


def mask(name: str) -> str:
    """`name` with every volatile cl ordinal reduced to its canonical form."""
    return LOCAL_STATIC_SCOPE.sub(CANONICAL_SCOPE, STATIC_ORDINAL.sub("$S", name))


def func(name: str, *, decorated: bool = False) -> str:
    """cl 5.0's spelling for a clang-proposed FUNCTION name."""
    out = ARRAY_STORAGE.sub(r"@@\1P", name)
    return mask(out if decorated else decorate(out))


def data(name: str, *, internal: bool, decorated: bool = False) -> str:
    """cl 5.0's spelling for a clang-proposed DATA name.

    `internal` is the declaration's storage, not its spelling: a file static, a
    namespace-scope `const`, and a function-local static all reach the object
    as `_<mangled>$S<n>`, whatever their mangling.
    """
    out = ARRAY_STORAGE.sub(r"@@\1P", name)
    if not decorated:
        out = decorate(out)
    if internal:
        if not out.startswith("_"):
            out = "_" + out
        out += "$S"
    return mask(out)


def discriminate(name: str, rva: int) -> str | None:
    """`name` respelled for one rva, or None when the family has no room.

    Only the `$S` family carries a discriminator: the ordinal slot cl fills
    with a per-object counter takes the retail rva instead, so the spelling is
    unique image-wide and `mask` still folds it onto the shared family.
    """
    return f"{name}{rva}" if name.endswith("$S") else None
