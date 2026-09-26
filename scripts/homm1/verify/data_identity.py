"""homm1.verify.data_identity - one retail datum, one source symbol (normal).

While `data_matching = false` (config/compare.toml) objdiff never sees a data
relocation's target: `homm1.compare.normalize.relax_data_relocations` sends
every one to `$data` on both sides. Nothing then notices two units naming ONE
retail global through two different externs, one extern reaching two retail
globals, or a DATA() claim that its own references contradict. This gate
reads the identities the relaxation hides, BEFORE it runs, so it works the
same in both modes:

  1. Both sides of every live unit are canonicalized (never relaxed): the raw
     base obj (`build/objdiff/base`) and the delinked target
     (`build/objdiff/target-new`).
  2. A function is paired when its RELOCATION LAYOUT lines up: the same count,
     offsets and types, and every target on the same side of the relaxation
     line (`normalize.is_relaxed`). Byte-identical bodies are a subset; a body
     that differs only in instructions without a relocation (a register
     choice, a spill) still carries every datum at the same operand, so it is
     evidence too. Within it, a relaxed relocation pairs only when the two
     bytes ahead of the operand (opcode/ModRM, other operands masked) agree
     as well: the SAME instruction reads it.
  3. Each pair states (base symbol, base addend) -> the retail rva the operand
     holds (the image word at the function's Model rva + offset; the delinked
     target's own symbol decomposes that same word). Its object base is the
     rva less the base addend.

Findings, FAILING in both modes (identity bugs, not debt):

  (a) one retail object base reached through two source symbols (identical
      string/FP literals the linker pooled are one identity);
  (b) one source symbol reaching two retail object bases, except compiler
      string literals whose bytes match every referenced retail copy;
  (c) a symbol a Model claim binds at one rva whose references reach another.

Not a finding: an evaluation-order difference - within one function, the
sites that miss their symbol's usual base (its claim, else its majority)
land exactly on those same symbols' usual addresses including addends: a swap of `a + b`, a
rotation of hoisted loop invariants. The names are right; only cl's load
order differs (counted, never failing).

Consistent placeholder externs are fine: the derived map is written to
`build/gen/data_identity.tsv` (symbol, rva, sites, units, status), which
states each placeholder extern's retail address for the re-enable step
(docs/build-system.md, "Data matching").

    python3 -m homm1.verify.data_identity [--list]
"""

from __future__ import annotations

from homm1.core.usage import logged

import struct
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

from homm1.compare import canonicalize as canon
from homm1.compare import normalize
from homm1.core.msvc_names import mask
from homm1.core.paths import BUILD
from homm1.walls.pairscan import is_local_label

TARGET = BUILD / "objdiff/target-new"
TSV = BUILD / "gen/data_identity.tsv"

_CODE = canon.MEM_EXECUTE | 0x20
_DIR32NB = 0x0007
_STATIC = 3
_DT_FUNCTION = 2
#: Bytes ahead of a relaxed operand that must agree for it to pair: the
#: opcode/ModRM of the instruction that holds it.
OPCODE_WINDOW = 2
#: The canonical prefix of cl's unnamed literals (`$SG` strings, `$T`
#: constants): `$anon_<kind>_<content digest>_<occurrence>`.
_LITERAL = "$anon_"
_SOURCE_CHANNELS = ("src", "src_compgen", "src_dyninit", "src_data_compgen")


@dataclass(frozen=True)
class Site:
    unit: str
    function: str
    offset: int
    key: str        # identity: the name, or `unit:name` for a TU-local static
    symbol: str     # the source spelling (cl ordinals masked)
    local: bool     # a TU-local static: its key and its claims are per unit
    addend: int
    retail: int     # the rva the operand holds
    target: str     # the delinked target's symbol + addend, for the report
    verified_literal: bool = False  # source string bytes equal this retail copy

    @property
    def base(self) -> int:
        return (self.retail - self.addend) & 0xFFFFFFFF

    @property
    def literal(self) -> bool:
        return self.key.split(":", 1)[-1].startswith(_LITERAL)

    def content(self) -> str:
        """The (a)-identity: a pooled literal is its CONTENT (the canonical
        name less unit and occurrence), anything else its key."""
        if self.literal:
            return self.key.split(":", 1)[-1].rsplit("_", 1)[0]
        return self.key

    def where(self) -> str:
        return f"{self.unit}:{self.function}+{self.offset:#x} ({self.target})"


def _canonical(payload: bytes) -> tuple[canon.CoffObject, dict[str, str]]:
    result = canon.canonicalize_coff(payload)
    spelled = {row.canonical_name: row.original_name for row in result.rows}
    return canon.CoffObject(result.data), spelled


