"""homm1.model - the one join: census rows x provider claims -> bindings.

    homm1 model            resolve, write build/gen/bindings.tsv +
                            violations.tsv (both write-if-changed - the
                            bindings file is the delink key), print a summary

THE RULE: functions.tsv / data.tsv contribute ONLY structure - starts, kinds,
derived extents. Every identity (name, unit, exact matched size) comes from a
channel: the extracted source claims (VA/DATA/VA_COMPGEN/RVA_DYNINIT/
DATA_COMPGEN) and the committed provider tables. A claim whose rva is not an
admitted census row is a violation; a claim size may never cross the next
admitted start.

Resolution policy (this module is the ONLY place policy lives):
  * LOW-confidence static-lib rows are leads, not claims - filtered here;
  * channel precedence per rva: src > src_compgen > src_dyninit >
    src_data_compgen > functions_zlib/data_zlib > data_vtables >
    data_compgen > data_static_libs > functions_static_libs; later claims on
    the same rva become recorded ALIASES, never silent losers;
  * function extent = claimed size when the winning channel states one
    (src / zlib), else the census-derived extent;
  * kind compatibility: func claims bind kind ''|helper (static-lib labels
    also thunk - retail interleaves are real); data claims must match their
    census kind where the channel implies one (vtable/rtti/common/copy).
"""

from __future__ import annotations

from typing import NamedTuple

from homm1.core.paths import BUILD
from homm1.core.tsv import write as write_tsv
from homm1.retail_labels import Claim, censuses, fragments as src_claims, providers

BINDINGS = BUILD / "gen/bindings.tsv"
VIOLATIONS = BUILD / "gen/violations.tsv"

_PRECEDENCE = ["src", "src_compgen", "src_dyninit", "src_data_compgen",
               "functions_zlib", "data_zlib", "data_vtables", "data_compgen",
               "data_static_libs", "functions_static_libs"]

#: channels whose claimed size is the exact matched extent (overrides derived,
#: bounded by it - the overrun check guards the other direction). Every channel
#: that states a size means it; label-only channels state None.
_SIZE_AUTHORITY = {"src", "src_compgen", "src_dyninit", "src_data_compgen",
                   "functions_zlib", "data_zlib", "data_vtables",
                   "data_static_libs", "data_compgen"}

#: census kinds a func claim may bind, per channel
_FUNC_KINDS = {"src": {"", "helper"}, "src_compgen": {"", "helper"},
               "src_dyninit": {""}, "functions_zlib": {""},
               "functions_static_libs": {"", "thunk", "helper"}}

#: census kind a data channel implies (None = any non-bookkeeping kind)
_DATA_KIND = {"data_vtables": "vtable", "data_zlib": None, "src": None}


class Binding(NamedTuple):
    rva: int
    size: int
    kind: str          # the census kind
    space: str         # 'text' | 'rdata' | 'data' | 'bss'
    name: str          # winning claim's name, or '' (unclaimed)
    unit: str
    channel: str       # winning channel, or ''
    aliases: tuple     # losing Claims on the same rva
    also: tuple = ()   # other units carrying the same header-inline claim


class Model(NamedTuple):
    functions: list[Binding]
    data: list[Binding]
    violations: list[str]

    def claimed(self, kind=None):
        rows = (self.functions if kind == "func" else
                self.data if kind == "data" else self.functions + self.data)
        return [b for b in rows if b.channel]


def _active(claim: Claim) -> bool:
    if claim.channel == "functions_static_libs":
        return claim.meta.get("confidence", "").upper() != "LOW"
    return True


def _func_groups(claims: list[Claim]) -> dict[tuple[int, str], list[Claim]]:
    """Source function claims grouped by the body they name."""
    groups: dict[tuple[int, str], list[Claim]] = {}
    for c in claims:
        if c.kind == "func" and c.channel in ("src", "src_compgen"):
            groups.setdefault((c.rva, c.name), []).append(c)
    return groups


def _emitted():
    """`(unit, name) -> did cl emit this body`, or None when the unit has no
    object to ask. Masked on both sides: cl stamps its per-object CodeView
    counter onto a TU-local symbol and our names never carry one."""
    from homm1.core.coff import Coff
    from homm1.core.msvc_names import mask
    from homm1.core.paths import BUILD as _B

    objs: dict[str, set[str] | None] = {}

    def defines(unit: str, name: str) -> bool | None:
        if unit not in objs:
            obj = _B / "objdiff/base" / f"{unit}.obj"
            try:
                objs[unit] = {mask(n) for n in Coff(obj).code_names()} \
                    if obj.is_file() else None
            except ValueError:
                objs[unit] = None
        return None if objs[unit] is None else mask(name) in objs[unit]
    return defines


