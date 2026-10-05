"""Diagnostic progress reporting while whole-image verification is incomplete.

Uses ordinary strict objdiff scores for source-annotated bodies. Reference
review is reported separately and never replaces a measured score with zero.
Missing comparisons remain zero in the denominator. This never updates MAX.
"""
from __future__ import annotations

import hashlib
import json
import struct
from collections import Counter

from homm1.compare.canonicalize import CoffObject
from homm1.core.paths import BUILD, IMAGE_BUILD, RETAIL
from homm1.core.tsv import read as read_tsv

ROOT = IMAGE_BUILD / "objdiff/baseline"


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


def reconstruction_census(census, bindings):
    """Annotations enroll bodies for matching; inventory labels only name targets.

    VA and VA_COMPGEN supply source bodies. Declaration-only labels, dynamic
    initializer owner pins, library identities and inferred TU ownership do not.
    Keep annotated bodies even when their comparison is absent or unscored.
    """
    bodies = {b.rva for b in bindings if b.channel in ("src", "src_compgen")}
    return [row for row in census if not row["kind"] and row["rva"] in bodies]


def audit_references(census, model, target_dir, pe, image_hash):
    """Check every reference independently of score reporting, without early exit."""
    from homm1.delink import pdb_synth

    reviewed, proofs = evidence(sorted(RETAIL.glob("buka-*.json")), image_hash)
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
    _, _, literals = read_tsv(IMAGE_BUILD / "gen/delink_data_manifest.tsv")
    for row in literals:
        if row["provenance"] == "retail-reloc-sg-literal":
            known_referents.setdefault(int(row["rva"], 16), set()).add(row["name"])
    for rva, name in pdb_synth.import_thunk_names(
            iat, named, {b.rva for b in model.functions}).items():
        known_referents.setdefault(rva, set()).add(name)
    absolute_sites = pe.highlow_sites(RETAIL / "absolute_relocations.tsv")
    by_rva = {b.rva: b for b in model.functions}
    objs, rows = {}, []
    for c in census:
        b = by_rva[c["rva"]]
        row = {"rva": hex(b.rva), "name": b.name, "unit": b.unit,
               "size": b.size, "references": [], "issues": []}
        if (b.rva, b.name) not in reviewed:
            row["issues"].append({"reason": "unreviewed function identity"})
        path = target_dir / (b.unit + ".c.obj")
        if path not in objs:
            objs[path] = CoffObject(path.read_bytes())
        obj = objs[path]
        syms = [s for s in obj.symbols.values() if s.name == b.name and s.section > 0]
        if len(syms) != 1:
            row["issues"].append({"reason": "missing or ambiguous target body"})
        else:
            sym = syms[0]
            seen_absolute = set()
            for reloc in obj.relocations:
                if reloc.section != sym.section or not sym.value <= reloc.site < sym.value + b.size:
                    continue
                site = b.rva + reloc.site - sym.value
                if reloc.typ == 6:
                    seen_absolute.add(site)
                payload = pe.read(site, 4)
                if payload is None or reloc.typ not in (6, 20):
                    row["issues"].append({"site_rva": hex(site),
                                          "reason": "unsupported reference"})
                    continue
                value = struct.unpack("<I", payload)[0]
                target = ((site + 4 + value) & 0xffffffff) if reloc.typ == 20 else value - pe.image_base
                name = obj.symbols[reloc.symbol_index].name
                ref = {"site_rva": hex(site), "target_rva": hex(target),
                       "name": name, "kind": reloc.typ}
                row["references"].append(ref)
                reason = reference_reason(name, site, target, b.rva, b.size, proofs, known_referents)
                if reason:
                    row["issues"].append(dict(ref, reason=reason))
            for site in absolute_sites:
                if b.rva <= site < b.rva + b.size and site not in seen_absolute:
                    row["issues"].append({"site_rva": hex(site),
                                          "reason": "missing absolute relocation"})
        rows.append(row)
    issues = [i for r in rows for i in r["issues"]]
    return {"image_sha256": image_hash, "summary": {
        "functions": len(rows),
        "functions_with_issues": sum(bool(r["issues"]) for r in rows),
        "reference_sites": sum(len(r["references"]) for r in rows),
        "issue_sites": len(issues),
        "reasons": dict(Counter(i["reason"] for i in issues)),
    }, "functions": rows}