def _windows(coff: canon.CoffObject) -> dict[str, tuple[canon.Section, int, int]]:
    """{function: (section, start, end)}; a window ends at the next defined
    non-label symbol of its code section."""
    out = {}
    for section in coff.sections:
        if not section.characteristics & _CODE:
            continue
        members = sorted(
            (s.value, s.index) for s in coff.symbols.values()
            if s.section == section.index and s.storage_class in (2, _STATIC)
            and not s.name.startswith(".") and not is_local_label(s.name))
        starts = sorted({value for value, _i in members})
        for value, index in members:
            symbol = coff.symbols[index]
            if (symbol.typ >> 4) & 0x3 != _DT_FUNCTION:
                continue
            later = [v for v in starts if v > value]
            out[symbol.name] = (section, value, later[0] if later else section.raw_size)
    return out


def _relocations(coff, section, lo, hi):
    """[(offset, relocation, relaxed?)] inside one window, in site order."""
    rows = [(r.site - lo, r, normalize.is_relaxed(coff, r))
            for r in coff.relocations
            if r.section == section.index and lo <= r.site < hi]
    return sorted(rows, key=lambda row: row[0])


def _masked(coff, section, lo, hi, relocs) -> bytearray:
    body = bytearray(coff.section_bytes(section)[lo:hi])
    for offset, relocation, _relaxed in relocs:
        width = canon.RELOCATION_WIDTHS.get(relocation.typ, 4)
        body[offset:offset + width] = bytes(width)
    return body


def _addend(coff, section, site: int) -> int:
    return struct.unpack_from("<I", coff.section_bytes(section), site)[0]


def unit_sites(unit: str, base_payload: bytes, target_payload: bytes,
               function_rva, retail_word, image_base: int, stats=None,
               retail_read=None):
    """[Site] for one unit's paired functions.

    `function_rva(unit, name)` is the body's retail rva (None: unknown), and
    `retail_word(rva)` the image's u32 there (None: unreadable)."""
    stats = stats if stats is not None else defaultdict(int)
    base, spelled = _canonical(base_payload)
    target, _ = _canonical(target_payload)
    base_windows, target_windows = _windows(base), _windows(target)
    sites = []
    for name in sorted(set(base_windows) & set(target_windows)):
        stats["functions"] += 1
        b_sec, b_lo, b_hi = base_windows[name]
        t_sec, t_lo, t_hi = target_windows[name]
        b_rel = _relocations(base, b_sec, b_lo, b_hi)
        t_rel = _relocations(target, t_sec, t_lo, t_hi)
        if [(o, r.typ, x) for o, r, x in b_rel] != \
                [(o, r.typ, x) for o, r, x in t_rel]:
            stats["functions with a different relocation layout"] += 1
            continue
        stats["functions paired"] += 1
        if not any(x for _o, _r, x in b_rel):
            continue
        rva = function_rva(unit, name)
        if rva is None:
            stats["paired functions with no Model rva"] += 1
            continue
        b_body = _masked(base, b_sec, b_lo, b_hi, b_rel)
        t_body = _masked(target, t_sec, t_lo, t_hi, t_rel)
        for (offset, b_r, relaxed), (_o, t_r, _x) in zip(b_rel, t_rel):
            if not relaxed:
                continue
            lo = max(0, offset - OPCODE_WINDOW)
            if b_body[lo:offset] != t_body[lo:offset]:
                stats["sites under a different instruction"] += 1
                continue
            word = retail_word(rva + offset)
            if word is None:
                stats["sites with no retail word"] += 1
                continue
            retail = word if b_r.typ == _DIR32NB else (word - image_base) & 0xFFFFFFFF
            symbol = base.symbols[b_r.symbol_index]
            spelling = mask(spelled.get(symbol.name, symbol.name))
            local = symbol.storage_class == _STATIC
            t_sym = target.symbols[t_r.symbol_index].name
            t_add = _addend(target, t_sec, t_r.site)
            addend = _addend(base, b_sec, b_r.site)
            verified_literal = False
            original = spelled.get(symbol.name, symbol.name)
            if (retail_read is not None and symbol.section > 0
                    and original.startswith(("??_C@_0", "$SG"))):
                section = base.sections[symbol.section - 1]
                payload = base.section_bytes(section)[symbol.value:]
                end = payload.find(b"\0")
                if end >= 0:
                    literal = payload[:end + 1]
                    verified_literal = retail_read(
                        (retail - addend) & 0xFFFFFFFF, len(literal)) == literal
            sites.append(Site(
                unit, name, offset,
                f"{unit}:{symbol.name}" if local else symbol.name,
                f"{unit}:{spelling}" if local else spelling, local,
                addend, retail,
                t_sym + (f"+{t_add:#x}" if t_add else ""), verified_literal))
            stats["sites"] += 1
    return sites


