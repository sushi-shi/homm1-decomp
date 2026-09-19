"""Align the HoMM1 function census with HoMM2 symbol-bearing donors.

This is a locating tool, not a source-claim provider.  It records the evidence
used to create ordinary ``VA`` source stubs under ``src``; labels continue to
enter the build through the same source path as reconstructed functions.

The preferred donor is the Buka 2.1 symbol model.  The PoL 2.0 CodeView model
is accepted as a second donor because it was built with the same VC4 family as
HoMM1 and often retains a closer function shape.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import math
import re
import struct
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path

from capstone import CS_ARCH_X86, CS_MODE_32, Cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG


@dataclass(frozen=True)
class Section:
    rva: int
    raw: int
    virtual_size: int
    raw_size: int


class PE:
    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[pe:pe + 4] != b"PE\0\0":
            raise ValueError(f"not a PE image: {path}")
        sections = struct.unpack_from("<H", self.data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", self.data, pe + 20)[0]
        optional = pe + 24
        self.image_base = struct.unpack_from("<I", self.data, optional + 28)[0]
        table = optional + optional_size
        self.sections: list[Section] = []
        for index in range(sections):
            off = table + index * 40
            virtual_size, rva, raw_size, raw = struct.unpack_from(
                "<IIII", self.data, off + 8
            )
            self.sections.append(Section(rva, raw, virtual_size, raw_size))

    def offset(self, rva: int) -> int | None:
        for section in self.sections:
            extent = max(section.virtual_size, section.raw_size)
            if section.rva <= rva < section.rva + extent:
                delta = rva - section.rva
                if delta < section.raw_size:
                    return section.raw + delta
                return None
        return None

    def read(self, rva: int, size: int) -> bytes:
        off = self.offset(rva)
        return b"" if off is None else self.data[off:off + size]

    def cstring(self, va: int) -> str | None:
        rva = va - self.image_base
        off = self.offset(rva)
        if off is None:
            return None
        end = self.data.find(b"\0", off, min(len(self.data), off + 256))
        if end < 0 or end - off < 4:
            return None
        blob = self.data[off:end]
        if any(byte < 0x20 or byte >= 0x7F for byte in blob):
            return None
        return blob.decode("ascii")


@dataclass
class Function:
    rva: int
    size: int
    name: str
    unit: str
    donor: str
    signature: str = ""
    tokens: tuple[str, ...] = ()
    mnemonics: tuple[str, ...] = ()
    strings: tuple[str, ...] = ()
    calls: int = 0
    grams: frozenset[tuple[str, ...]] = frozenset()
    callees: tuple[int, ...] = ()


def _integer(value: str) -> int:
    return int(value, 0)


def homm1_functions(path: Path) -> list[Function]:
    starts: list[int] = []
    with path.open(newline="") as stream:
        lines = (line for line in stream if not line.lstrip().startswith("#"))
        for row in csv.DictReader(lines, delimiter="\t"):
            starts.append(_integer(row["rva"]))
    starts.sort()
    return [
        Function(rva, starts[index + 1] - rva if index + 1 < len(starts) else 1,
                 f"sub_{rva:08x}", "", "homm1")
        for index, rva in enumerate(starts)
    ]


def buka21_functions(path: Path) -> list[Function]:
    out = []
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream):
            if row["kind"] != "func":
                continue
            out.append(Function(_integer(row["rva"]), _integer(row["size"]),
                                row["name"], row["unit"], "buka21"))
    return sorted(out, key=lambda function: function.rva)


_MARKER = re.compile(r"^RVA\((0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+)\)\s*$")


def pol20_functions(root: Path) -> list[Function]:
    out = []
    for path in sorted(root.rglob("*.cpp")):
        if "_external" in path.parts:
            # Keep CRT/middleware names from Buka's reviewed providers.  The
            # NWC carcass is the useful cross-game source correspondence.
            continue
        lines = path.read_text(errors="replace").splitlines()
        index = 0
        while index < len(lines):
            marker = _MARKER.match(lines[index].strip())
            if marker is None:
                index += 1
                continue
            signature = lines[index + 1].strip() if index + 1 < len(lines) else ""
            while signature.endswith(",") and index + 2 < len(lines):
                index += 1
                signature += " " + lines[index + 1].strip()
            name = signature[:-1].strip() if signature.endswith(";") else signature
            out.append(Function(int(marker.group(1), 16) - 0x400000,
                                int(marker.group(2), 16), name,
                                str(path.relative_to(root).with_suffix("")),
                                "pol20", signature))
            index += 2
    return sorted(out, key=lambda function: function.rva)


def _small(value: int) -> str:
    signed = value if value < 0x80000000 else value - 0x100000000
    return str(signed) if -0x1000 <= signed <= 0x1000 else "#"


def analyze(pe: PE, functions: list[Function]) -> None:
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    starts = {function.rva for function in functions}
    for function in functions:
        tokens: list[str] = []
        mnemonics: list[str] = []
        strings: set[str] = set()
        calls = 0
        callees: set[int] = set()
        for instruction in decoder.disasm(pe.read(function.rva, function.size),
                                          pe.image_base + function.rva):
            mnemonic = instruction.mnemonic
            mnemonics.append(mnemonic)
            if mnemonic == "call":
                calls += 1
            operands = []
            for operand in instruction.operands:
                if operand.type == X86_OP_REG:
                    operands.append(instruction.reg_name(operand.reg))
                elif operand.type == X86_OP_IMM:
                    if mnemonic == "call":
                        target_rva = (operand.imm & 0xFFFFFFFF) - pe.image_base
                        if target_rva in starts:
                            callees.add(target_rva)
                    text = pe.cstring(operand.imm & 0xFFFFFFFF)
                    if text is not None:
                        strings.add(text)
                        operands.append("str")
                    elif mnemonic.startswith("j") or mnemonic == "call":
                        operands.append("target")
                    else:
                        operands.append(_small(operand.imm & 0xFFFFFFFF))
                elif operand.type == X86_OP_MEM:
                    memory = operand.mem
                    base = instruction.reg_name(memory.base) if memory.base else ""
                    index = instruction.reg_name(memory.index) if memory.index else ""
                    displacement = memory.disp & 0xFFFFFFFF
                    text = pe.cstring(displacement) if not base and not index else None
                    if text is not None:
                        strings.add(text)
                        disp = "str"
                    elif not base and not index and pe.offset(displacement - pe.image_base) is not None:
                        disp = "addr"
                    else:
                        disp = _small(displacement)
                    operands.append(f"[{base}:{index}:{memory.scale}:{disp}]")
            tokens.append(mnemonic + (":" + ",".join(operands) if operands else ""))
        function.tokens = tuple(tokens)
        function.mnemonics = tuple(mnemonics)
        function.strings = tuple(sorted(strings))
        function.calls = calls
        function.grams = frozenset(_ngrams(function.mnemonics))
        function.callees = tuple(sorted(callees))


def _digest(values: tuple[str, ...]) -> str:
    return hashlib.sha1("\n".join(values).encode()).hexdigest()


def _ngrams(values: tuple[str, ...], width: int = 3) -> set[tuple[str, ...]]:
    return {values[index:index + width]
            for index in range(max(0, len(values) - width + 1))}


def _jaccard(left: set, right: set) -> float:
    union = left | right
    return len(left & right) / len(union) if union else 0.0


def similarity(left: Function, right: Function) -> tuple[float, str]:
    if left.tokens and left.tokens == right.tokens:
        return 1.0, "normalized-instructions"
    if left.mnemonics and left.mnemonics == right.mnemonics:
        return 0.97, "instruction-sequence"
    shape = _jaccard(left.grams, right.grams)
    size = math.exp(-abs(math.log(max(left.size, 1) / max(right.size, 1))))
    count = math.exp(-abs(math.log(max(len(left.mnemonics), 1) /
                                  max(len(right.mnemonics), 1))))
    shared = set(left.strings) & set(right.strings)
    string_score = 1.0 if shared else 0.0
    call_score = 1.0 - min(abs(left.calls - right.calls) / max(left.calls, right.calls, 1), 1.0)
    score = 0.45 * shape + 0.18 * size + 0.12 * count + 0.15 * string_score + 0.10 * call_score
    reasons = [f"shape={shape:.3f}", f"size={size:.3f}", f"calls={call_score:.3f}"]
    if shared:
        reasons.append("strings=" + "|".join(sorted(shared)[:3]))
    return score, ";".join(reasons)


def candidates(targets: list[Function], donors: list[Function]) -> list[dict[str, str]]:
    exact_tokens: dict[str, list[Function]] = defaultdict(list)
    exact_mnemonics: dict[str, list[Function]] = defaultdict(list)
    string_index: dict[str, set[int]] = defaultdict(set)
    grams = []
    for index, donor in enumerate(donors):
        exact_tokens[_digest(donor.tokens)].append(donor)
        exact_mnemonics[_digest(donor.mnemonics)].append(donor)
        for value in donor.strings:
            string_index[value].add(index)
        grams.append(_ngrams(donor.mnemonics))

    rows = []
    for target in targets:
        pool: set[int] = set()
        token_hits = exact_tokens.get(_digest(target.tokens), [])
        mnemonic_hits = exact_mnemonics.get(_digest(target.mnemonics), [])
        for donor in token_hits + mnemonic_hits:
            pool.add(donors.index(donor))
        for value in target.strings:
            pool.update(string_index.get(value, ()))
        if not pool:
            target_grams = _ngrams(target.mnemonics)
            ranked = sorted(range(len(donors)),
                            key=lambda index: _jaccard(target_grams, grams[index]),
                            reverse=True)
            pool.update(ranked[:12])
        scored = sorted((similarity(target, donors[index])[0], index,
                         similarity(target, donors[index])[1]) for index in pool)
        best_score, best_index, evidence = scored[-1]
        runner = scored[-2][0] if len(scored) > 1 else 0.0
        donor = donors[best_index]
        rows.append({
            "rva": f"0x{target.rva:08x}",
            "size": f"0x{target.size:x}",
            "donor": donor.donor,
            "donor_rva": f"0x{donor.rva:08x}",
            "symbol": donor.name,
            "unit": donor.unit,
            "score": f"{best_score:.6f}",
            "margin": f"{best_score - runner:.6f}",
            "target_strings": "|".join(target.strings),
            "evidence": evidence,
        })
    return rows


_REVIEWED_ANCHORS = {
    "buka21": {
        0x4F640: 0x65BF0,  # PollSound
        0x4F6B2: 0x65D05,  # ForcePollSound
        0x5A584: 0x8DF6F,  # PollRemote
        0x5C15C: 0x71883,  # AppAbout
        0x5DC9B: 0x71FF8,  # KBTickCount
        0x78940: 0xB65E0,  # soundManager::PollSound
    },
    "pol20": {
        0x4F640: 0x96450,
        0x4F6B2: 0x9659E,
        0x5C15C: 0x1C70E,
        0x5DC9B: 0x1D011,
    },
}


def graph_matches(targets: list[Function], donors: list[Function]) -> list[dict[str, str]]:
    """Grow unique machine-code anchors through the direct call graph.

    A match is deliberately one-to-one.  Exact bodies seed the graph; reviewed
    cross-version anchors cover functions known to have changed substantially.
    Each propagation round then rewards candidates that call, or are called by,
    already corresponding functions.
    """
    donor_kind = donors[0].donor
    target_by_rva = {function.rva: function for function in targets}
    donor_by_rva = {function.rva: function for function in donors}
    target_callers: dict[int, set[int]] = defaultdict(set)
    donor_callers: dict[int, set[int]] = defaultdict(set)
    for function in targets:
        for callee in function.callees:
            target_callers[callee].add(function.rva)
    for function in donors:
        for callee in function.callees:
            donor_callers[callee].add(function.rva)

    mapping: dict[int, int] = {}
    evidence: dict[int, str] = {}
    used: set[int] = set()
    for target_rva, donor_rva in _REVIEWED_ANCHORS.get(donor_kind, {}).items():
        if target_rva in target_by_rva and donor_rva in donor_by_rva:
            mapping[target_rva] = donor_rva
            used.add(donor_rva)
            evidence[target_rva] = "reviewed-anchor"

    for attribute, label, minimum in (("tokens", "unique-normalized", 3),
                                      ("mnemonics", "unique-instructions", 5)):
        left: dict[str, list[Function]] = defaultdict(list)
        right: dict[str, list[Function]] = defaultdict(list)
        for function in targets:
            values = getattr(function, attribute)
            if len(values) >= minimum:
                left[_digest(values)].append(function)
        for function in donors:
            values = getattr(function, attribute)
            if len(values) >= minimum:
                right[_digest(values)].append(function)
        for digest, target_rows in left.items():
            donor_rows = right.get(digest, [])
            if len(target_rows) == len(donor_rows) == 1:
                target, donor = target_rows[0], donor_rows[0]
                if target.rva not in mapping and donor.rva not in used:
                    mapping[target.rva] = donor.rva
                    used.add(donor.rva)
                    evidence[target.rva] = label

    # Feature shortlist: all shared-string candidates plus the best structural
    # functions.  It is stable across rounds; graph evidence changes.
    string_index: dict[str, set[int]] = defaultdict(set)
    for index, donor in enumerate(donors):
        for value in donor.strings:
            string_index[value].add(index)
    shortlists: dict[int, list[int]] = {}
    for target in targets:
        pool: set[int] = set()
        for value in target.strings:
            pool.update(string_index.get(value, ()))
        structural = sorted(
            range(len(donors)),
            key=lambda index: _jaccard(target.grams, donors[index].grams),
            reverse=True,
        )[:20]
        pool.update(structural)
        shortlists[target.rva] = list(pool)

    for _round in range(12):
        proposals = []
        for target in targets:
            if target.rva in mapping:
                continue
            mapped_out = {mapping[callee] for callee in target.callees
                          if callee in mapping}
            mapped_in = {mapping[caller] for caller in target_callers[target.rva]
                         if caller in mapping}
            scored = []
            for index in shortlists[target.rva]:
                donor = donors[index]
                if donor.rva in used:
                    continue
                base, detail = similarity(target, donor)
                out_hits = len(mapped_out & set(donor.callees))
                in_hits = len(mapped_in & donor_callers[donor.rva])
                graph = out_hits * 0.24 + in_hits * 0.20
                scored.append((base + graph, base, out_hits + in_hits,
                               donor.rva, detail))
            if not scored:
                continue
            scored.sort()
            best = scored[-1]
            runner = scored[-2][0] if len(scored) > 1 else 0.0
            total, base, graph_hits, donor_rva, detail = best
            accept = ((total >= 0.92 and total - runner >= 0.08)
                      or (graph_hits >= 2 and base >= 0.34 and
                          total - runner >= 0.04)
                      or (graph_hits >= 3 and base >= 0.25))
            if accept:
                proposals.append((total - runner, total, target.rva, donor_rva,
                                  base, graph_hits, detail))
        if not proposals:
            break
        added = 0
        for margin, total, target_rva, donor_rva, base, graph_hits, detail in \
                sorted(proposals, reverse=True):
            if target_rva in mapping or donor_rva in used:
                continue
            mapping[target_rva] = donor_rva
            used.add(donor_rva)
            evidence[target_rva] = (f"graph:{graph_hits};base={base:.6f};"
                                    f"margin={margin:.6f};{detail}")
            added += 1
        if not added:
            break

    rows = []
    for target_rva in sorted(mapping):
        target, donor = target_by_rva[target_rva], donor_by_rva[mapping[target_rva]]
        rows.append({
            "rva": f"0x{target.rva:08x}",
            "size": f"0x{target.size:x}",
            "donor": donor_kind,
            "donor_rva": f"0x{donor.rva:08x}",
            "symbol": donor.name,
            "unit": donor.unit,
            "evidence": evidence[target_rva],
        })
    return rows


def write(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]), delimiter="\t",
                                lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def _path_unit(function: Function, units: list[str]) -> str | None:
    """Unit named by an embedded retail ``__FILE__`` literal, if present."""
    by_stem = defaultdict(list)
    for unit in units:
        by_stem[Path(unit).name.lower()].append(unit)
    for value in function.strings:
        match = re.search(r"(?:^|[\\/])([^\\/]+)\.(?:cpp|c)$", value, re.I)
        if match:
            choices = by_stem.get(match.group(1).lower(), [])
            if len(choices) == 1:
                return choices[0]
    return None


def infer_segments(targets: list[Function], donors: list[Function], path: Path,
                   cutoff: int = 0x80188) -> None:
    """Infer contiguous HoMM1 TU runs with a stay-biased Viterbi pass.

    The state is donor TU ownership, not donor link position.  That distinction
    matters because HoMM1, PoL 2.0 and Buka 2.1 linked the same NWC objects in
    different global orders while preserving function order inside each object.
    """
    targets = [function for function in targets if function.rva < cutoff]
    grouped: dict[str, list[Function]] = defaultdict(list)
    for donor in donors:
        grouped[donor.unit].append(donor)
    units = sorted(grouped)
    emissions: list[list[float]] = []
    reasons: list[list[str]] = []
    for target in targets:
        forced = _path_unit(target, units)
        scores, why = [], []
        for unit in units:
            score, donor = max(
                ((similarity(target, donor)[0], donor) for donor in grouped[unit]),
                key=lambda item: item[0],
            )
            reason = donor.name
            if target.tokens == donor.tokens and target.tokens:
                score += 1.4
                reason = "exact:" + reason
            elif target.mnemonics == donor.mnemonics and target.mnemonics:
                score += 1.0
                reason = "instructions:" + reason
            shared = set(target.strings) & set(donor.strings)
            if shared:
                score += min(0.8, 0.2 * len(shared))
                reason = "strings:" + reason
            if forced == unit:
                score += 4.0
                reason = "source-path:" + reason
            elif forced is not None:
                score -= 4.0
            scores.append(score)
            why.append(reason)
        emissions.append(scores)
        reasons.append(why)

    # A switch costs roughly one strong structural observation.  This keeps
    # generic wrappers inside a run while source paths/exact functions still
    # establish sharp boundaries.
    switch = 0.72
    previous = emissions[0][:]
    back: list[list[int]] = [[-1] * len(units)]
    for row in emissions[1:]:
        best_index = max(range(len(units)), key=previous.__getitem__)
        best_value = previous[best_index]
        current, choice = [], []
        for state, emission in enumerate(row):
            stay = previous[state]
            change = best_value - switch
            if stay >= change:
                current.append(stay + emission)
                choice.append(state)
            else:
                current.append(change + emission)
                choice.append(best_index)
        previous = current
        back.append(choice)
    states = [max(range(len(units)), key=previous.__getitem__)]
    for index in range(len(targets) - 1, 0, -1):
        states.append(back[index][states[-1]])
    states.reverse()

    rows = []
    begin = 0
    for index in range(1, len(targets) + 1):
        if index < len(targets) and states[index] == states[begin]:
            continue
        state = states[begin]
        rows.append({
            "start_rva": f"0x{targets[begin].rva:08x}",
            "end_rva": f"0x{(targets[index].rva if index < len(targets) else cutoff):08x}",
            "functions": str(index - begin),
            "unit": units[state],
            "first_evidence": reasons[begin][state],
            "last_evidence": reasons[index - 1][state],
        })
        begin = index
    # Two sparse stretches are ambiguous to the Viterbi pass because Buka's
    # generic BASE/Misc helpers resemble the intervening code. Function names
    # and adjacency settle the HoMM1 ownership: its remote block begins with
    # WriteModemPacket, while the late window block continues through
    # KBTickCount. Keeping BASE/Misc for either would make one object occupy
    # disjoint linker spans.
    reviewed = {
        (0x5A16B, 0x5B659): "SOURCE/REMOTE",
        (0x5C9FE, 0x5DDE0): "SOURCE/kbwin",
    }
    for row in rows:
        bounds = (int(row["start_rva"], 0), int(row["end_rva"], 0))
        if bounds in reviewed:
            row["unit"] = reviewed[bounds]
            row["first_evidence"] = "reviewed-order:" + row["first_evidence"]
            row["last_evidence"] = "reviewed-order:" + row["last_evidence"]
    write(path, rows)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--homm1-exe", type=Path, required=True)
    parser.add_argument("--homm1-functions", type=Path, required=True)
    parser.add_argument("--buka21-exe", type=Path)
    parser.add_argument("--buka21-symbols", type=Path)
    parser.add_argument("--pol20-exe", type=Path)
    parser.add_argument("--pol20-carcass", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--segments-output", type=Path)
    parser.add_argument("--graph-output", type=Path)
    args = parser.parse_args(argv)

    targets = homm1_functions(args.homm1_functions)
    analyze(PE(args.homm1_exe), targets)
    donor_sets: list[list[Function]] = []
    if args.buka21_exe and args.buka21_symbols:
        donor_sets.append(buka21_functions(args.buka21_symbols))
        analyze(PE(args.buka21_exe), donor_sets[-1])
    if args.pol20_exe and args.pol20_carcass:
        donor_sets.append(pol20_functions(args.pol20_carcass))
        analyze(PE(args.pol20_exe), donor_sets[-1])
    if not donor_sets:
        parser.error("at least one donor executable and symbol model is required")
    if args.segments_output:
        primary = next((items for items in donor_sets
                        if items and items[0].donor == "buka21"), None)
        if primary is None:
            primary = next((items for items in donor_sets
                            if items and items[0].donor == "pol20"), None)
        assert primary is not None
        infer_segments(targets, primary, args.segments_output)

    combined: dict[int, list[dict[str, str]]] = defaultdict(list)
    for donor_set in donor_sets:
        for row in candidates(targets, donor_set):
            combined[int(row["rva"], 16)].append(row)
    rows = []
    for target in targets:
        choices = sorted(combined[target.rva],
                         key=lambda row: (float(row["score"]),
                                          row["donor"] == "buka21"), reverse=True)
        best = choices[0]
        if len(choices) > 1:
            other = choices[1]
            best["evidence"] += (f";alternate={other['donor']}:{other['symbol']}"
                                 f"@{other['donor_rva']}:{other['score']}")
        rows.append(best)
    write(args.output, rows)
    if args.graph_output:
        graph_by_rva: dict[int, list[dict[str, str]]] = defaultdict(list)
        for donor_set in donor_sets:
            for row in graph_matches(targets, donor_set):
                graph_by_rva[int(row["rva"], 16)].append(row)
        graph_rows = []
        for rva in sorted(graph_by_rva):
            choices = sorted(
                graph_by_rva[rva],
                key=lambda row: (row["donor"] == "buka21",
                                 row["evidence"] == "reviewed-anchor"),
                reverse=True,
            )
            best = choices[0]
            if len(choices) > 1:
                alternate = choices[1]
                best["evidence"] += (f";alternate={alternate['donor']}:"
                                     f"{alternate['symbol']}@{alternate['donor_rva']}")
            graph_rows.append(best)
        write(args.graph_output, graph_rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
