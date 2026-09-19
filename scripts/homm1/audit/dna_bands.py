"""Recover executable ownership with the HoMM2 masked-byte/DNA census.

This is the HoMM1 port of HoMM2's ``audit unmatched-census`` pass.  It
compares retail function bytes with functions in the exact VC4 libraries used
by the link, masking relocation operands on both sides.  Exact matches anchor
an object's definition order; functions bracketed by two anchors are then
identified from that order (the executable's "DNA bands").

The structural pass also admits compiler-owned rows which Ghidra did not make
top-level functions: C++ EH funclets, import thunks and linker alignment fill.
Running without ``--write-config`` only writes the review report.
"""

from __future__ import annotations

import argparse
import csv
import re
import struct
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

from homm1.compare.canonicalize import (
    CoffObject, FUNCTION_TYPE, MEM_EXECUTE, RELOCATION_WIDTHS,
)
from homm1.core.paths import BUILD, REPO, RETAIL, msvc_dir
from homm1.core.tsv import read as read_tsv
from homm1.delink import eh_band, pdb_synth
from homm1.delink.image import retail
from homm1.delink.implib import _ar_members
from homm1.model import resolve
from homm1.retail_labels.censuses import functions


OUTPUT = BUILD / "gen/homm1-dna-bands.tsv"
EVIDENCE = REPO / "evidence/homm1-dna-bands.tsv"
STATIC_LIBS = RETAIL / "functions_static_libs.tsv"
BANDS_EVIDENCE = REPO / "evidence/homm1-code-link-bands.tsv"
RUNTIME_LIBS = ("libc.lib", "oldnames.lib")
PREFIX = 24
PRIVATE = re.compile(r"^(?:_?\$E\d+|\?\?_[EG])")

HEADER = ("rva", "size", "name", "class", "source", "symbol", "detail")


def trim_padding(payload: bytes) -> bytes:
    end = len(payload)
    while end > 1 and payload[end - 1] in (0x90, 0xCC):
        end -= 1
    return payload[:end]


def mask_bytes(payload: bytes, sites: list[tuple[int, int]]) -> bytes:
    out = bytearray(payload)
    for site, width in sites:
        if site < 0 or site >= len(out):
            continue
        end = min(len(out), site + width)
        out[site:end] = b"\0" * (end - site)
    return bytes(out)


@dataclass(frozen=True)
class Candidate:
    source: str
    symbol: str
    order: int
    body: bytes
    sites: tuple[tuple[int, int], ...]


def _coff_candidates(payload: bytes, source: str) -> list[Candidate]:
    try:
        coff = CoffObject(payload)
    except (ValueError, IndexError, struct.error):
        return []
    out: list[Candidate] = []
    order = 0
    for section in coff.sections:
        if not section.characteristics & MEM_EXECUTE or not section.raw_size:
            continue
        symbols = sorted(
            (s for s in coff.symbols.values()
             if s.section == section.index and s.typ == FUNCTION_TYPE),
            key=lambda s: (s.value, s.index),
        )
        relocs = [r for r in coff.relocations if r.section == section.index]
        section_body = coff.data[section.raw_offset:
                                 section.raw_offset + section.raw_size]
        for index, symbol in enumerate(symbols):
            end = (symbols[index + 1].value if index + 1 < len(symbols)
                   else section.raw_size)
            if end <= symbol.value:
                continue
            local = tuple(
                (r.site - symbol.value, RELOCATION_WIDTHS.get(r.typ, 4))
                for r in relocs if symbol.value <= r.site < end
            )
            raw = trim_padding(section_body[symbol.value:end])
            local = tuple((site, width) for site, width in local
                          if site < len(raw))
            out.append(Candidate(source, symbol.name, order,
                                 mask_bytes(raw, list(local)), local))
            order += 1
    return out


def _candidate_index():
    by_size: dict[int, list[Candidate]] = defaultdict(list)
    orders: dict[str, list[Candidate]] = defaultdict(list)

    def add(rows):
        for row in rows:
            by_size[len(row.body)].append(row)
            orders[row.source].append(row)

    # Own objects are included only to recover compiler-private bodies.  They
    # can never turn an ordinary game body into a library exclusion.
    base = BUILD / "objdiff/base"
    for path in sorted(base.rglob("*.obj")):
        source = "base:" + str(path.relative_to(base).with_suffix(""))
        add(_coff_candidates(path.read_bytes(), source))

    libdir = msvc_dir() / "lib"
    for library in RUNTIME_LIBS:
        path = libdir / library
        if not path.is_file():
            continue
        for member, payload in _ar_members(path):
            add(_coff_candidates(payload, f"{library}:{member}"))
    return by_size, orders