def _usual_bases(sites, claims) -> dict[str, int]:
    """{key: the base a symbol normally reaches}: its single Model claim when
    it has one, else the base most of its sites reach."""
    counts: dict[str, dict[int, int]] = defaultdict(lambda: defaultdict(int))
    first: dict[str, Site] = {}
    for site in sites:
        counts[site.key][site.base] += 1
        first.setdefault(site.key, site)
    usual = {}
    for key, bases in counts.items():
        site = first[key]
        spelling = site.symbol.split(":", 1)[-1]
        bound = {rva for unit, rva in claims.get(spelling, ())
                 if not site.local or unit == site.unit}
        usual[key] = next(iter(bound)) if len(bound) == 1 else \
            max(bases, key=lambda b: (bases[b], -b))
    return usual


def operand_swaps(sites, claims) -> set[Site]:
    """Sites of cl evaluation-order differences: within one function, the
    sites that miss their symbol's usual base (_usual_bases) land EXACTLY on
    the usual addresses including addends - a permutation (a swap of `a + b`,
    a rotation of hoisted loop invariants). Every name is right; only the
    order cl emits the loads in differs (TU state in an unedited function,
    spelling order in an edited one), so it is not an identity finding. A
    real split - a symbol landing on a base none of the misplaced symbols
    owns - is never such a permutation and still fails."""
    usual = _usual_bases(sites, claims)
    by_function: dict[tuple[str, str], list[Site]] = defaultdict(list)
    for site in sites:
        by_function[(site.unit, site.function)].append(site)
    permuted: set[Site] = set()
    for group in by_function.values():
        missed = [site for site in group if site.base != usual[site.key]]
        if len(missed) < 2:
            continue
        # Compare complete addresses, including member offsets. VC4 can swap
        # two members of ONE object just as it can swap two scalar globals.
        # Subtracting each candidate addend before pairing invents two object
        # bases for e.g. image.width * image.height emitted in reverse order.
        # Exact multiset equality still rejects missing/duplicated members.
        if sorted(site.retail for site in missed) == \
                sorted((usual[site.key] + site.addend) & 0xFFFFFFFF
                       for site in missed):
            permuted.update(missed)
    return permuted


def conflicts(sites, claims) -> list[str]:
    """(a)/(b)/(c) over every unit's sites. `claims` is
    {source spelling: {(unit, rva)}} from the Model. Operand-order swaps
    (operand_swaps) are TU state and excluded."""
    swapped = operand_swaps(sites, claims)
    sites = [site for site in sites if site not in swapped]
    by_base: dict[int, dict[str, list[Site]]] = defaultdict(lambda: defaultdict(list))
    by_key: dict[str, dict[int, list[Site]]] = defaultdict(lambda: defaultdict(list))
    for site in sites:
        by_base[site.base][site.content()].append(site)
        by_key[site.key][site.base].append(site)
    out = []
    for rva, names in sorted(by_base.items()):
        if len(names) > 1:
            out.append(
                f"data-identity: retail {rva:#08x} is reached through "
                f"{len(names)} source symbols - "
                + "; ".join(f"{group[0].symbol} at {group[0].where()}"
                            for group in sorted(names.values(),
                                                key=lambda g: g[0].symbol))
                + " - one storage, one declaration")
    for key, bases in sorted(by_key.items()):
        # Compiler string pooling can differ between the two builds. Accept
        # multiple copies only after proving every copy's bytes; ordinary
        # named storage and contradictory explicit claims remain errors.
        if len(bases) > 1 and not all(
                s.verified_literal for group in bases.values() for s in group):
            first = next(iter(bases.values()))[0]
            out.append(
                f"data-identity: {first.symbol} reaches {len(bases)} retail "
                f"bases - " + "; ".join(f"{rva:#08x} at {group[0].where()}"
                                        for rva, group in sorted(bases.items())))
        for rva, group in sorted(bases.items()):
            site = group[0]
            spelling = site.symbol.split(":", 1)[-1]
            bound = {c_rva for c_unit, c_rva in claims.get(spelling, ())
                     if not site.local or c_unit == site.unit}
            if bound and rva not in bound:
                out.append(
                    f"data-identity: {site.symbol} is claimed at "
                    + ", ".join(f"{b:#08x}" for b in sorted(bound))
                    + f" but its references reach {rva:#08x} at {site.where()}")
    return out