def unmaterialized(claims: list[Claim]) -> list[Claim]:
    """Function claims NO claiming unit's object defines, and nothing else
    names the rva.

    Every VA() names a body retail emitted, so a claim with no body anywhere
    is a reconstruction gap - typically a header inline no TU odr-uses, which
    cl therefore never emits. A rva ANOTHER source claim spells differently is
    not that gap: the two pins compete for one body, the materialized one wins
    the binding and the loser is recorded as its alias (the pre-canonical
    pipeline forgave exactly this case, by the same test). Derived, never a
    hand-kept list; the selftest's corpus control reads it to tell a missing
    body apart from a name-spelling defect. A unit with no object on disk
    cannot adjudicate and its group is skipped.
    """
    from collections import Counter

    defines = _emitted()
    groups = _func_groups(claims)
    spellings = Counter(rva for rva, _name in groups)
    out = []
    for (rva, _name), cs in groups.items():
        if spellings[rva] > 1:                  # contested: the alias records it
            continue
        verdicts = [defines(c.unit, c.name) for c in cs]
        if None not in verdicts and not any(verdicts):
            out.append(cs[0])
    return out


def _materialized(claims: list[Claim], violations: list[str]) -> list[Claim]:
    """Keep only the units that MATERIALIZED a function claim.

    A header inline's `VA()` reaches every including TU, so extraction - a
    pure function of source - claims it from all of them. Which TUs actually
    hold a body is not a source fact: cl emits the COMDAT only where the
    inline is odr-used, and the retail linker picked one of exactly those
    copies. So the emitting set is read here, from the units' own objects,
    and it decides both the owner and the `also_units` roll-up; the delink
    partition needs a unit whose object can be compared against the body it
    is given. Data claims are main-file-only (a `DATA()` in a header is a
    FATAL of its own) and never multi-unit, so only functions are filtered.

    A claim no object defines survives unfiltered - the label is real evidence
    about retail either way - and is reported once as the modelling gap it is.

    WHY HERE and not in delink/pdb_synth (asked and settled): `unit` is read
    by sema, walls, verify and the exe-map as well as the delinker, and all of
    them take a Model - deciding the owner downstream would leave every other
    consumer looking at the alphabetical tie-break. Ownership of a multiply
    emitted inline is inherently a materialization fact, so SOME object read
    is unavoidable; this is the module whose docstring already says policy
    lives here. Unlike the per-claim authority check it replaced, it only
    breaks a TIE - a missing object degrades attribution, never a label.
    """
    defines = _emitted()
    drop: set[int] = set()
    for cs in _func_groups(claims).values():
        if len(cs) == 1:
            continue
        keep = {id(c) for c in cs if defines(c.unit, c.name)}
        if keep:
            drop.update(id(c) for c in cs if id(c) not in keep)
    gaps = unmaterialized(claims)
    if gaps:
        first = min(f"{c.name} at 0x{c.rva:06x}" for c in gaps)
        violations.append(
            f"{len(gaps)} function claim(s) have no body in ANY claiming "
            f"unit's object (first: {first}) - the reconstruction is missing "
            f"the definition retail emitted")
    return [c for c in claims if id(c) not in drop]


def _disambiguate(data: list[Binding], violations: list[str]) -> list[Binding]:
    """One name per address image-wide, for the `$S` family.

    Several translation units can hold a same-named TU-local static (cl tells
    them apart with its per-object CodeView counter, which is exactly the
    volatile number our names drop). The delink data manifest and the synth
    PDB need one name per address, so a canonical name that binds more than
    one rva takes the rva into the ordinal slot cl's counter occupied - the
    `$T<rva>` convention, and `msvc_names.mask` folds it straight back onto
    the shared family, so compare still pairs the copies by content. A
    collision the family cannot respell stays a violation."""
    from homm1.core.msvc_names import discriminate
    from collections import Counter

    rvas: dict[str, set[int]] = {}
    for b in data:
        if b.name:
            rvas.setdefault(b.name, set()).add(b.rva)
    shared = {n for n, r in rvas.items() if len(r) > 1}
    if not shared:
        return data
    out, stuck = [], Counter()
    for b in data:
        if b.name in shared:
            spelling = discriminate(b.name, b.rva)
            if spelling is None:
                stuck[b.name] += 1
            else:
                b = b._replace(name=spelling)
        out.append(b)
    if stuck:
        first = min(stuck)
        violations.append(
            f"{len(stuck)} data name(s) bind at multiple rvas and have no "
            f"per-rva spelling (first: {first} x{stuck[first]})")
    return out