def _image_rows():
    image = retail()
    rows = []
    for row in functions():
        rva, span = row["rva"], row["size"]
        raw = trim_padding(image.payload(rva, span))
        sites = [(site - rva, 4) for site in image.relocs_in(rva, rva + len(raw))]
        rows.append({"rva": rva, "size": span, "kind": row["kind"],
                     "body": mask_bytes(raw, sites), "mask": sites})
    return rows


def _matches(image_body: bytes, candidates: list[Candidate]) -> list[Candidate]:
    out = []
    for candidate in candidates:
        # Candidate REL32/DIR32 positions are authoritative even where the PE
        # base-relocation table cannot record them.  Apply their mask to both
        # sides, exactly as HoMM2's union-mask fallback does.
        if mask_bytes(image_body, list(candidate.sites)) == candidate.body:
            out.append(candidate)
    return out


def _thunks() -> dict[int, tuple[str, str, int]]:
    image = retail()
    slots = {rva: (name, dll, ordinal)
             for rva, name, dll, ordinal in image.import_slots()}
    out = {}
    for row in functions():
        body = image.payload(row["rva"], min(6, row["size"]))
        if len(body) != 6 or body[:2] != b"\xff\x25":
            continue
        slot = struct.unpack_from("<I", body, 2)[0] - image.image_base
        if slot not in slots:
            continue
        name, dll, ordinal = slots[slot]
        out[row["rva"]] = (dll, name or f"ordinal_{ordinal}", slot)
    return out


def _donor_helper_hints() -> dict[int, tuple[str, str]]:
    """Private identities exposed only by the donor-order segmentation.

    These cannot be source names (the ``$E`` ordinal is compiler-private), but
    a segment whose first ordered donor identity is ``$E<n>`` proves that its
    first retail row is a lifecycle helper.  Keep the ordinal as evidence and
    classify the row by kind, exactly as HoMM2's compgen census does.
    """
    path = REPO / "evidence/homm2-tu-segments.tsv"
    if not path.is_file():
        return {}
    _body, _header, rows = read_tsv(path)
    out = {}
    for row in rows:
        name = row.get("first_evidence", "")
        if PRIVATE.match(name):
            out[int(row["start_rva"], 16)] = (name, row["unit"])
    return out


