"""homm1.verify.tu_order - the linker-layout acceptance gate (normal tier).

link.exe 5.10 lays each .obj's .text down as ONE CONTIGUOUS block, input
sections in cl's emission order (== file order), objs in link-line order
(docs/link-text-layout.md). A faithful reconstruction therefore satisfies:

  INTRA-TU  VA() functions appear in FILE ORDER strictly increasing in
            retail RVA, spans non-overlapping;
  INTER-TU  each TU's [min_start, max_end) block never interleaves another's.

The one legitimate exception: kept-COMDAT exiles
(config/cleanliness/kept-comdat-exiles.tsv), re-PROVEN every run - a row
whose rva is no longer pinned/emitted by its owner, or that left its host's
span (+/- seam slack), FAILS. Ratcheted against
config/cleanliness/tu-order-baseline.tsv: a rise fails; the gate never
writes the baseline (bless with --update).

    python3 -m homm1.verify.tu_order [--gate] [--tu NAME] [--update]
"""

from __future__ import annotations

import argparse
import re
import sys

from homm1.core.paths import BUILD, CONFIG, REPO, SRC
from homm1.core.tsv import read as read_tsv
from homm1.verify.srcscan import VA_RE, claim_rva

EXILES_TSV = CONFIG / "cleanliness/kept-comdat-exiles.tsv"
BASELINE = CONFIG / "cleanliness/tu-order-baseline.tsv"
CLAIMS = BUILD / "gen/claims"

SIG_RE = re.compile(r"([A-Za-z_]\w*)::(~?[A-Za-z_]\w*|operator[^\(]*)")
#: shared special-member pool bands (ctors/dtors linker-pooled across classes)
POOLS = ((0x0F400, 0x13600), (0x80000, 0x90000))
SEAM_SLACK = 0x1800
OUTLIER_GAP = 0x8000
MAX_OUTLIERS = 3


def pooled(rva: int) -> bool:
    return any(lo <= rva < hi for lo, hi in POOLS)


class Entry:
    __slots__ = ("rva", "size", "line", "name", "tu")

    def __init__(self, rva, size, line, name, tu):
        self.rva, self.size, self.line = rva, size, line
        self.name, self.tu = name, tu

    @property
    def end(self) -> int:
        return self.rva + self.size


def load_exiles() -> dict[int, tuple[str, str, str]]:
    out = {}
    if EXILES_TSV.is_file():
        for ln in EXILES_TSV.read_text().splitlines():
            if not ln.strip() or ln.startswith("#"):
                continue
            f = ln.split("\t")
            if len(f) >= 4:
                out[int(f[0], 16)] = (f[1], f[2], f[3])
    return out


def load_emitted_claims() -> dict[int, set[str]]:
    """rva -> units whose generated claim fragment carries that function.

    A header-inline RVA pin is absent from the .cpp scan but present in every
    emitting unit's fragment, which is what lets the exile gate verify the
    OWNER's copy without mistaking the selected host copy for ownership."""
    out: dict[int, set[str]] = {}
    if not CLAIMS.is_dir():
        return out
    for path in CLAIMS.rglob("*.tsv"):
        try:
            _b, _h, rows = read_tsv(path)
        except (OSError, ValueError):
            continue
        for r in rows:
            if r.get("kind") != "func" or not r.get("rva"):
                continue
            unit = path.relative_to(CLAIMS).with_suffix("").as_posix()
            out.setdefault(int(r["rva"], 16), set()).add(unit.casefold())
    return out


def load_in_file_order(exclude_pools: bool = False) -> dict[str, list[Entry]]:
    tus: dict[str, list[Entry]] = {}
    for path in sorted(SRC.rglob("*.cpp")):
        tu = path.stem
        rel = path.relative_to(REPO)
        lines = path.read_text(errors="replace").splitlines()
        seq: list[Entry] = []
        for i, ln in enumerate(lines):
            m = VA_RE.search(ln)
            if not m:
                continue
            rva = claim_rva(m)
            s = m.group(2)
            size = int(s, 16) if s.lower().startswith("0x") else int(s)
            if exclude_pools and pooled(rva):
                continue
            name = None
            for j in range(i, min(i + 4, len(lines))):
                sm = SIG_RE.search(lines[j])
                if sm:
                    name = f"{sm.group(1)}::{sm.group(2)}"
                    break
            seq.append(Entry(rva, size, i + 1, name or "?", str(rel)))
        if seq:
            tus[tu] = seq
    return tus


