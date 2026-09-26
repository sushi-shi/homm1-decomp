#!/usr/bin/env python3
"""Classify residuals and run bounded N-island/M-frontier searches.

``candidates`` derives the whole source-owned population from the live wall
inventory and normalized object pairs. ``run`` samples independent compiler
state islands, optionally crossed with class-appropriate AST source shapes,
then retains the M highest distinct target states for agent inspection.

Campaign output is hypothesis evidence. It never edits reconstructed source or
banks MAX; an agent must explain and reimplement a defensible source pattern.
"""

from __future__ import annotations

from homm1.core.usage import logged

import argparse
import json
import re
import subprocess
import time
from collections import Counter
from pathlib import Path

from homm1.delink.coffx import Obj
from homm1.model import resolve
from homm1.permute.generate_ast_variants import main as variants_main
from homm1.permute.tu_state_noise import load_units, project_root
from homm1.verify.scores import is_eh_band
from homm1.walls import diagnose as wall_diagnose
from homm1.walls import inventory


CLASS_ORDER = {
    "regalloc": 0,
    "cfg": 1,
    "inline": 2,
    "referent": 3,
    "none": 4,
    "unavailable": 5,
}

CLASS_FAMILIES = {
    "regalloc": "commutative_order,relational_order,independent_statement_order,"
                "declaration_split,declaration_merge,declaration_hoist,identifier_rename",
    "cfg": "terminal_return_order,independent_statement_order,relational_order,"
           "commutative_order,declaration_split,declaration_merge,declaration_hoist",
    "inline": "inline_expression,inline_read_advance,inline_nested_expression,"
              "inline_member_access,inline_global_read,declaration_hoist",
    "referent": "commutative_order,relational_order,identifier_rename",
    "none": "commutative_order,relational_order,independent_statement_order,"
            "declaration_split,declaration_merge,declaration_hoist",
    "unavailable": "commutative_order",
}

_SYMBOL_HEADER = re.compile(r"^[0-9a-fA-F]+ <(.+)>:$")


def object_assemblies(path: Path, function_symbols: set[str]) -> dict[str, str]:
    """Disassemble a normalized object once and split its COMDATs by symbol."""
    result = subprocess.run(
        ["llvm-objdump", "-dr", "--x86-asm-syntax=intel", str(path)],
        capture_output=True, text=True,
    )
    if result.returncode:
        raise OSError(
            f"llvm-objdump failed for {path}: "
            + "\n".join(result.stderr.strip().splitlines()[-8:])
        )
    assemblies: dict[str, list[str]] = {}
    current = None
    for line in result.stdout.splitlines():
        match = _SYMBOL_HEADER.match(line)
        if match:
            name = match.group(1)
            if name in function_symbols:
                current = name
                assemblies.setdefault(current, [line])
            elif not (current is not None and (name.startswith("$") or "+0x" in name)):
                current = None
            continue
        if current is not None:
            assemblies[current].append(line)
    return {name: "\n".join(lines) for name, lines in assemblies.items()}


def assembly_skeleton(
    payload: bytes, relocs: dict, assembly: str, data: set[int],
) -> tuple[bytes, int, int, int, int, str]:
    """Wall-diagnose skeleton using one precomputed whole-object disassembly."""
    masked = bytearray(payload)
    for offset in relocs:
        masked[offset:offset + 4] = b"\0\0\0\0"
    calls = branches = returns = instructions = 0
    retained = []
    origin = 0
    for line in assembly.splitlines():
        symbol = _SYMBOL_HEADER.match(line)
        if symbol:
            origin = int(line.split(None, 1)[0], 16)
            retained.append(line)
            continue
        if ":" not in line or "\t" not in line:
            retained.append(line)
            continue
        address_field, remainder = line.split(":", 1)
        byte_field, _tab, instruction = remainder.partition("\t")
        encoded = byte_field.split()
        if not encoded or any(
            len(value) != 2 or any(char not in "0123456789abcdefABCDEF" for char in value)
            for value in encoded
        ):
            retained.append(line)
            continue
        try:
            address = int(address_field.strip(), 16)
        except ValueError:
            retained.append(line)
            continue
        address -= origin
        if address < 0 or address >= len(payload):
            continue
        body = " ".join(instruction.split())
        retained.append(f"{address:x}:\t{body}")
        if address in data:
            continue
        instructions += 1
        if wall_diagnose._CALL.search(body):
            calls += 1
        elif wall_diagnose._JCC.search(body):
            branches += 1
        elif wall_diagnose._RET.search(body):
            returns += 1
    return bytes(masked), calls, branches, returns, instructions, "\n".join(retained)


