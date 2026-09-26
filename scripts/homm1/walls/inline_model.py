"""Call-set and visibility evidence from Giten's inline diagnostic.

Only --gap RVA is applicable without a measured VC4 inline-budget model.
Giten's VC5 /O2 budget arithmetic is deliberately not presented as a VC4 fact.
"""
from homm1.core.usage import logged


def die(message):
    raise SystemExit(message)


def gap_from_rva(token: str) -> int:
    """`--gap <rva>`: name the call-set delta and report candidacy evidence.

    AGENTS.md points every inline/call-set wall at this verb, but the only form
    it had took a spec JSON of front-end `cb` estimates - numbers nobody has for
    a real row - so the documented lever could not be invoked at an address at
    all.  What IS exactly derivable from an address is the two questions that
    come BEFORE the budget arithmetic, and they decide whether the budget lever
    applies:

      1. WHICH callees differ, from the same normalized pair `walls diagnose`
         reads;
      2. what our own base obj's symbol table proves. A locally defined COMDAT
         positively proves inline visibility. An UNDEFINED symbol does not
         prove the opposite: a header inline may expand at one site and leave
         an external reference at a declined or nested site. An absent symbol
         is likewise consistent with every local site expanding. `/O2` still
         implies `/Ob1`; source declarations, an `/Ob0` census, or ordered
         call-site topology must settle the ambiguous cases.

    The `cb` arithmetic still needs `cb`, and this refuses to invent one:
    measure it with `--measure-cb` and pass a spec.  A guessed budget deficit
    printed as a model output would be indistinguishable from a measured one.
    """
    from collections import Counter

    from homm1.walls.diagnose import (
        NORM, _call_targets, _find_function, _jump_table_bytes, _locate,
        _skeleton,
    )
    from homm1.delink.coffx import Obj

    b, why = _locate(token)
    if b is None:
        die(why)
    base_p = NORM / "base" / f"{b.unit}.obj"
    tgt_p = next((p for p in (NORM / "target" / f"{b.unit}.c.obj",
                              NORM / "target" / f"{b.unit}.obj") if p.is_file()),
                 None)
    if not base_p.is_file() or tgt_p is None:
        die(f"normalized pair missing for {b.unit} - run `homm1 build` first")

    calls = {}
    for tag, path in (("base", base_p), ("target", tgt_p)):
        payload, rel, _size = _find_function(Obj(path), b.name)
        if payload is None:
            die(f"{tag} obj does not define {b.name}")
        _m, _c, _br, _r, _i, asm = _skeleton(
            payload, rel, data=_jump_table_bytes(rel, b.name))
        calls[tag] = Counter(n for n, _a in _call_targets(rel, asm, b.name))

    obj = Obj(base_p)
    symbols = {obj.sym_name(i) for i, _v, _sn in obj.iter_symbols()}
    defined = {obj.sym_name(i) for i, _v, sn in obj.iter_symbols() if sn > 0}

    print(f"[budget-gap] {b.name}  [{b.unit}]  rva 0x{b.rva:06x}")
    delta = [n for n in sorted(calls["base"].keys() | calls["target"].keys())
             if calls["base"][n] != calls["target"][n]]
    if not delta:
        print("  call-set delta: none - this row is not an inline/call-set "
              "wall, so the budget model has nothing to say about it.")
        return 0
    for n in delta:
        bn, tn = calls["base"][n], calls["target"][n]
        side = "base calls MORE" if bn > tn else "base calls FEWER"
        print(f"  {n}\n      target {tn}, base {bn}   ({side})")
        if bn > tn:
            if n in defined:
                print("      DEFINED: this TU emits a definition. "
                      "Visibility alone does not prove inlining;\n"
                      "                      inspect source and configured "
                      "compiler flags;\n"
                      "                      VC5 budget constants are not validated for VC4.")
            elif n in symbols:
                print("      AMBIGUOUS: UNDEFINED external in this obj. It may "
                      "be an ordinary out-of-line\n"
                      "                 function OR a visible header inline "
                      "with another declined/nested\n"
                      "                 reference. Inspect the declaration, "
                      "/Ob0 census, and site topology.")
            else:
                print("      AMBIGUOUS: no base symbol-table row. The call-set "
                      "delta alone cannot\n"
                      "                 distinguish a missing site from an "
                      "inline expansion; inspect source\n"
                      "                 visibility and ordered site topology.")
        else:
            if n in defined:
                print("      DEFINED: this TU emits a definition, and "
                      "base has fewer calls.\n"
                      "                      Expansion is possible, but compare "
                      "site topology before\n"
                      "                      attributing the delta to budget.")
            elif n in symbols:
                print("      AMBIGUOUS: UNDEFINED external in this obj does not "
                      "rule out a header-inline\n"
                      "                 expansion. It also does not rule out "
                      "tail sharing. Inspect the\n"
                      "                 declaration, /Ob0 census, and ordered "
                      "site topology.")
            else:
                print("      AMBIGUOUS: symbol absent from the base obj. This is "
                      "consistent with full\n"
                      "                 expansion OR with no corresponding "
                      "source site; compare the local\n"
                      "                 expansion and tail-sharing topology.")
    return 0


@logged
def main(argv=None):
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gap", required=True, help="function RVA, VA or symbol")
    args = parser.parse_args(argv)
    return gap_from_rva(args.gap)


if __name__ == "__main__":
    raise SystemExit(main())