def split_exiles(tus, exiles):
    claimed = {e.rva: tu for tu, seq in tus.items() for e in seq}
    if not exiles:
        return tus, claimed, 0
    clean, dropped = {}, 0
    for tu, seq in tus.items():
        keep = [e for e in seq if e.rva not in exiles]
        dropped += len(seq) - len(keep)
        if keep:
            clean[tu] = keep
    return clean, claimed, dropped


def verify_exiles(exiles, claimed, spans, emitted) -> list[str]:
    bad = []
    for rva, (owner, host, name) in sorted(exiles.items()):
        got = claimed.get(rva)
        owner_emits = owner.casefold() in emitted.get(rva, set())
        if got is None and not owner_emits:
            bad.append(f"exile {rva:#010x} {name}: no owner VA() pin/emission "
                       f"found (owner {owner})")
        elif got is not None and got != owner and not owner_emits:
            bad.append(f"exile {rva:#010x} {name}: pinned in {got}, ledger "
                       f"says {owner}")
        sp = spans.get(host)
        if sp is None:
            bad.append(f"exile {rva:#010x} {name}: host unit {host} has no span")
        elif not (sp[0] - SEAM_SLACK <= rva < sp[1] + SEAM_SLACK):
            bad.append(f"exile {rva:#010x} {name}: outside host {host} "
                       f"[{sp[0]:#x}-{sp[1]:#x}] +/-{SEAM_SLACK:#x}")
    return bad


def check_intra(tus):
    viol = {}
    for tu, seq in tus.items():
        vs = []
        for a, b in zip(seq, seq[1:]):
            if b.rva <= a.rva:
                vs.append(f"  L{a.line} {a.rva:#08x} {a.name}  ->  "
                          f"L{b.line} {b.rva:#08x} {b.name}   "
                          f"[file order not ascending]")
            elif a.size and a.end > b.rva:
                vs.append(f"  L{a.line} {a.rva:#08x}+{a.size:#x}={a.end:#08x} "
                          f"{a.name}  overlaps  L{b.line} {b.rva:#08x} {b.name}")
        if vs:
            viol[tu] = vs
    return viol


def tu_cluster(seq):
    """(dense run, outliers): peel small remote groups (<= MAX_OUTLIERS) that
    sit >= OUTLIER_GAP from the run - one stray manufactures a pair with every
    unit in between."""
    ent = sorted((e for e in seq if not pooled(e.rva)), key=lambda e: e.rva)
    outliers = []
    while len(ent) > 1:
        rvas = [e.rva for e in ent]
        gap, i = max((rvas[k + 1] - rvas[k], k) for k in range(len(rvas) - 1))
        if gap < OUTLIER_GAP:
            break
        lo, hi = ent[:i + 1], ent[i + 1:]
        drop = lo if len(lo) <= len(hi) else hi
        if len(drop) > MAX_OUTLIERS:
            break
        outliers += drop
        ent = [e for e in ent if e not in drop]
    return ent, outliers


def tu_spans(tus):
    spans = {}
    for tu, seq in tus.items():
        own, _strays = tu_cluster(seq)
        if not own:
            continue
        starts = [e.rva for e in own]
        ends = [e.end for e in own if e.size]
        spans[tu] = (min(starts), max(ends) if ends else max(starts))
    return spans


def check_inter(tus):
    spans = tu_spans(tus)
    ordered = sorted(spans.items(), key=lambda kv: kv[1][0])
    out = []
    for i in range(len(ordered)):
        ta, (sa, ea) = ordered[i]
        for j in range(i + 1, len(ordered)):
            tb, (sb, eb) = ordered[j]
            if sb >= ea:
                break
            if sa < eb and sb < ea:
                out.append((ta, (sa, ea), tb, (sb, eb)))
    return out


def _load_baseline():
    base_tu, base_pairs = {}, None
    if BASELINE.is_file():
        for ln in BASELINE.read_text().splitlines():
            if not ln.strip() or ln.startswith("#"):
                continue
            k, _, v = ln.partition("\t")
            if k == "(interleave-pairs)":
                base_pairs = int(v)
            else:
                base_tu[k] = int(v)
    return base_tu, base_pairs


def _write_baseline(cur, pairs):
    rows = [f"{tu}\t{n}" for tu, n in sorted(cur.items())]
    rows.append(f"(interleave-pairs)\t{pairs}")
    BASELINE.write_text("\n".join(rows) + "\n")


