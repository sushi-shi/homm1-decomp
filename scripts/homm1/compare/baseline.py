"""Conservative progress reporting while strict whole-image delinking is blocked.

Uses the ordinary delinker, normalizer and objdiff comparison. Diagnostic
objects retain unknown relocation names; only functions whose identity and
reference sites have current-image evidence contribute to this lower bound.
Unscored census entries remain in the denominator. This never updates MAX.
"""
from __future__ import annotations

import hashlib
import json
import struct
from collections import Counter

from homm1.compare.canonicalize import CoffObject
from homm1.core.paths import BUILD, RETAIL
from homm1.core.tsv import read as read_tsv

ROOT = BUILD / "objdiff/baseline"


def walk(value):
    if isinstance(value, dict):
        yield value
        for child in value.values():
            yield from walk(child)
    elif isinstance(value, list):
        for child in value:
            yield from walk(child)


def evidence(paths, image_hash):
    """Read explicit reviewed identities/sites, never infer correspondence.

    The two reference schemas are existing retail-fact schemas. Conflicting
    spellings at a site are withheld. No archived NWC evidence is admitted.
    """
    functions, references = set(), {}
    for path in paths:
        doc = json.loads(path.read_text())
        if doc.get("image_sha256") != image_hash:
            continue
        for row in walk(doc):
            if all(k in row for k in ("name", "rva", "size")):
                functions.add((int(row["rva"], 16), row["name"]))
            if all(k in row for k in ("site_rva", "target_rva", "symbol")):
                site, target, name = row["site_rva"], row["target_rva"], row["symbol"]
            elif all(k in row for k in ("site", "target", "name", "kind")):
                site, target, name = row["site"], row["target"], row["name"]
            else:
                continue
            if not all(isinstance(v, str) for v in (site, target, name)):
                continue
            key = (int(site, 16), int(target, 16))
            references.setdefault(key, set()).add(name)
    return functions, references


def reference_reason(name, site, target, owner, size, proofs, known_referents):
    if name.startswith(("UNPROVISIONED_", "FUN_", "DAT_")):
        return "unprovided reference"
    if name in known_referents.get(target, set()):
        return ""
    if owner <= target < owner + size:
        return ""  # an in-function label; strict objdiff still checks its addend
    names = proofs.get((site, target), set())
    if names != {name}:
        return "unreviewed or conflicting reference"
    return ""


def summarize(report, target_dir, out_dir):
    from homm1.core.inputs import read_verified, targets
    from homm1.core.pe import image
    from homm1.delink import pdb_synth
    from homm1.model import resolve
    from homm1.retail_labels import censuses
    from homm1.verify.scores import functions, split_eh_band

    pin = targets()["game"]
    read_verified(pin, pin.destination)
    pe = image()
    model = resolve()
    # The Buka census already separates EH and pad starts. An inherited NWC
    # DNA partition must not silently remove bytes from the new denominator.
    banners, _, _ = read_tsv(RETAIL / "functions.tsv")
    if f"# image-sha256: {pin.sha256}" not in banners:
        raise ValueError("baseline requires an image-pinned structural census")
    bands = censuses.link_bands()
    if any(band != "crt" for _, _, band in bands):
        raise ValueError("baseline library exclusions need reviewed CRT bands")
    census = [r for r in censuses.functions() if not r["kind"]
              and not any(lo <= r["rva"] < hi for lo, hi, _ in bands)]
    reviewed, proofs = evidence(sorted(RETAIL.glob("buka-*.json")), pin.sha256)
    reviewed.update((rva, value[0]) for rva, value in
                    pdb_synth.referent_function_names().items())
    known_referents = {}
    iat, _ = pdb_synth.implib.resolve_iat(pdb_synth.retail().import_slots(),
                                       pdb_synth.BASE_DIR)
    for rva, name in iat:
        known_referents.setdefault(rva, set()).add(name)
    # A reviewed referent is valid at every use; a caller need not be reviewed
    # again just to reuse the same established identity.
    known_targets = {}
    for (_site, target), names in proofs.items():
        known_targets.setdefault(target, set()).update(names)
    for target, names in known_targets.items():
        if len(names) == 1:
            known_referents.setdefault(target, set()).update(names)
    named = pdb_synth.referent_function_names()
    for rva, (name, _unit, _size) in named.items():
        known_referents.setdefault(rva, set()).add(name)
    # These literal rows have full candidate/retail payload checks in the
    # existing manifest generator. Compiler $SG ordinals are not stable.
    _, _, literals = read_tsv(BUILD / "gen/delink_data_manifest.tsv")
    for row in literals:
        if row["provenance"] == "retail-reloc-sg-literal":
            known_referents.setdefault(int(row["rva"], 16), set()).add(row["name"])
    for rva, name in pdb_synth.import_thunk_names(
            iat, named, {b.rva for b in model.functions}).items():
        known_referents.setdefault(rva, set()).add(name)
    split_eh_band(report)
    cur = functions(report)
    absolute_sites = pe.highlow_sites(RETAIL / "absolute_relocations.tsv")
    by_rva = {b.rva: b for b in model.functions}
    objs, rows = {}, []
    for c in census:
        b = by_rva[c["rva"]]
        key = (b.unit.rsplit("/", 1)[-1], b.name)
        row = {"rva": hex(b.rva), "name": b.name, "unit": b.unit,
               "census_size": c["size"], "size": b.size, "score": 0.0}
        reason = "unmapped function"
        if (b.rva, b.name) in reviewed and b.channel == "src" and key in cur:
            path = target_dir / (b.unit + ".c.obj")
            if path not in objs:
                objs[path] = CoffObject(path.read_bytes())
            obj = objs[path]
            syms = [s for s in obj.symbols.values() if s.name == b.name and s.section > 0]
            reason = "missing or ambiguous target body"
            if len(syms) == 1:
                sym = syms[0]
                reason = ""
                seen_absolute = set()
                for reloc in obj.relocations:
                    if reloc.section != sym.section or not sym.value <= reloc.site < sym.value + b.size:
                        continue
                    site = b.rva + reloc.site - sym.value
                    if reloc.typ == 6:
                        seen_absolute.add(site)
                    payload = pe.read(site, 4)
                    if payload is None or reloc.typ not in (6, 20):
                        reason = "unsupported reference"
                        break
                    value = struct.unpack("<I", payload)[0]
                    target = ((site + 4 + value) & 0xffffffff) if reloc.typ == 20 else value - pe.image_base
                    name = obj.symbols[reloc.symbol_index].name
                    reason = reference_reason(name, site, target, b.rva, b.size, proofs, known_referents)
                    if reason:
                        row["blocked_site"] = hex(site)
                        row["blocked_name"] = name
                        break
                if not reason and any(b.rva <= site < b.rva + b.size
                                      and site not in seen_absolute for site in absolute_sites):
                    reason = "missing absolute relocation"
                if not reason:
                    row["score"] = cur[key]
        row["status"] = reason or "scored"
        rows.append(row)
    summary = totals(rows)
    summary.update(kind="strict-comparison-lower-bound", image_sha256=pin.sha256,
                   input_digest=input_digest(), rows=rows)
    scored = [r for r in rows if r["status"] == "scored"]
    exact = summary["exact_functions"]
    path = out_dir / "baseline.json"
    path.write_text(json.dumps(summary, indent=2) + "\n")
    print(f"Baseline: {exact}/{len(rows)} exact ({summary['exact_percent']:.2f}%); "
          f"{summary['fuzzy_percent']:.2f}% fuzzy across the whole game. "
          f"{len(scored)} scored; {len(rows)-len(scored)} unscored (counted as zero).")
    return summary


