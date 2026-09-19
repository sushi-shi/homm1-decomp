"""homm1.ghidra.export - the Model, flattened for the Ghidra viewer.

    homm1 ghidra export [--out P] [--quiet]

The Ghidra side is a VIEWER: this module is the one place that decides what
the reconstruction knows about an address, and it writes that decision to a
single self-contained payload (build/ghidra/knowledge.json). The Ghidra-side
script reads only that file, so it never imports this tree and never re-scans
src/ - the same discipline model.py imposes on the delinker.

Inputs (no others):
  * build/gen/bindings.tsv  - the serialized Model (homm1.model); regenerated
                              here from resolve() when absent;
  * config/retail/link_bands.tsv - the coarse link-layout bands;
  * the retail image        - image base only (core/pe is the section
                              authority; nothing here hardcodes a constant).

`pad` rows are the linker's fill, not objects: they are counted and dropped.
Everything else is exported, claimed or not - an unclaimed census row is still
a real object start with a derived extent, which is knowledge Ghidra's own
carve does not have.

The payload carries a content `digest` over its own body; project.py stamps it
after a successful apply, so `update` is a hash compare, not a re-analysis.
"""

from __future__ import annotations

import hashlib
import json
from bisect import bisect_right
from pathlib import Path

from homm1.core.paths import BUILD, RETAIL
from homm1.core.tsv import read as read_tsv
from homm1.core.tsv import rint

PAYLOAD = BUILD / "ghidra/knowledge.json"
LINK_BANDS = RETAIL / "link_bands.tsv"

#: bumped when the Ghidra-side script needs a payload shape it cannot ignore
SCHEMA = 1

#: census kinds that are linker fill rather than an object
DROP_KINDS = ("pad",)


def bands() -> list[dict]:
    """The coarse link-layout bands, ascending."""
    _, _, rows = read_tsv(LINK_BANDS)
    out = [{"lo": rint(r["lo"]), "hi": rint(r["hi"]), "band": r["band"],
            "note": r["note"]} for r in rows]
    out.sort(key=lambda b: b["lo"])
    return out


def _band_lookup(band_rows: list[dict]):
    """rva -> band name ('' outside every band, e.g. .idata)."""
    los = [b["lo"] for b in band_rows]

    def lookup(rva: int) -> str:
        i = bisect_right(los, rva) - 1
        if i < 0:
            return ""
        b = band_rows[i]
        return b["band"] if rva < b["hi"] else ""

    return lookup


def bindings() -> list[dict]:
    """The serialized Model's rows. Resolves + serializes first when the
    bindings file is absent, so a fresh checkout can export without a build."""
    from homm1.model import BINDINGS, resolve, serialize
    if not BINDINGS.is_file():
        serialize(resolve())
    _, _, rows = read_tsv(BINDINGS)
    return rows


def _split(value: str) -> list[str]:
    return [p for p in value.split(";") if p]


def payload() -> dict:
    """The complete export document, including its own content digest."""
    from homm1.core.pe import image

    band_rows = bands()
    band_of = _band_lookup(band_rows)
    text, data = [], []
    dropped = 0
    for row in bindings():
        kind = row["kind"]
        if kind in DROP_KINDS:
            dropped += 1
            continue
        rva = rint(row["rva"])
        rec = {"rva": rva, "size": rint(row["size"]), "kind": kind,
               "name": row["name"], "unit": row["unit"],
               "channel": row["channel"], "band": band_of(rva),
               "also": _split(row["also_units"]),
               "aliases": _split(row["aliases"])}
        if row["space"] == "text":
            text.append(rec)
        else:
            rec["space"] = row["space"]
            data.append(rec)
    text.sort(key=lambda r: r["rva"])
    data.sort(key=lambda r: r["rva"])

    pe = image()
    body = {
        "schema": SCHEMA,
        "image_base": pe.image_base,
        "exe": str(pe.path),
        "bands": band_rows,
        "text": text,
        "data": data,
        "counts": counts(text, data, dropped),
    }
    body["digest"] = hashlib.sha256(
        json.dumps(body, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()
    return body


def counts(text: list[dict], data: list[dict], dropped: int) -> dict:
    """The numbers `build`/`update` report; also the payload's own census."""
    return {
        "functions": len(text),
        "functions_claimed": sum(1 for r in text if r["channel"]),
        "functions_census_only": sum(1 for r in text if not r["channel"]),
        "functions_eh": sum(1 for r in text if r["kind"] == "eh"),
        "functions_thunk": sum(1 for r in text if r["kind"] == "thunk"),
        "data": len(data),
        "data_claimed": sum(1 for r in data if r["channel"]),
        "data_census_only": sum(1 for r in data if not r["channel"]),
        "vtables": sum(1 for r in data if r["kind"] == "vtable"),
        "strings": sum(1 for r in data if r["kind"] == "string"),
        "units": len({r["unit"] for r in text + data if r["unit"]}),
        "aliases": sum(len(r["aliases"]) for r in text + data),
        "pad_rows_dropped": dropped,
    }


def write(out: Path | str = PAYLOAD) -> tuple[dict, bool]:
    """Write the payload write-if-changed; returns (payload, changed)."""
    doc = payload()
    text = json.dumps(doc, indent=1, sort_keys=True) + "\n"
    out = Path(out)
    if out.is_file() and out.read_text() == text:
        return doc, False
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(text)
    return doc, True


def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="homm1 ghidra export", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", default=str(PAYLOAD),
                    help=f"payload path (default {PAYLOAD})")
    ap.add_argument("--quiet", action="store_true",
                    help="write the payload, print nothing")
    a = ap.parse_args(argv)
    doc, changed = write(a.out)
    if not a.quiet:
        c = doc["counts"]
        print(f"[ghidra] {a.out} {'UPDATED' if changed else 'unchanged'} "
              f"digest={doc['digest'][:12]}")
        print(f"[ghidra] functions {c['functions']} "
              f"({c['functions_claimed']} claimed, "
              f"{c['functions_census_only']} census-only) | "
              f"data {c['data']} ({c['data_claimed']} claimed) | "
              f"bands {len(doc['bands'])} | units {c['units']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