def _data_expected_kind(claim: Claim) -> str | None:
    if claim.channel == "data_compgen":
        return claim.meta.get("class")            # 'common' | 'copy'
    if claim.channel == "src_data_compgen":
        # the pinned VALUE decides the storage cl generated: a float constant
        # is an FP-pool slot, a narrow string literal a pooled `??_C@` datum
        return "fppool" if claim.name.startswith("$T") else "string"
    if claim.channel == "data_static_libs":
        if claim.name.startswith("??_7"):
            return "vtable"
        if claim.name.startswith("??_R"):
            return "rtti"
        return None
    return _DATA_KIND.get(claim.channel)


def _band_owner_fn():
    """rva -> owning unit per link_order.tsv's contribution bands, or None."""
    import bisect
    try:
        bands = censuses.link_order_bands()
    except Exception:
        return lambda rva: None
    los = [lo for lo, _hi, _u in bands]

    def owner(rva: int):
        i = bisect.bisect_right(los, rva) - 1
        if i >= 0 and bands[i][0] <= rva < bands[i][1]:
            return bands[i][2]
        return None
    return owner


def resolve() -> Model:
    violations: list[str] = []

    fn_rows = {r["rva"]: r for r in censuses.functions()}
    dt_rows = {r["rva"]: r for r in censuses.data()}

    all_claims = [c for c in providers.all_claims() + src_claims.all_claims()
                  if _active(c)]
    unknown = [c for c in all_claims if c.channel not in _PRECEDENCE]
    if unknown:
        violations.append(f"{len(unknown)} claim(s) from unknown channel "
                          f"{unknown[0].channel!r} - skipped")
        all_claims = [c for c in all_claims if c.channel in _PRECEDENCE]
    all_claims = _materialized(all_claims, violations)
    # A dyninit pin's OWNER is not a symbol cl emits (the body is a volatile
    # _$E<n>); the binding stays unnamed like the old spine, the owner rides
    # as an alias so audits keep it. Keyword owners are a source-hygiene wart,
    # reported ONCE.
    kw_owners = sum(1 for c in all_claims
                    if c.channel == "src_dyninit" and c.name in ("int", "char"))
    if kw_owners:
        violations.append(f"{kw_owners} RVA_DYNINIT pin(s) spell a KEYWORD as "
                          f"the owner ('int') - name the real owning datum")
    # A header-inline definition carries its VA()/DATA() macro into EVERY
    # including TU, so identical (kind, rva, channel, name) claims arrive from
    # several units. The OWNER is the unit whose retail link band contains the
    # rva (link_order.tsv is the authority); alphabetical only as fallback.
    band_owner = _band_owner_fn()
    merged: dict[tuple, Claim] = {}
    for c in sorted(all_claims, key=lambda c: (c.kind, c.rva, c.channel,
                                               c.name, c.unit)):
        key = (c.kind, c.rva, c.channel, c.name)
        prev = merged.get(key)
        if prev is None:
            merged[key] = c
        elif c.unit and c.unit != prev.unit:
            prev.meta.setdefault("also_units", []).append(c.unit)
    for key, c in merged.items():
        also = c.meta.get("also_units")
        if also:
            owner = band_owner(c.rva)
            if owner and owner != c.unit and owner in also:
                also.remove(owner)
                also.append(c.unit)
                merged[key] = c._replace(unit=owner)
    per_rva: dict[tuple[str, int], list[Claim]] = {}
    for c in merged.values():
        per_rva.setdefault((c.kind, c.rva), []).append(c)

    def pick(cands: list[Claim]) -> tuple[Claim, list[Claim]]:
        ordered = sorted(cands, key=lambda c: _PRECEDENCE.index(c.channel))
        return ordered[0], ordered[1:]

    functions: list[Binding] = []
    for rva, row in sorted(fn_rows.items()):
        cands = per_rva.pop(("func", rva), [])
        if not cands:
            functions.append(Binding(rva, row["size"], row["kind"], "text",
                                     "", "", "", ()))
            continue
        win, rest = pick(cands)
        if win.channel == "src_dyninit":
            rest = [win] + list(rest)          # keep the owner pin visible
            win = win._replace(name="")
        allowed = _FUNC_KINDS.get(win.channel, {""})
        if row["kind"] not in allowed:
            violations.append(
                f"func claim {win.name} ({win.channel}) binds kind="
                f"{row['kind']!r} row 0x{rva:06x} (allowed {sorted(allowed)})")
        size = row["size"]
        if win.channel in _SIZE_AUTHORITY and win.size:
            if win.size > row["size"]:
                violations.append(
                    f"claim {win.name} at 0x{rva:06x} size 0x{win.size:x} "
                    f"crosses the next admitted start 0x{rva + row['size']:06x}"
                    f" (derived extent 0x{row['size']:x})")
            else:
                size = win.size
        functions.append(Binding(rva, size, row["kind"], "text",
                                 win.name, win.unit, win.channel, tuple(rest),
                                 tuple(win.meta.get("also_units", ()))))

    data: list[Binding] = []
    for rva, row in sorted(dt_rows.items()):
        cands = per_rva.pop(("data", rva), [])
        if not cands:
            data.append(Binding(rva, row["size"], row["kind"], row["region"],
                                "", "", "", ()))
            continue
        win, rest = pick(cands)
        expected = _data_expected_kind(win)
        if expected is not None and row["kind"] != expected:
            violations.append(
                f"data claim {win.name} ({win.channel}) expects kind="
                f"{expected!r} but census row 0x{rva:06x} is {row['kind']!r}")
        elif expected is None and row["kind"] in ("pad", "ehtable"):
            violations.append(
                f"data claim {win.name} ({win.channel}) binds bookkeeping "
                f"kind={row['kind']!r} row 0x{rva:06x}")
        if win.channel == "src" and row["kind"] in ("copy", "common"):
            # a compiler-generated row outranked by a source spelling is the
            # phantom-shadow signature (a DATA() pin re-inventing a guard byte
            # or header-static copy the compgen manifest already owns)
            shadowed = next((c for c in rest if c.channel == "data_compgen"
                             and c.name != win.name), None)
            if shadowed is not None:
                violations.append(
                    f"src data claim {win.name} shadows the data_compgen "
                    f"identity {shadowed.name} on {row['kind']!r} row "
                    f"0x{rva:06x} - a phantom view of compiler-owned storage")
        size = row["size"]
        if win.channel in _SIZE_AUTHORITY and win.size:
            if win.size > row["size"]:
                violations.append(
                    f"data claim {win.name} at 0x{rva:06x} size 0x{win.size:x}"
                    f" crosses the next admitted start 0x{rva + row['size']:06x}"
                    f" (derived extent 0x{row['size']:x})")
            else:
                size = win.size
        data.append(Binding(rva, size, row["kind"], row["region"],
                            win.name, win.unit, win.channel, tuple(rest),
                            tuple(win.meta.get("also_units", ()))))

    # the delink data manifest needs ONE name -> ONE extent image-wide
    data = _disambiguate(data, violations)
    from collections import Counter
    dup = Counter(b.name for b in data if b.name)
    dups = {n: k for n, k in dup.items() if k > 1}
    if dups:
        first = next(iter(sorted(dups)))
        violations.append(
            f"{len(dups)} data name(s) bind at multiple rvas (first: {first} "
            f"x{dups[first]}) - per-rva alias spellings needed")

    # claims that hit no census row at all
    for (kind, rva), cands in sorted(per_rva.items()):
        for c in cands:
            violations.append(
                f"{kind} claim {c.name} ({c.channel}) at 0x{rva:06x} is not "
                f"an admitted census row")

    return Model(functions, data, violations)