def comparison_rows(census, bindings, cur):
    """Use measured objdiff scores directly; only absent comparisons are unscored."""
    by_rva = {b.rva: b for b in bindings}
    rows = []
    for c in census:
        b = by_rva[c["rva"]]
        key = (b.unit.rsplit("/", 1)[-1], b.name)
        rows.append({"rva": hex(b.rva), "name": b.name, "unit": b.unit,
                     "census_size": c["size"], "size": b.size,
                     "score": cur.get(key, 0.0),
                     "status": "scored" if key in cur else "missing source comparison"})
    return rows


def summarize(report, target_dir, out_dir):
    from homm1.core.inputs import read_verified, targets
    from homm1.core.pe import image
    from homm1.model import resolve
    from homm1.retail_labels import censuses
    from homm1.verify.scores import functions, split_eh_band

    pin = targets()["game"]
    read_verified(pin, pin.destination)
    model = resolve()
    banners, _, _ = read_tsv(RETAIL / "functions.tsv")
    if f"# image-sha256: {pin.sha256}" not in banners:
        raise ValueError("baseline requires an image-pinned structural census")
    census = reconstruction_census(censuses.functions(), model.functions)
    audit = audit_references(census, model, target_dir, image(), pin.sha256)
    audit["input_digest"] = input_digest()
    (out_dir / "reference-audit.json").write_text(json.dumps(audit, indent=2) + "\n")
    split_eh_band(report)
    rows = comparison_rows(census, model.functions, functions(report))
    summary = totals(rows)
    summary.update(kind="strict-diagnostic-comparison", image_sha256=pin.sha256,
                   input_digest=audit["input_digest"], rows=rows)
    (out_dir / "baseline.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(f"Baseline: {summary['exact_functions']}/{len(rows)} exact "
          f"({summary['exact_percent']:.2f}%); {summary['fuzzy_percent']:.2f}% fuzzy "
          f"across source-annotated bodies. {summary['scored_functions']} scored; "
          f"{len(rows)-summary['scored_functions']} missing comparisons (counted as zero).")
    print(f"Reference audit: {audit['summary']['reference_sites']} sites; "
          f"{audit['summary']['issue_sites']} issues (reported separately from scores).")
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


def module_table(rows, sources):
    """Roll up annotated reconstruction bodies by their source module."""
    from homm1.verify import readme as rm
    groups = {}
    for row in rows:
        source = sources.get(row["unit"])
        module = rm.module_of(source) if source else "(unmapped)"
        groups.setdefault(module, []).append(row)
    table_rows = []
    for module in sorted(groups, key=lambda k: (k == "(unmapped)", -len(groups[k]))):
        group = groups[module]
        total = totals(group)
        units = str(len({r["unit"] for r in group})) if module != "(unmapped)" else "—"
        table_rows.append([
            f"`{module}`", units,
            f"{total['exact_functions']:,} / {total['functions']:,} "
            f"({total['exact_percent']:.1f}%)",
            f"{total['fuzzy_percent']:.1f}%",
        ])
    return rm._md_table(["Module", "Units", "Functions exact", "Fuzzy"],
                        "lrrr", table_rows)


def readme():
    from homm1 import manifest
    from homm1.verify import readme as rm
    path = ROOT / "baseline.json"
    summary = json.loads(path.read_text())
    if summary.get("input_digest") != input_digest():
        raise ValueError("baseline is stale; run homm1 compare --baseline")
    return rm.write_block("\n".join([
        rm.RM_START,
        f"**Matching: {summary['exact_percent']:.2f}% exact "
        f"({summary['exact_functions']:,}/{summary['functions']:,} annotated functions); "
        f"{summary['fuzzy_percent']:.2f}% fuzzy.**",
        "",
        *module_table(summary["rows"], {u["unit"]: u["source"] for u in manifest.units()}),
        "",
        f"{summary['scored_functions']:,} functions compared with strict objdiff; "
        f"{summary['functions']-summary['scored_functions']:,} missing comparisons, counted as zero. "
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