def classify_pair(binding, cache: dict, function_symbols: set[str]) -> tuple[str, dict]:
    unit = binding.unit
    if unit not in cache:
        base_path = wall_diagnose.NORM / "base" / f"{unit}.obj"
        target_path = next((
            path for path in (
                wall_diagnose.NORM / "target" / f"{unit}.c.obj",
                wall_diagnose.NORM / "target" / f"{unit}.obj",
            ) if path.is_file()
        ), None)
        if not base_path.is_file() or target_path is None:
            return "unavailable", {"reason": "normalized pair missing"}
        cache[unit] = tuple(
            (Obj(path), object_assemblies(path, function_symbols))
            for path in (base_path, target_path)
        )
    sides = {}
    for tag, (obj, assemblies) in zip(("base", "target"), cache[unit]):
        payload, relocs, size = wall_diagnose._find_function(obj, binding.name)
        if payload is None:
            return "unavailable", {"reason": f"{tag} function missing"}
        assembly = assemblies.get(binding.name)
        if assembly is None:
            return "unavailable", {"reason": f"{tag} disassembly missing"}
        table = wall_diagnose._jump_table_bytes(relocs, binding.name)
        masked, calls, branches, returns, instructions, assembly = \
            assembly_skeleton(payload, relocs, assembly, table)
        sides[tag] = {
            "payload": payload,
            "relocs": relocs,
            "size": size,
            "masked": masked,
            "calls": calls,
            "branches": branches,
            "returns": returns,
            "instructions": instructions,
            "assembly": assembly,
            "referents": wall_diagnose._referents(relocs),
            "call_targets": sorted(
                name for name, _addend in wall_diagnose._call_targets(relocs, assembly)
            ),
        }
    base, target = sides["base"], sides["target"]
    if base["masked"] == target["masked"] and base["referents"] != target["referents"]:
        classification = "referent"
    elif base["call_targets"] != target["call_targets"]:
        classification = "inline"
    elif (base["branches"], base["returns"]) != (
        target["branches"], target["returns"]
    ):
        classification = "cfg"
    elif base["masked"] != target["masked"]:
        classification = "regalloc"
    else:
        classification = "none"
    return classification, {
        side: {
            key: value for key, value in metrics.items()
            if key not in {"payload", "relocs", "masked", "assembly"}
        }
        for side, metrics in sides.items()
    }


def campaign_priority(row: dict) -> tuple:
    if row["proven"]:
        band = 0
    elif row["cur"] >= 95.0:
        band = 1
    elif row["cur"] >= 90.0:
        band = 2
    else:
        band = 3
    return (
        band,
        CLASS_ORDER[row["classification"]],
        -row["cur"],
        row["hist_max"] if row["hist_max"] is not None else 101.0,
        int(row["rva"], 0),
    )


def classified_candidates(
    *, unit: str | None = None, below: float = 100.0, include_eh: bool = False,
    rvas: set[int] | None = None,
) -> list[dict]:
    root = project_root()
    units = load_units(root)
    bindings = {
        (binding.unit, binding.name): binding
        for binding in resolve().functions
        if binding.name
    }
    symbols_by_unit: dict[str, set[str]] = {}
    for binding_unit, binding_name in bindings:
        symbols_by_unit.setdefault(binding_unit, set()).add(binding_name)
    bank = inventory.baseline_rows()
    rows = []
    raw_rows = [
        raw for raw in inventory.build(unit, below)
        if not rvas or (raw["rva"] and int(raw["rva"], 0) in rvas)
    ]
    active_unit = None
    cache = {}
    for raw in sorted(raw_rows, key=lambda row: row["unit"]):
        if raw["unit"] != active_unit:
            cache.clear()
            active_unit = raw["unit"]
        binding = bindings.get((raw["unit"], raw["symbol"]))
        if binding is None or not raw["rva"]:
            continue
        eh_band = is_eh_band(raw["symbol"])
        if eh_band and not include_eh:
            continue
        unit_cfg = units.get(raw["unit"])
        source = unit_cfg["source"] if unit_cfg else None
        if source is None or not (root / source).is_file():
            classification, evidence = "unavailable", {"reason": "source missing"}
        else:
            classification, evidence = classify_pair(
                binding, cache, symbols_by_unit[binding.unit]
            )
        _best, _hist, source_hash = bank.get(binding.rva, (None, None, ""))
        row = {
            **raw,
            "source": str(source) if source is not None else "",
            "source_hash": source_hash,
            "classification": classification,
            "strategy": (
                "state-replay" if raw["proven"]
                else "state-and-source-islands" if classification in {"regalloc", "cfg", "inline"}
                else "identity-first-plus-islands" if classification == "referent"
                else "exploratory-islands"
            ),
            "eh_band": eh_band,
            "evidence": evidence,
        }
        row["priority"] = campaign_priority(row)
        rows.append(row)
    rows.sort(key=campaign_priority)
    for index, row in enumerate(rows, 1):
        row["rank"] = index
        row["priority"] = list(row["priority"])
    return rows


