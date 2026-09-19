# Ported from HoMM2 Gold 2.1 Buka at 299514f88900c0cf30ba03422c72830a38fc1cb7.
#!/usr/bin/env python3
"""Generate the exhaustive live non-exact function queue."""

import csv
import json
import os
from pathlib import Path


REPO = Path(os.environ.get("HOMM1_DIR", Path(__file__).resolve().parents[3]))
IMAGE_BASE = 0x400000
REPORT = REPO / "build/objdiff/report.json"
SYMBOLS = REPO / "build/gen/symbol_names.csv"
OUTPUT = REPO / "build/gen/residual_function_queue.tsv"


def symbol_inventory(path):
    rows = {}
    with Path(path).open(encoding="latin-1", newline="") as stream:
        for row in csv.DictReader(stream):
            if row.get("kind") != "func":
                continue
            rows[(row["unit"], row["name"])] = row
    return rows


def residual_rows(report, symbols):
    rows = []
    missing = []
    for unit in report["units"]:
        unit_name = unit["name"]
        for function in unit.get("functions", []):
            fuzzy = float(function.get("fuzzy_match_percent", 0.0) or 0.0)
            # HoMM1 also checks resolved retail relocation identity. A fuzzy
            # 100% score alone must not remove a non-exact body from the queue.
            if function.get('exact', fuzzy >= 100.0):
                continue
            key = (unit_name, function["name"])
            symbol = symbols.get(key)
            if symbol is None:
                missing.append(key)
                continue
            rva = int(symbol["rva"], 0)
            rows.append({
                "unit": unit_name,
                "rva": rva,
                "fuzzy": fuzzy,
                "size": int(function.get("size", 0) or 0),
                "name": function["name"],
                "demangled": function.get("metadata", {}).get(
                    "demangled_name", function["name"]),
            })
    if missing:
        sample = ", ".join("%s:%s" % row for row in missing[:5])
        raise ValueError("%d report functions lack an RVA inventory row: %s" %
                         (len(missing), sample))
    rows.sort(key=lambda row: (-row["fuzzy"], row["rva"], row["unit"], row["name"]))
    for rank, row in enumerate(rows, 1):
        row["rank"] = rank
    return rows


def write_queue(path, rows):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fields = ("rank", "fuzzy", "unit", "rva", "va", "size", "name", "demangled")
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t",
                                lineterminator="\n")
        writer.writeheader()
        for row in rows:
            output = dict(row)
            output["fuzzy"] = "%.6f" % row["fuzzy"]
            output["rva"] = "0x%08x" % row["rva"]
            output["va"] = "0x%08x" % (row["rva"] + IMAGE_BASE)
            writer.writerow({field: output.get(field, "") for field in fields})


def main():
    from homm1.checkpoint import fresh_report
    rows = campaign_rows(fresh_report())
    write_queue(OUTPUT, rows)
    print("residual queue: %d live non-exact functions -> %s" %
          (len(rows), OUTPUT.relative_to(REPO)))


def campaign_rows(measurement):
    """Adapt HoMM1's measured report to the donor's objdiff-report schema."""
    units, symbols = {}, {}
    for fn in measurement['functions']:
        units.setdefault(fn['unit'], []).append(dict(name=fn['symbol'], size=fn['retail_size'],
                                                     fuzzy_match_percent=fn['score'], exact=fn['exact']))
        symbols[(fn['unit'], fn['symbol'])] = dict(rva=hex(fn['rva']))
    return residual_rows(dict(units=[dict(name=name, functions=functions) for name, functions in units.items()]), symbols)


if __name__ == "__main__":
    main()