def totals(rows):
    total_bytes = sum(r["census_size"] for r in rows)
    scored = [r for r in rows if r["status"] == "scored"]
    exact = sum(r["score"] == 100.0 for r in scored)
    return {
        "functions": len(rows), "code_bytes": total_bytes,
        "scored_functions": len(scored), "exact_functions": exact,
        "exact_percent": 100 * exact / len(rows) if rows else 0,
        "fuzzy_percent": sum(r["score"] * min(r["size"], r["census_size"])
                             for r in scored) / total_bytes if total_bytes else 0,
        "unscored_reasons": dict(Counter(r["status"] for r in rows
                                        if r["status"] != "scored")),
    }


def input_digest():
    """Detect changes since measurement; README/docs edits do not change code."""
    from homm1.core.paths import REPO
    paths = {REPO / "flake.nix", REPO / "flake.lock"}
    for directory in ("src", "include", "config", "scripts/homm1", "nix/patches",
                      "locales", "build/objdiff/base"):
        paths.update(p for p in (REPO / directory).rglob("*")
                     if p.is_file() and "__pycache__" not in p.parts)
    h = hashlib.sha256()
    for path in sorted(paths):
        h.update(str(path.relative_to(REPO)).encode() + b"\0")
        h.update(hashlib.sha256(path.read_bytes()).digest())
    return h.hexdigest()


def readme():
    from homm1.verify import readme as rm
    path = ROOT / "baseline.json"
    summary = json.loads(path.read_text())
    if summary.get("input_digest") != input_digest():
        raise ValueError("baseline is stale; run homm1 compare --baseline")
    return rm.write_block("\n".join([
        rm.RM_START,
        f"**Matching lower bound: {summary['exact_percent']:.2f}% exact "
        f"({summary['exact_functions']:,}/{summary['functions']:,} functions); "
        f"{summary['fuzzy_percent']:.2f}% fuzzy.**",
        "",
        f"{summary['scored_functions']:,} functions scored with strict references; "
        f"{summary['functions']-summary['scored_functions']:,} unscored, counted as zero. "
        "Full build verification remains incomplete.",
        "Generated by `homm1 compare --baseline` and `homm1 verify readme --baseline`.",
        rm.RM_END,
    ]))


def run(base_dir):
    from homm1.core import data_matching
    from homm1.delink.run import run as delink
    from homm1.compare.run import run as compare
    from homm1.compare.run import BASE_DIR
    if base_dir.resolve() != BASE_DIR.resolve():
        raise ValueError("baseline uses the configured build objects; omit --base-dir")
    if not data_matching.enabled():
        raise ValueError("baseline requires strict data matching")
    ROOT.mkdir(parents=True, exist_ok=True)
    target = ROOT / "retail"
    delink(target_dir=target, delink_dir=ROOT / "delink",
           report_unprovided=ROOT / "unprovided.tsv")
    report = compare(base_dir, target, ROOT / "compare", quiet=True)
    return summarize(report, target, ROOT)