def run_census():
    by_size, orders = _candidate_index()
    library_candidates = [candidate for rows in orders.values()
                          for candidate in rows
                          if candidate.source.split(":", 1)[0] in RUNTIME_LIBS]
    thunks = _thunks()
    helper_hints = _donor_helper_hints()
    rows = []
    anchors: dict[str, list[tuple[int, int]]] = defaultdict(list)
    image_by_rva = {r["rva"]: r for r in _image_rows()}

    for item in image_by_rva.values():
        rva, body = item["rva"], item["body"]
        row = {"rva": rva, "size": item["size"], "name": f"FUN_{rva + 0x400000:08X}",
               "class": "unknown", "source": "", "symbol": "", "detail": ""}
        if item["kind"] == "eh":
            row["class"] = "eh-funclet"
            rows.append(row)
            continue
        if item["kind"] == "pad":
            row["class"] = "linker-pad"
            rows.append(row)
            continue
        if item["kind"] == "helper":
            symbol, unit = helper_hints.get(rva, ("", ""))
            row.update(**{"class": "compiler-helper", "source": unit,
                          "symbol": symbol,
                          "detail": ("HoMM2 donor definition-order segment"
                                     if symbol else "committed helper-kind row")})
            rows.append(row)
            continue
        if rva in helper_hints:
            symbol, unit = helper_hints[rva]
            row.update(**{"class": "helper-order", "source": unit,
                          "symbol": symbol,
                          "detail": "HoMM2 donor definition-order segment"})
            rows.append(row)
            continue
        if rva in thunks:
            dll, symbol, slot = thunks[rva]
            row.update(**{"class": "import-thunk", "source": dll,
                          "symbol": symbol, "detail": f"IAT RVA 0x{slot:x}"})
            rows.append(row)
            continue
        matches = _matches(body, by_size.get(len(body), []))
        library = [m for m in matches if m.source.split(":", 1)[0] in RUNTIME_LIBS]
        private = [m for m in matches if m.source.startswith("base:")
                   and PRIVATE.match(m.symbol)]
        chosen = None
        if library and len({m.symbol for m in library}) == 1:
            chosen = library[0]
            row["class"] = "crt-exact"
        elif private and len({m.symbol for m in private}) == 1:
            chosen = private[0]
            row["class"] = "helper-exact"
        if chosen:
            row["source"], row["symbol"] = chosen.source, chosen.symbol
            same_source = [m for m in matches if m.source == chosen.source]
            if len(same_source) == 1:
                anchors[chosen.source].append((chosen.order, rva))
            if len(matches) > 1:
                row["detail"] = f"{len(matches)} byte-identical candidates"
        rows.append(row)

    # HoMM2's 24-byte prefix pass.  This recovers names where object/image
    # extents differ while keeping collisions visible and unclaimed.
    for row in rows:
        if row["class"] != "unknown":
            continue
        body = image_by_rva[row["rva"]]["body"]
        if len(body) < PREFIX:
            continue
        matches = [candidate for candidate in library_candidates
                   if len(candidate.body) >= PREFIX
                   and mask_bytes(body[:PREFIX], list(candidate.sites))
                   == candidate.body[:PREFIX]]
        if matches and len({candidate.symbol for candidate in matches}) == 1:
            row.update(**{"class": "crt-prefix", "source": matches[0].source,
                          "symbol": matches[0].symbol,
                          "detail": f"unique masked {PREFIX}-byte prefix"})

    # Same object, consecutive exact anchors: fill the bracketed definitions.
    by_rva = {r["rva"]: r for r in rows}
    ordered_rvas = sorted(by_rva)
    for source, hits in anchors.items():
        candidates = {c.order: c for c in orders[source]}
        hits.sort()
        for (lo_order, lo_rva), (hi_order, hi_rva) in zip(hits, hits[1:]):
            missing_orders = list(range(lo_order + 1, hi_order))
            missing_rvas = [rva for rva in ordered_rvas
                            if lo_rva < rva < hi_rva
                            and by_rva[rva]["class"] == "unknown"]
            if not missing_orders or len(missing_orders) != len(missing_rvas):
                continue
            for order, rva in zip(missing_orders, missing_rvas):
                candidate = candidates.get(order)
                if candidate is None:
                    continue
                is_library = source.split(":", 1)[0] in RUNTIME_LIBS
                is_private = source.startswith("base:") and PRIVATE.match(candidate.symbol)
                if not (is_library or is_private):
                    continue
                by_rva[rva].update(
                    **{"class": "crt-order" if is_library else "helper-order",
                       "source": source, "symbol": candidate.symbol,
                       "detail": f"between exact anchors 0x{lo_rva:x},0x{hi_rva:x}"})

    # Exact identities make the link boundary visible.  In this image the
    # final vendor import-thunk block is immediately followed by LIBC and all
    # later non-thunk text has dense LIBC anchors through the section end.
    # Preserve ambiguous names as address labels while still assigning their
    # proven band ownership; those rows leave the game reconstruction target.
    crt_anchors = sorted(row["rva"] for row in rows
                         if row["class"] in ("crt-exact", "crt-prefix"))
    if crt_anchors:
        before = [row["rva"] for row in rows
                  if row["class"] == "import-thunk" and row["rva"] < crt_anchors[0]]
        if before:
            boundary = next((row["rva"] for row in rows
                             if row["rva"] > max(before)
                             and row["class"] != "linker-pad"), crt_anchors[0])
            for row in rows:
                if row["rva"] >= boundary and row["class"] == "unknown":
                    row.update(**{"class": "crt-band", "source": "libc.lib:(link-band)",
                                  "symbol": f"__crt_{row['rva'] + 0x400000:08X}",
                                  "detail": (f"LIBC band 0x{boundary:x}..text-end; "
                                             "symbol unresolved")})
    return rows


def _eh_records():
    model = resolve()
    groups = eh_band.groups(retail().pe.path, pdb_synth.unit_names(model))
    return eh_band.records(groups)


def _pad_starts(starts: list[int]) -> set[int]:
    image = retail()
    text_end = image.pe.section(".text")["va"] + image.pe.section(".text")["vsize"]
    out = set()
    for start, end in zip(starts, starts[1:] + [text_end]):
        body = image.payload(start, end - start)
        pos = len(body)
        while pos and body[pos - 1] in (0x90, 0xCC):
            pos -= 1
        if pos and pos < len(body):
            out.add(start + pos)
    return out


def partition_report(rows):
    """Split EH children and alignment fill for reporting, never delinking."""
    by_rva = {row["rva"]: dict(row) for row in rows}
    eh_rvas = {rva for rva, _name, _unit, _size in _eh_records()}
    starts = sorted(set(by_rva) | eh_rvas)
    pads = _pad_starts(starts)
    starts = sorted(set(starts) | pads)
    text = retail().pe.section(".text")
    text_end = text["va"] + text["vsize"]
    out = []
    for rva, end in zip(starts, starts[1:] + [text_end]):
        if rva in pads:
            row = {"rva": rva, "size": end - rva,
                   "name": f"FUN_{rva + retail().image_base:08X}",
                   "class": "linker-pad", "source": "", "symbol": "",
                   "detail": "trailing 90/CC alignment fill"}
        elif rva in eh_rvas:
            row = {"rva": rva, "size": end - rva,
                   "name": f"FUN_{rva + retail().image_base:08X}",
                   "class": "eh-funclet", "source": "", "symbol": "",
                   "detail": "FuncInfo/unwind-map child record"}
        else:
            row = by_rva[rva]
            row["size"] = end - rva
        out.append(row)
    return out