def write_candidates(rows: list[dict], output: Path | None, as_json: bool, limit: int) -> None:
    if output is not None:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(rows, indent=2) + "\n")
    if as_json:
        print(json.dumps(rows[:limit], indent=2))
        return
    counts = Counter(row["classification"] for row in rows)
    print(
        f"[permute] {len(rows)} source-owned candidate(s): "
        + ", ".join(f"{name}={counts[name]}" for name in CLASS_ORDER if counts[name])
    )
    print(f"{'rank':>4} {'rva':>10} {'hist':>7} {'cur':>8} {'class':>11}  unit/symbol")
    for row in rows[:limit]:
        hist = "?" if row["hist_max"] is None else f"{row['hist_max']:.2f}"
        print(
            f"{row['rank']:4d} {row['rva']:>10} {hist:>7} {row['cur']:8.3f} "
            f"{row['classification']:>11}  {row['unit']}/{row['symbol'][:68]}"
        )


def default_depth(classification: str, proven: bool) -> int:
    return 0 if proven or classification in {"regalloc", "referent", "none"} else 2


def completion_message(results: list[dict], output_root: Path) -> str:
    exact = sum(result["exact"] for result in results)
    improved = sum(
        result["best_score"] is not None and result["baseline_score"] is not None
        and result["best_score"] > result["baseline_score"] + 1e-6
        for result in results
    )
    structural = [
        result["candidate"]["rva"] for result in results
        if result.get("search_route") == "structural"
    ]
    message = (
        f"campaign complete: targets={len(results)} exact={exact} improved={improved}"
    )
    if structural:
        return (
            message
            + f"; only a single compiler island was found for {len(structural)} "
            + "target(s), so their next search should be structural: "
            + ", ".join(structural)
        )
    return message + f"; inspect M-frontiers under {output_root}"