def serialize(model: Model) -> tuple[bool, bool]:
    """Write bindings.tsv (the delink key) + violations.tsv, write-if-changed."""
    header = ["rva", "size", "kind", "space", "name", "unit", "channel",
              "also_units", "aliases"]

    def alias(a):
        # `|` separates the four fields; `:` appears inside C++ names (`::`)
        return f"{a.channel}|{a.name}|0x{a.size or 0:x}|{a.unit}"

    rows = []
    for b in model.functions + model.data:
        rows.append([f"0x{b.rva:08x}", f"0x{b.size:x}", b.kind, b.space,
                     b.name, b.unit, b.channel, ";".join(b.also),
                     ";".join(alias(a) for a in b.aliases)])
    changed_b = write_tsv(BINDINGS, ["# GENERATED by homm1.model - the "
                                     "resolved claim set (the delink key)."],
                          header, rows)
    changed_v = write_tsv(VIOLATIONS, ["# GENERATED by homm1.model."],
                          ["violation"], [[v] for v in model.violations])
    return changed_b, changed_v


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    import argparse
    from collections import Counter

    # Parse even though there are no options: `homm1 model --help` used to
    # run the whole join and REWRITE bindings.tsv (help as a side effect), and
    # a typo'd flag was accepted in silence.
    argparse.ArgumentParser(
        prog="homm1 model", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter).parse_args(argv)
    model = resolve()
    changed_b, _ = serialize(model)
    fn_ch = Counter(b.channel or "(unclaimed)" for b in model.functions)
    dt_ch = Counter(b.channel or "(unclaimed)" for b in model.data)
    print("functions:", dict(fn_ch.most_common()))
    print("data:     ", dict(dt_ch.most_common()))
    print(f"violations: {len(model.violations)}"
          + (f" (first: {model.violations[0]})" if model.violations else ""))
    print(f"bindings.tsv {'UPDATED' if changed_b else 'unchanged'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