def write_config(rows) -> dict[str, int]:
    # EH/pad subranges are evidence consumed by verify.universe.  They stay
    # out of functions.tsv because pdb_synth already carves EH and Vostok's
    # module partition must retain the original Ghidra parent starts.
    eh = _eh_records()
    report = partition_report(rows)
    pad = [row for row in report if row["class"] == "linker-pad"]
    source_extents = 0

    library_rows = []
    for row in rows:
        if not row["class"].startswith("crt-"):
            continue
        library, _, member = row["source"].partition(":")
        confidence = {"crt-exact": "HIGH", "crt-prefix": "MEDIUM",
                      "crt-order": "MEDIUM", "crt-band": "BAND"}[row["class"]]
        source = {"crt-exact": "masked-exact", "crt-prefix": "masked-prefix",
                  "crt-order": "definition-order", "crt-band": "link-band"}[row["class"]]
        library_rows.append((row["rva"], row["symbol"], library, confidence,
                             f"{source}:{member}"))
    STATIC_LIBS.write_text(
        "# Delinker-active CRT/SDK providers. DNA identities intentionally stay\n"
        "# in evidence/homm1-dna-bands.tsv: enrolling the whole runtime band here\n"
        "# changes Vostok's anonymous-module partition.\n"
        "rva\tname\tlib\tconfidence\tsource\n"
    )
    first_crt = min(row["rva"] for row in rows
                    if row["class"].startswith("crt-"))
    main_thunks = sorted(
        (row for row in rows
         if row["class"] == "import-thunk" and row["rva"] < first_crt),
        key=lambda row: row["rva"],
    )
    thunk_start = main_thunks[-1]["rva"]
    for previous, current in zip(reversed(main_thunks[:-1]),
                                 reversed(main_thunks[1:])):
        if previous["rva"] + previous["size"] != current["rva"]:
            break
        thunk_start = previous["rva"]
    text = retail().pe.section(".text")
    text_lo, text_hi = text["va"], text["va"] + text["vsize"]
    BANDS_EVIDENCE.write_text(
        "# Code-only link-layout ownership recovered from the executable DNA census.\n"
        "# This remains evidence until data matching begins; config/retail/link_bands.tsv\n"
        "# deliberately stays empty because it activates strict data-target ownership.\n"
        "lo\thi\tband\n"
        f"0x{text_lo:08x}\t0x{thunk_start:08x}\tgame\n"
        f"0x{thunk_start:08x}\t0x{first_crt:08x}\timport-thunks\n"
        f"0x{first_crt:08x}\t0x{text_hi:08x}\tlibc\n"
    )
    return {"eh": len(eh),
            "thunk": sum(row["class"] == "import-thunk" for row in report),
            "helper": sum(row["class"] in ("compiler-helper", "helper-order")
                          for row in report),
            "pad": len(pad),
            "library": len(library_rows), "source_extents": source_extents}


def write_report(rows, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as stream:
        stream.write("# HoMM1 executable DNA census against VC4 LIBC.LIB/OLDNAMES.LIB.\n")
        stream.write("# Exact rows mask relocation fields on both sides; order rows are bracketed by exact anchors.\n")
        writer = csv.DictWriter(stream, HEADER, delimiter="\t", lineterminator="\n")
        writer.writeheader()
        for row in rows:
            cooked = dict(row)
            cooked["rva"] = f"0x{row['rva']:08x}"
            cooked["size"] = f"0x{row['size']:x}"
            # Keep the seven-column TSV shape without leaving a trailing tab;
            # generated evidence is still expected to pass `git diff --check`.
            cooked["detail"] = row.get("detail") or "-"
            writer.writerow(cooked)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write-config", action="store_true")
    args = parser.parse_args(argv)
    rows = run_census()
    report = partition_report(rows)
    write_report(report, OUTPUT)
    totals = defaultdict(lambda: [0, 0])
    for row in report:
        totals[row["class"]][0] += 1
        totals[row["class"]][1] += row["size"]
    print(f"[dna-bands] {len(rows)} functions -> {OUTPUT.relative_to(REPO)}")
    for kind in sorted(totals):
        print(f"[dna-bands] {kind:16} {totals[kind][0]:4} functions "
              f"{totals[kind][1]:7} bytes")
    if args.write_config:
        write_report(report, EVIDENCE)
        counts = write_config(rows)
        print("[dna-bands] wrote " + ", ".join(f"{k}={v}" for k, v in counts.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
