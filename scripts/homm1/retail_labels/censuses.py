"""homm1.retail_labels.censuses - the base censuses: structure only.

functions.tsv and data.tsv contribute STARTS and KINDS; a row's extent is
DERIVED to the next row (functions: to .text's virtual end; data: to the next
row within the same section, else the section edge). Identity never comes
from here - that is the providers' job (the model enforces it).
"""

from __future__ import annotations

from pathlib import Path

from homm1.core.paths import RETAIL
from homm1.core.pe import image
from homm1.core.tsv import read as read_tsv

FUNCTION_KINDS = ("", "thunk", "eh", "helper", "pad")
DATA_KINDS = ("", "string", "fppool", "vtable", "rtti", "ehtable", "guard",
              "common", "copy", "pad")



def _rows(path: Path, kinds: tuple[str, ...],
          in_range) -> list[dict]:
    _b, _h, raw = read_tsv(path)
    rows = []
    for r in raw:
        rva = int(r["rva"], 16)
        kind = r.get("kind", "").strip()
        if kind not in kinds:
            raise ValueError(f"{path}: unknown kind {kind!r} at 0x{rva:08x}")
        if not in_range(rva):
            raise ValueError(f"{path}: 0x{rva:08x} outside its address space")
        rows.append({"rva": rva, "kind": kind})
    rows.sort(key=lambda r: r["rva"])
    for a, b in zip(rows, rows[1:]):
        if a["rva"] == b["rva"]:
            raise ValueError(f"{path}: duplicate row 0x{a['rva']:08x}")
    return rows


def functions(path: Path | None = None) -> list[dict]:
    """[{rva, kind, size}] - size derived to the next start / .text end.
    The edges come from the retail PE's own section table, never constants."""
    text_lo, text_end = image().text_span()
    rows = _rows(path or RETAIL / "functions.tsv", FUNCTION_KINDS,
                 lambda v: text_lo <= v < text_end)
    for row, nxt in zip(rows, rows[1:] + [None]):
        row["size"] = (nxt["rva"] if nxt else text_end) - row["rva"]
    return rows


def data(path: Path | None = None) -> list[dict]:
    """[{rva, kind, region, size}] - size derived within the row's region;
    the region edges come from the retail PE's section table."""
    regions = image().data_regions()

    def region(v):
        return next((k for k, (lo, hi) in regions.items() if lo <= v < hi),
                    None)
    rows = _rows(path or RETAIL / "data.tsv", DATA_KINDS,
                 lambda v: region(v) is not None)
    for r in rows:
        r["region"] = region(r["rva"])
    # data+bss are ONE PE section (.data raw bytes + loader-zero tail), so an
    # extent may legitimately cross the rawsize edge - a datum at the edge is
    # partly stored, partly zero-fill. Regions stay as reporting tags; the
    # extent cap only honours REAL edges (section ends).
    contiguous = {"data": "bss"}
    for row, nxt in zip(rows, rows[1:] + [None]):
        hi = regions[row["region"]][1]
        if nxt is not None and (nxt["region"] == row["region"]
                                or contiguous.get(row["region"]) == nxt["region"]):
            hi = nxt["rva"]
        elif contiguous.get(row["region"]):
            hi = regions[contiguous[row["region"]]][1]
        row["size"] = hi - row["rva"]
    return rows


def link_order_bands(path: Path | None = None) -> list[tuple[int, int, str]]:
    """[(lo, hi, unit)] from link_order.tsv - per-unit retail contribution
    bands (positional columns; class=comdat-owner rows carry no span)."""
    out = []
    for line in (path or RETAIL / "link_order.tsv").read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        f = line.split("\t")
        if len(f) >= 4 and f[2].startswith("0x"):
            out.append((int(f[2], 16), int(f[3], 16), f[1]))
    out.sort()
    return out


def link_bands(path: Path | None = None) -> list[tuple[int, int, str]]:
    """[(lo, hi, band)] - the coarse link-layout bands, sorted."""
    _b, _h, raw = read_tsv(path or RETAIL / "link_bands.tsv")
    out = [(int(r["lo"], 16), int(r["hi"], 16), r["band"]) for r in raw]
    out.sort()
    for (alo, ahi, an), (blo, _bh, bn) in zip(out, out[1:]):
        if ahi > blo:
            raise ValueError(f"link_bands: {an} overlaps {bn} at 0x{blo:08x}")
    return out