def run_campaign(args: argparse.Namespace) -> int:
    root = project_root()
    wanted = {value - 0x400000 if value >= 0x400000 else value for value in args.rva}
    rows = classified_candidates(unit=args.unit, below=args.below, rvas=wanted or None)
    if wanted:
        selected = [row for row in rows if int(row["rva"], 0) in wanted]
        missing = wanted - {int(row["rva"], 0) for row in selected}
        if missing:
            raise ValueError(
                "RVA(s) absent from the live source-owned inventory: "
                + ", ".join(f"0x{value:x}" for value in sorted(missing))
            )
    else:
        selected = rows[: args.targets]
    stamp = time.strftime("%Y%m%d-%H%M%S")
    output_root = (root / args.output).resolve() if args.output else \
        root / "build/permute-campaign" / stamp
    output_root.mkdir(parents=True, exist_ok=False)
    (output_root / "candidates.json").write_text(json.dumps(rows, indent=2) + "\n")
    results = []
    campaign_document = {
        "schema": 1,
        "configuration": {
            "islands": args.islands,
            "frontier": args.frontier,
            "targets": len(selected),
            "seed": args.seed,
            "state_families": args.state_families,
            "candidate_limit": args.candidate_limit,
        },
        "results": results,
    }
    for index, row in enumerate(selected, 1):
        depth = args.max_depth if args.max_depth is not None else \
            default_depth(row["classification"], row["proven"])
        source = root / row["source"]
        unit_stem = row["unit"].replace("/", "-")
        stem = f"{index:03d}-{unit_stem}-0x{int(row['rva'], 0):06x}"
        manifest = output_root / f"{stem}.manifest.json"
        batch = output_root / f"{stem}.results"
        command = [
            str(source.relative_to(root)), row["rva"],
            "--min-depth", "0", "--max-depth", str(depth),
            "--families", args.families or CLASS_FAMILIES[row["classification"]],
            "--state-trials", str(args.islands),
            "--state-families", args.state_families,
            "--state-seed", hex(args.seed ^ int(row["rva"], 0)),
            "--limit", str(args.candidate_limit),
            "--frontier", str(args.frontier),
            "--continue-after-exact",
            "--wall-time-seconds", str(args.wall_time_seconds),
            "--compile-timeout", str(args.compile_timeout_seconds),
            "-o", str(manifest), "--run", "--batch-output", str(batch),
        ]
        print(
            f"[{index}/{len(selected)}] {row['classification']} {row['unit']} "
            f"{row['rva']} {row['symbol']} islands={args.islands} frontier={args.frontier}",
            flush=True,
        )
        exit_code = 0
        error = ""
        try:
            exit_code = variants_main(
                command, prog="homm1 permute campaign", description=__doc__
            )
        except (OSError, ValueError, SystemExit) as exc:
            exit_code = exc.code if isinstance(exc, SystemExit) and isinstance(exc.code, int) else 2
            error = str(exc)
        summary_path = batch / "results.json"
        summary = json.loads(summary_path.read_text()) if summary_path.is_file() else {}
        result = {
            "candidate": row,
            "exit_code": exit_code,
            "error": error,
            "manifest": str(manifest),
            "results": str(summary_path),
            "baseline_score": summary.get("baseline", {}).get("score"),
            "best_score": (summary.get("best") or {}).get("score"),
            "exact": bool(summary.get("exact_source")),
            "island_count": summary.get("island_count", summary.get("state_count", 0)),
            "search_route": summary.get("search_route"),
            "frontier": summary.get("frontier", []),
        }
        results.append(result)
        (output_root / "campaign.json").write_text(
            json.dumps(campaign_document, indent=2) + "\n"
        )
    print(completion_message(results, output_root))
    return 1 if any(result["exit_code"] for result in results) else 0


@logged
def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="homm1 permute", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command", required=True)
    candidates = sub.add_parser("candidates", help="classify the live source-owned queue")
    candidates.add_argument("--unit")
    candidates.add_argument("--below", type=float, default=100.0)
    candidates.add_argument("--include-eh", action="store_true")
    candidates.add_argument("--limit", type=int, default=40)
    candidates.add_argument("--json", action="store_true")
    candidates.add_argument("--output", type=Path)

    run = sub.add_parser("campaign", help="run N islands and retain an M-solution frontier")
    run.add_argument("--rva", action="append", type=lambda value: int(value, 0), default=[])
    run.add_argument("--unit")
    run.add_argument("--below", type=float, default=100.0)
    run.add_argument("--targets", type=int, default=1)
    run.add_argument("--islands", type=int, default=32)
    run.add_argument("--frontier", type=int, default=4)
    run.add_argument("--candidate-limit", type=int, default=512)
    run.add_argument("--max-depth", type=int)
    run.add_argument("--families")
    run.add_argument("--state-families", default="forest")
    run.add_argument("--seed", type=lambda value: int(value, 0), default=0x47525A54)
    run.add_argument("--compile-timeout-seconds", type=float, default=120.0)
    run.add_argument("--wall-time-seconds", type=float, default=1200.0)
    run.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    if args.command == "candidates":
        if args.limit < 1:
            parser.error("--limit must be positive")
        rows = classified_candidates(
            unit=args.unit, below=args.below, include_eh=args.include_eh,
        )
        write_candidates(rows, args.output, args.json, args.limit)
        return 0
    if min(
        args.targets, args.islands, args.frontier, args.candidate_limit,
        args.compile_timeout_seconds, args.wall_time_seconds,
    ) <= 0:
        parser.error("target/island/frontier/limit/time values must be positive")
    if args.max_depth is not None and args.max_depth < 0:
        parser.error("--max-depth must be non-negative")
    try:
        return run_campaign(args)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))


if __name__ == "__main__":
    raise SystemExit(main())