def identity_rows(sites, statuses) -> list[tuple[str, int, int, str, str]]:
    """[(symbol, rva, sites, units, status)] - one row per symbol and base.
    `statuses` maps a site key to its status word."""
    grouped: dict[tuple[str, int], list[Site]] = defaultdict(list)
    for site in sites:
        grouped[(site.key, site.base)].append(site)
    rows = []
    for (key, rva), group in grouped.items():
        rows.append((group[0].symbol, rva, len(group),
                     ",".join(sorted({s.unit for s in group})),
                     statuses.get(key, "unclaimed")))
    return sorted(rows, key=lambda row: (row[1], row[0]))


def write_tsv(rows, path: Path = TSV) -> None:
    text = ("# data identity (homm1.verify.data_identity): each data symbol a "
            "paired function references -> the retail rva its references "
            "reach (object base). Derived every run, never hand-kept.\n"
            "# status: claimed (a source DATA() claim), provided (a retail "
            "provider table), placeholder (an extern with no definition or "
            "claim yet: define it at this rva), literal, unclaimed.\n"
            "symbol\trva\tsites\tunits\tstatus\n"
            + "".join(f"{s}\t0x{r:06x}\t{n}\t{u}\t{st}\n"
                      for s, r, n, u, st in rows))
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(".tmp")
    tmp.write_text(text)
    tmp.replace(path)


# --------------------------------------------------------------------------- #
# the real inputs                                                             #
# --------------------------------------------------------------------------- #
def _model_inputs():
    """(function_rva(unit, name), claims, {spelling: channel})."""
    from homm1.model import resolve
    model = resolve()
    by_unit, by_name = {}, defaultdict(set)
    for b in model.functions:
        if b.name:
            by_unit[(b.unit, b.name)] = b.rva
            by_name[b.name].add(b.rva)

    def function_rva(unit, name):
        if (unit, name) in by_unit:
            return by_unit[(unit, name)]
        rvas = by_name.get(name, ())
        return next(iter(rvas)) if len(rvas) == 1 else None

    claims, channels = defaultdict(set), {}
    for b in model.data:
        for claim in (b, *b.aliases):
            if claim.name and claim.channel:
                claims[claim.name].add((b.unit, b.rva))
                channels.setdefault(claim.name, claim.channel)
    return function_rva, claims, channels


def scan(stats=None):
    """([Site], claims, statuses) over every live unit with a target."""
    from homm1.sema.image import retail
    from homm1.verify.undefined_closure import BASE, live_base_objs, placeholder_externs
    stats = stats if stats is not None else defaultdict(int)
    image = retail()
    function_rva, claims, channels = _model_inputs()
    sites = []
    for base in live_base_objs():
        unit = base.relative_to(BASE).with_suffix("").as_posix()
        target = normalize.target_object(TARGET, unit)
        if target is None:
            stats["units with no target"] += 1
            continue
        stats["units"] += 1
        sites += unit_sites(unit, base.read_bytes(), target.read_bytes(),
                            function_rva, image.u32, image.base, stats, image.read)
    placeholders = placeholder_externs()
    stats["placeholder externs no paired site reaches"] = len(
        set(placeholders) - {site.key for site in sites})
    statuses = {}
    for site in sites:
        spelling = site.symbol.split(":", 1)[-1]
        channel = channels.get(spelling)
        statuses[site.key] = (
            ("claimed" if channel in _SOURCE_CHANNELS else "provided")
            if channel else "placeholder" if site.key in placeholders
            else "literal" if site.literal or site.verified_literal else "unclaimed")
    return sites, claims, statuses


def gate_findings(stats=None) -> list[str]:
    from homm1.verify.undefined_closure import BASE
    if not BASE.is_dir() or not any(BASE.rglob("*.obj")):
        return ["data-identity: no base objs - run `homm1 build` first "
                "(never vacuous)"]
    stats = stats if stats is not None else defaultdict(int)
    sites, claims, statuses = scan(stats)
    if not stats["functions paired"]:
        return ["data-identity: no function paired with its delinked target "
                f"under {TARGET} - run `homm1 build` first (never vacuous)"]
    write_tsv(identity_rows(sites, statuses))
    stats["evaluation-order sites (not failing)"] = len(operand_swaps(sites, claims))
    return conflicts(sites, claims)


@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 verify data-identity",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--list", action="store_true",
                    help="print the derived symbol -> retail rva map")
    a = ap.parse_args(argv)
    stats: dict = defaultdict(int)
    bad = gate_findings(stats)
    print("data-identity: " + ", ".join(f"{k}={v}" for k, v in stats.items())
          + f"; map in {TSV.relative_to(BUILD.parent)}")
    if a.list and TSV.is_file():
        print(TSV.read_text(), end="")
    for line in bad:
        print("  " + line, file=sys.stderr)
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