def gate_findings():
    """(findings, summary) - the ratchet vs the committed baseline. Never
    writes; --update is the manual bless."""
    tus = load_in_file_order()
    exiles = load_exiles()
    tus, claimed, n_exiled = split_exiles(tus, exiles)
    bad = verify_exiles(exiles, claimed, tu_spans(tus), load_emitted_claims())
    findings = [f"tu-order EXILE LEDGER STALE: {b}" for b in bad]
    if not sum(len(v) for v in tus.values()):
        # No labelled bodies means no order to violate: OK here would mean
        # "the layout theorem holds" on the strength of an empty scan.
        findings.append("tu-order: 0 RVA-labelled function(s) found under "
                        "src/ - nothing was ordered, so 0 violations is "
                        "vacuous, not a pass. Check src/ and re-run.")
    intra = check_intra(tus)
    inter = check_inter(tus)
    cur = {tu: len(v) for tu, v in intra.items()}
    pairs = len(inter)
    base_tu, base_pairs = _load_baseline()
    if base_pairs is None:
        findings.append(f"tu-order: no committed baseline ({BASELINE}) - "
                        f"bless the backlog with --update")
        return findings, (cur, pairs, n_exiled, len(exiles))
    for tu, n in sorted(cur.items()):
        if n > base_tu.get(tu, 0):
            findings.append(f"tu-order RATCHET VIOLATED: {tu}  "
                            f"{base_tu.get(tu, 0)} -> {n} intra violation(s)")
    if pairs > base_pairs:
        findings.append(f"tu-order RATCHET VIOLATED: interleave pairs  "
                        f"{base_pairs} -> {pairs}")
    return findings, (cur, pairs, n_exiled, len(exiles))


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="homm1 verify tu-order",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 when a TU rises above its committed violation count")
    ap.add_argument("--tu",
                    help="explain one TU's ordering in detail (the .cpp stem, "
                         "matched case-insensitively: `grunt` == `Grunt`)")
    ap.add_argument("--update", action="store_true",
                    help="bless the current backlog into the baseline "
                         "(manual; only ever roll floors DOWN)")
    a = ap.parse_args(argv)

    if a.tu:
        tus = load_in_file_order()
        exiles = load_exiles()
        # this key is the .cpp STEM ("Grunt"), while every other --unit flag
        # in the toolchain takes the lowercase unit ("grunt"): accept either
        # rather than answering a correct unit name with "no such TU".
        name = a.tu
        if name not in tus:
            fold = {t.casefold(): t for t in tus}
            name = fold.get(a.tu.casefold(), a.tu)
        seq = tus.get(name)
        if not seq:
            near = sorted(t for t in tus if a.tu.casefold() in t.casefold())
            hint = f" (did you mean: {', '.join(near[:5])}?)" if near else ""
            print(f"no such TU: {a.tu}{hint} - the key is a src/**/<name>.cpp "
                  f"stem; `homm1 sema map units` lists them", file=sys.stderr)
            return 2
        print(f"{name}  ({len(seq)} functions, file order):")
        prev = None
        for e in seq:
            if e.rva in exiles:
                print(f"  L{e.line:<5} {e.rva:#08x} +{e.size:#06x} {e.name}  "
                      f"[kept-COMDAT exile -> {exiles[e.rva][1]}]")
                continue
            flag = "  <-- NOT ASCENDING" if prev and e.rva <= prev.rva else ""
            print(f"  L{e.line:<5} {e.rva:#08x} +{e.size:#06x} -> "
                  f"{e.end:#08x}  {e.name}{flag}")
            prev = e
        return 0

    findings, (cur, pairs, n_exiled, n_ledger) = gate_findings()
    if a.update:
        _write_baseline(cur, pairs)
        print(f"tu-order: baseline blessed - {len(cur)} TU(s) with intra "
              f"violations, {pairs} interleave pair(s)")
        return 0
    for f in findings:
        print(f, file=sys.stderr)
    if findings:
        print("fix the layout, never bless it up "
              "(`python3 -m homm1.verify.tu_order --tu <name>` for detail)",
              file=sys.stderr)
        return 1 if a.gate else 0
    print(f"tu-order: no new wiring defects; backlog {len(cur)} TU(s) / "
          f"{pairs} pair(s) (frozen in {BASELINE.name}); {n_ledger} "
          f"kept-COMDAT exile rows verified, {n_exiled} .cpp bodies excluded")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
