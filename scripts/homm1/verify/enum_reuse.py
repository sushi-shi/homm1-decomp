"""homm1.verify.enum_reuse - evaluated enum and constant-reuse census.

Equal integer values are review leads, never proof that two domains are the
same.  This audit combines two views:

* a lexical inventory of every enum block and member under ``src/`` and
  ``include/``, including H1_ENUM_* declarations and inactive/unreferenced
  source; and
* a libclang pass over every project translation unit, which evaluates aliases,
  shifts, negative values, character constants, and implicit increments with
  the target ABI.

The two views must cover one another.  A declaration missing from the evaluated
view is a fatal coverage hole rather than a silently incomplete report.

    homm1 verify enum-reuse                  # write the four derived TSV reports
    homm1 verify enum-reuse --duplicates     # print values declared twice+
    homm1 verify enum-reuse --value 10       # inspect one value
    homm1 verify enum-reuse --json           # machine-readable full census
    homm1 verify enum-reuse --init-ledger    # snapshot the review worklist
"""

from __future__ import annotations

from homm1.core.usage import logged

import argparse
import csv
import json
import multiprocessing
import re
import sys
from collections import Counter, defaultdict
from concurrent.futures import ProcessPoolExecutor
from dataclasses import asdict, dataclass
from itertools import combinations
from pathlib import Path

from homm1.core.paths import BUILD, IMAGE_BUILD, REPO
from homm1.verify.constants import _flags, _require_cl_mode, _source_path
from homm1.verify.srcscan import blank_comments


CDB = IMAGE_BUILD / "clangd/compile_commands.json"
REPORT = IMAGE_BUILD / "gen/enum_reuse.tsv"
COLLISION_REPORT = IMAGE_BUILD / "gen/enum_value_collisions.tsv"
PAIR_REPORT = IMAGE_BUILD / "gen/enum_domain_pairs.tsv"
ROLE_PAIR_REPORT = IMAGE_BUILD / "gen/enum_role_pairs.tsv"
BARE_CONSTANTS = IMAGE_BUILD / "gen/bare_constants.tsv"
LEDGER = REPO / "config/reviews/enum-reuse.tsv"

LEDGER_FIELDS = (
    "source_enum", "members", "decision", "current_enums", "member_reuse",
    "reason",
)
LEDGER_DECISIONS = frozenset(("pending", "retain", "canonical", "reuse"))

_MACRO_BLOCK = re.compile(
    r"\bH1_ENUM_(BEGIN|BEGIN_SPLIT|FLAGS_BEGIN|CONST_BEGIN)"
    r"\(\s*(\w+)\s*(?:,\s*(\w+)\s*)?\)(?P<body>.*?)"
    r"\bH1_ENUM_(?:END|END_SPLIT|FLAGS_END|CONST_END)\(",
    re.S,
)
_RAW_ENUM = re.compile(
    r"\b(?:typedef\s+)?enum\s+(?:(?:class|struct)\s+)?"
    r"(?P<name>[A-Za-z_]\w*)?\s*\{"
)
_MEMBER_NAME = re.compile(r"[A-Za-z_]\w*")


@dataclass(frozen=True)
class Member:
    name: str
    line: int
    column: int
    offset: int
    expression: str


@dataclass(frozen=True)
class Block:
    source_enum: str
    domain: str
    kind: str
    storage: str
    file: str
    line: int
    offset: int
    end_offset: int
    members: tuple[Member, ...]


@dataclass(frozen=True)
class RawConstant:
    value: int
    name: str
    file: str
    line: int
    column: int
    offset: int
    parent_name: str
    parent_file: str
    parent_offset: int
    context: str


@dataclass(frozen=True)
class Constant:
    value: int
    name: str
    file: str
    line: int
    column: int
    offset: int
    source_enum: str
    domain: str
    kind: str
    storage: str
    expression: str
    contexts: tuple[str, ...]


def _project_files(repo: Path):
    for root_name in ("include", "src"):
        root = repo / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.is_file() and path.suffix in (".h", ".hpp", ".inl", ".c", ".cpp"):
                yield path


def _line_column(text: str, offset: int) -> tuple[int, int]:
    line = text.count("\n", 0, offset) + 1
    column = offset - text.rfind("\n", 0, offset)
    return line, column


def _matching_brace(text: str, opening: int) -> int:
    depth = 0
    for offset in range(opening, len(text)):
        if text[offset] == "{":
            depth += 1
        elif text[offset] == "}":
            depth -= 1
            if depth == 0:
                return offset
    raise ValueError(f"unterminated enum opening at byte {opening}")


def _members(text: str, body_start: int, body_end: int) -> tuple[Member, ...]:
    """Split an enum body on top-level commas and retain source offsets."""
    boundaries = []
    start = body_start
    depth = 0
    quote = ""
    escaped = False
    for offset in range(body_start, body_end):
        char = text[offset]
        if quote:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = ""
            continue
        if char in "\"'":
            quote = char
        elif char in "([{":
            depth += 1
        elif char in ")]}" and depth:
            depth -= 1
        elif char == "," and depth == 0:
            boundaries.append((start, offset))
            start = offset + 1
    boundaries.append((start, body_end))

    result = []
    for start, end in boundaries:
        segment = text[start:end]
        match = _MEMBER_NAME.search(segment)
        if match is None:
            continue
        offset = start + match.start()
        line, column = _line_column(text, offset)
        source = text[offset:end].strip()
        expression = source.split("=", 1)[1].strip() if "=" in source else ""
        result.append(Member(match.group(), line, column, offset, expression))
    return tuple(result)


def _source_enum(rel: str, domain: str, line: int, members, seen: Counter) -> str:
    anonymous = members[0].name if members else f"line-{line}"
    base = f"{rel}:{domain}" if domain else f"{rel}:<anonymous:{anonymous}>"
    seen[base] += 1
    return base if seen[base] == 1 else f"{base}#{seen[base]}"


def scan_blocks(*, repo: Path = REPO, paths=None) -> list[Block]:
    """Inventory source enum blocks without preprocessing."""
    blocks = []
    seen: Counter = Counter()
    for path in list(paths) if paths is not None else _project_files(repo):
        # Domains.h's macro machinery names formal parameters, not domains.
        if path.resolve() == (repo / "include/Domains.h").resolve():
            continue
        original = path.read_text(errors="replace")
        text = blank_comments(original)
        rel = str(path.resolve().relative_to(repo.resolve()))
        occupied = []
        for match in _MACRO_BLOCK.finditer(text):
            kind, domain, storage = match.group(1), match.group(2), match.group(3) or ""
            body_start, body_end = match.start("body"), match.end("body")
            line, _ = _line_column(text, match.start())
            members = _members(text, body_start, body_end)
            blocks.append(Block(
                _source_enum(rel, domain, line, members, seen), domain,
                kind.lower(), storage,
                rel, line, match.start(), match.end(), members,
            ))
            occupied.append((match.start(), match.end()))

        for match in _RAW_ENUM.finditer(text):
            if any(start <= match.start() < end for start, end in occupied):
                continue
            line_start = text.rfind("\n", 0, match.start()) + 1
            if text[line_start:match.start()].lstrip().startswith("#"):
                continue
            opening = text.find("{", match.start(), match.end())
            closing = _matching_brace(text, opening)
            domain = match.group("name") or ""
            line, _ = _line_column(text, match.start())
            members = _members(text, opening + 1, closing)
            blocks.append(Block(
                _source_enum(rel, domain, line, members, seen), domain, "raw", "", rel,
                line, match.start(), closing + 1, members,
            ))
    return sorted(blocks, key=lambda block: (block.file, block.offset))


def _scan_entry(payload):
    entry, repo_text = payload
    repo = Path(repo_text)
    import clang.cindex as cidx

    path = _source_path(entry, repo)
    args = _flags(entry)
    try:
        _require_cl_mode(args)
    except RuntimeError as exc:
        return [], f"{path}: {exc}"
    try:
        tu = cidx.Index.create().parse(str(path), args=args)
    except cidx.TranslationUnitLoadError as exc:
        return [], f"{path}: libclang could not load TU: {exc}"
    errors = [diag for diag in tu.diagnostics if diag.severity >= cidx.Diagnostic.Error]
    if errors:
        return [], f"{path}: parse error: {errors[0]}"

    context = str(path.relative_to(repo))
    constants = []
    for node in tu.cursor.walk_preorder():
        if node.kind != cidx.CursorKind.ENUM_CONSTANT_DECL or node.location.file is None:
            continue
        source = Path(node.location.file.name).resolve()
        try:
            rel = str(source.relative_to(repo))
        except ValueError:
            continue
        if not rel.startswith(("src/", "include/")):
            continue
        parent = node.semantic_parent
        parent_file = ""
        parent_offset = -1
        parent_name = ""
        if parent is not None:
            parent_name = parent.spelling
            if parent.location.file is not None:
                parent_path = Path(parent.location.file.name).resolve()
                try:
                    parent_file = str(parent_path.relative_to(repo))
                except ValueError:
                    parent_file = ""
                parent_offset = parent.location.offset
        constants.append(RawConstant(
            node.enum_value, node.spelling, rel, node.location.line,
            node.location.column, node.location.offset, parent_name,
            parent_file, parent_offset, context,
        ))
    return constants, None


def scan_entries(entries: list[dict], *, repo: Path = REPO, jobs: int = 1):
    payloads = [(entry, str(repo.resolve())) for entry in entries]
    rows: dict[tuple[str, int, int], RawConstant] = {}
    contexts: dict[tuple[str, int, int], set[str]] = defaultdict(set)
    errors = []
    if jobs <= 1:
        results = map(_scan_entry, payloads)
        pool = None
    else:
        pool = ProcessPoolExecutor(max_workers=jobs)
        results = pool.map(_scan_entry, payloads)
    try:
        for constants, error in results:
            if error:
                errors.append(error)
                continue
            for constant in constants:
                key = (constant.file, constant.offset, constant.value)
                rows.setdefault(key, constant)
                contexts[key].add(constant.context)
    finally:
        if pool is not None:
            pool.shutdown()
    return rows, contexts, errors


def _join(raw, contexts, blocks):
    by_member = {}
    by_name = defaultdict(list)
    for block in blocks:
        for member in block.members:
            by_member[(block.file, member.offset)] = (block, member)
            by_name[(block.file, member.name)].append((block, member))

    constants = []
    uncovered_ast = []
    covered_members = set()
    for key, constant in raw.items():
        match = by_member.get((constant.file, constant.offset))
        if match is None:
            candidates = by_name.get((constant.file, constant.name), [])
            if len(candidates) == 1:
                match = candidates[0]
        if match is None:
            uncovered_ast.append(constant)
            continue
        block, member = match
        covered_members.add((block.file, member.offset))
        constants.append(Constant(
            constant.value, constant.name, constant.file, constant.line,
            constant.column, constant.offset, block.source_enum, block.domain,
            block.kind, block.storage, member.expression,
            tuple(sorted(contexts[key])),
        ))

    uncovered_source = [
        (block, member)
        for block in blocks
        for member in block.members
        if (block.file, member.offset) not in covered_members
    ]
    constants.sort(key=lambda row: (row.value, row.file, row.offset))
    return constants, uncovered_source, uncovered_ast


def collect(*, cdb: Path = CDB, repo: Path = REPO, jobs: int = 1):
    if not cdb.is_file():
        raise FileNotFoundError(f"{cdb}: no compile database; run homm1 configure")
    entries = json.loads(cdb.read_text())
    entries = [
        entry for entry in entries
        if Path(entry["file"]).suffix in (".c", ".cpp")
        and str(entry["file"]).replace("\\", "/").startswith("src/")
    ]
    if not entries:
        raise RuntimeError(f"{cdb}: no project C++ translation units")
    blocks = scan_blocks(repo=repo)
    raw, contexts, errors = scan_entries(entries, repo=repo, jobs=jobs)
    constants, uncovered_source, uncovered_ast = _join(raw, contexts, blocks)
    return constants, blocks, uncovered_source, uncovered_ast, errors


def _write_tsv(path: Path, fieldnames: tuple[str, ...], rows) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames, dialect="excel-tab",
                                lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def write_report(path: Path, constants: list[Constant]) -> None:
    fields = (
        "value", "hex", "name", "file", "line", "column", "offset",
        "source_enum", "domain", "kind", "storage", "expression", "contexts",
    )
    rows = []
    for constant in constants:
        row = asdict(constant)
        row["hex"] = hex(constant.value)
        row["contexts"] = ";".join(constant.contexts)
        rows.append({name: row[name] for name in fields})
    _write_tsv(path, fields, rows)


def _literal_counts(path: Path) -> dict[int, Counter]:
    result: dict[int, Counter] = defaultdict(Counter)
    if not path.is_file():
        return result
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream, dialect="excel-tab"):
            if row["scope"] != "function-body" or row["value"] == "":
                continue
            value = int(row["value"])
            result[value][row["review_group"]] += 1
    return result


def write_collision_report(path: Path, constants: list[Constant], literals_path: Path) -> None:
    by_value = defaultdict(list)
    for constant in constants:
        by_value[constant.value].append(constant)
    literals = _literal_counts(literals_path)
    fields = (
        "value", "hex", "declarations", "domains", "members",
        "function_literal_sites", "literal_groups",
    )
    rows = []
    for value, declarations in sorted(by_value.items()):
        domains = sorted({row.source_enum for row in declarations})
        literal_groups = literals.get(value, Counter())
        if len(domains) < 2 and not literal_groups:
            continue
        rows.append({
            "value": value,
            "hex": hex(value),
            "declarations": len(declarations),
            "domains": ";".join(domains),
            "members": ";".join(f"{row.name}@{row.file}:{row.line}"
                                for row in declarations),
            "function_literal_sites": sum(literal_groups.values()),
            "literal_groups": ";".join(f"{name}={count}"
                                       for name, count in sorted(literal_groups.items())),
        })
    _write_tsv(path, fields, rows)


def write_pair_report(path: Path, constants: list[Constant]) -> None:
    """Rank enum pairs by shared evaluated values without asserting identity."""
    by_domain = defaultdict(list)
    for constant in constants:
        by_domain[constant.source_enum].append(constant)
    fields = (
        "left", "right", "left_members", "right_members", "shared_values",
        "shared_count", "min_coverage_pct", "exact_value_set",
        "exact_value_sequence",
    )
    rows = []
    for left_name, right_name in combinations(sorted(by_domain), 2):
        left = by_domain[left_name]
        right = by_domain[right_name]
        left_values = {row.value for row in left}
        right_values = {row.value for row in right}
        shared = sorted(left_values & right_values)
        exact_set = left_values == right_values
        if len(shared) < 2 and not exact_set:
            continue
        coverage = 100.0 * len(shared) / min(len(left_values), len(right_values))
        rows.append({
            "left": left_name,
            "right": right_name,
            "left_members": ";".join(f"{row.name}={row.value}" for row in left),
            "right_members": ";".join(f"{row.name}={row.value}" for row in right),
            "shared_values": ";".join(str(value) for value in shared),
            "shared_count": len(shared),
            "min_coverage_pct": f"{coverage:.2f}",
            "exact_value_set": "yes" if exact_set else "no",
            "exact_value_sequence": (
                "yes" if [row.value for row in left] == [row.value for row in right]
                else "no"
            ),
        })
    rows.sort(key=lambda row: (
        row["exact_value_sequence"] != "yes",
        row["exact_value_set"] != "yes",
        -float(row["min_coverage_pct"]),
        -row["shared_count"],
        row["left"],
        row["right"],
    ))
    _write_tsv(path, fields, rows)


def _member_roles(members: list[Constant]) -> dict[tuple[int, tuple[str, ...]], str]:
    """Remove only a prefix common to every member of one source enum."""
    names = [member.name.split("_") for member in members]
    prefix_length = 0
    for parts in zip(*names):
        if len(set(parts)) != 1:
            break
        prefix_length += 1
    if prefix_length == min(map(len, names)):
        prefix_length -= 1
    return {
        (member.value, tuple(parts[prefix_length:])): member.name
        for member, parts in zip(members, names)
    }


def write_role_pair_report(path: Path, constants: list[Constant]) -> None:
    """Shortlist equal values with equal member roles; still only review leads."""
    by_domain = _members_by_enum(constants)
    roles = {name: _member_roles(members) for name, members in by_domain.items()}
    fields = ("left", "right", "matching_roles", "left_count", "right_count", "role_coverage_pct")
    rows = []
    for left, right in combinations(sorted(roles), 2):
        shared = sorted(roles[left].keys() & roles[right].keys())
        if len(shared) < 2:
            continue
        rows.append({
            "left": left,
            "right": right,
            "matching_roles": ";".join(
                f"{value}:{roles[left][value, role]}={roles[right][value, role]}"
                for value, role in shared
            ),
            "left_count": len(by_domain[left]),
            "right_count": len(by_domain[right]),
            "role_coverage_pct": f"{100.0 * len(shared) / min(len(by_domain[left]), len(by_domain[right])):.2f}",
        })
    rows.sort(key=lambda row: (
        -float(row["role_coverage_pct"]),
        -len(row["matching_roles"].split(";")),
        row["left"], row["right"],
    ))
    _write_tsv(path, fields, rows)


def _members_by_enum(constants: list[Constant]) -> dict[str, list[Constant]]:
    result = defaultdict(list)
    for constant in constants:
        result[constant.source_enum].append(constant)
    for members in result.values():
        members.sort(key=lambda row: row.offset)
    return result


def init_ledger(path: Path, constants: list[Constant], blocks: list[Block]) -> None:
    """Snapshot the complete starting worklist; never overwrite review work."""
    if path.exists():
        raise FileExistsError(f"{path}: review ledger already exists")
    by_enum = _members_by_enum(constants)
    rows = []
    for block in blocks:
        members = by_enum.get(block.source_enum, ())
        rows.append({
            "source_enum": block.source_enum,
            "members": ";".join(f"{row.name}={row.value}" for row in members),
            "decision": "pending",
            "current_enums": block.source_enum,
            "member_reuse": "",
            "reason": "",
        })
    _write_tsv(path, LEDGER_FIELDS, rows)


def _parse_members(text: str, *, source_enum: str) -> tuple[dict[str, int], list[str]]:
    members = {}
    findings = []
    for item in filter(None, text.split(";")):
        if "=" not in item:
            findings.append(f"{source_enum}: malformed member snapshot {item!r}")
            continue
        name, value_text = item.rsplit("=", 1)
        try:
            value = int(value_text, 0)
        except ValueError:
            findings.append(f"{source_enum}: invalid value in snapshot {item!r}")
            continue
        if name in members:
            findings.append(f"{source_enum}: duplicate snapshot member {name}")
        members[name] = value
    return members, findings


def _parse_reuse(text: str, *, source_enum: str) -> tuple[dict[str, str], list[str]]:
    """Parse ``OLD=target-source-enum::TARGET`` member provenance entries."""
    reuse = {}
    findings = []
    for item in filter(None, text.split(";")):
        if "=" not in item:
            findings.append(f"{source_enum}: malformed member_reuse {item!r}")
            continue
        old, target = item.split("=", 1)
        if "::" not in target:
            findings.append(
                f"{source_enum}: member_reuse target needs source-enum::member: {item!r}")
            continue
        if old in reuse:
            findings.append(f"{source_enum}: duplicate member_reuse for {old}")
        reuse[old] = target
    return reuse, findings


def check_ledger(path: Path, constants: list[Constant]) -> list[str]:
    """Prove that every starting member has a reviewed, value-preserving home."""
    if not path.is_file():
        return [f"{path}: missing review ledger (run --init-ledger once)"]
    current = {
        f"{row.source_enum}::{row.name}": row.value
        for row in constants
    }
    claimed = set()
    findings = []
    seen = set()
    with path.open(newline="") as stream:
        reader = csv.DictReader(stream, dialect="excel-tab")
        if tuple(reader.fieldnames or ()) != LEDGER_FIELDS:
            return [f"{path}: expected columns {','.join(LEDGER_FIELDS)}"]
        for line, row in enumerate(reader, 2):
            source_enum = row["source_enum"]
            if not source_enum:
                findings.append(f"{path}:{line}: empty source_enum")
                continue
            if source_enum in seen:
                findings.append(f"{path}:{line}: duplicate source_enum {source_enum}")
                continue
            seen.add(source_enum)
            decision = row["decision"]
            if decision not in LEDGER_DECISIONS:
                findings.append(
                    f"{source_enum}: invalid decision {decision!r}")
            if decision == "pending":
                findings.append(f"{source_enum}: review is pending")
            if decision != "pending" and not row["reason"].strip():
                findings.append(f"{source_enum}: reviewed row needs an evidence reason")
            members, row_findings = _parse_members(
                row["members"], source_enum=source_enum)
            findings.extend(row_findings)
            reuse, row_findings = _parse_reuse(
                row["member_reuse"], source_enum=source_enum)
            findings.extend(row_findings)
            unknown = sorted(set(reuse) - set(members))
            if unknown:
                findings.append(
                    f"{source_enum}: member_reuse names absent from snapshot: "
                    + ", ".join(unknown))
            if decision == "reuse" and not reuse:
                findings.append(f"{source_enum}: reuse decision has no member mapping")
            if decision in ("retain", "canonical") and reuse:
                findings.append(
                    f"{source_enum}: {decision} decision cannot redirect members")

            target_enums = set()
            for name, value in members.items():
                target = reuse.get(name, f"{source_enum}::{name}")
                target_enum, separator, target_name = target.rpartition("::")
                if not separator or not target_enum or not target_name:
                    findings.append(
                        f"{source_enum}: invalid target for {name}: {target!r}")
                    continue
                target_enums.add(target_enum)
                if target not in current:
                    findings.append(
                        f"{source_enum}: {name} has no current target {target}")
                elif current[target] != value:
                    findings.append(
                        f"{source_enum}: {name}={value} maps to {target}="
                        f"{current[target]}")
                claimed.add(target)
            declared_enums = set(filter(None, row["current_enums"].split(";")))
            if target_enums != declared_enums:
                findings.append(
                    f"{source_enum}: current_enums is {sorted(declared_enums)}, "
                    f"member targets use {sorted(target_enums)}")

    unclaimed = sorted(set(current) - claimed)
    if unclaimed:
        preview = ", ".join(unclaimed[:10])
        suffix = " ..." if len(unclaimed) > 10 else ""
        findings.append(
            f"{len(unclaimed)} current member(s) have no starting-ledger provenance: "
            f"{preview}{suffix}")
    return findings


def _print_tsv(constants: list[Constant]) -> None:
    writer = csv.writer(sys.stdout, delimiter="\t", lineterminator="\n")
    writer.writerow(("value", "hex", "name", "file", "line", "domain", "kind"))
    for row in constants:
        writer.writerow((row.value, hex(row.value), row.name, row.file, row.line,
                         row.domain, row.kind))


@logged
def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        prog="homm1 verify enum-reuse", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--value", action="append", type=lambda text: int(text, 0),
                        default=[], help="evaluated integer (repeatable)")
    parser.add_argument("--duplicates", action="store_true",
                        help="print only values declared in two or more domains")
    parser.add_argument("--json", action="store_true",
                        help="print the selected constants as JSON")
    parser.add_argument("--no-report", action="store_true",
                        help="do not write build/gen derived reports")
    parser.add_argument("--init-ledger", action="store_true",
                        help="create the complete pending review ledger; refuse overwrite")
    parser.add_argument("--jobs", type=int,
                        default=min(4, multiprocessing.cpu_count()),
                        help="parallel libclang translation-unit workers")
    args = parser.parse_args(argv)
    try:
        constants, blocks, uncovered_source, uncovered_ast, errors = collect(
            jobs=max(1, args.jobs))
    except (FileNotFoundError, RuntimeError, ValueError) as exc:
        print(f"[enum-reuse] FATAL: {exc}")
        return 2
    if errors or uncovered_source or uncovered_ast:
        for error in errors[:10]:
            print(f"   {error}")
        for block, member in uncovered_source[:10]:
            print(f"   {block.file}:{member.line}: source member not evaluated: "
                  f"{block.domain}::{member.name}")
        for row in uncovered_ast[:10]:
            print(f"   {row.file}:{row.line}: evaluated member not inventoried: "
                  f"{row.parent_name}::{row.name}")
        total = len(errors) + len(uncovered_source) + len(uncovered_ast)
        print(f"[enum-reuse] FATAL: {total} coverage/parsing finding(s)")
        return 2

    by_value = Counter(row.value for row in constants)
    domains_by_value = defaultdict(set)
    for row in constants:
        domains_by_value[row.value].add(row.source_enum)
    selected = [
        row for row in constants
        if (not args.value or row.value in args.value)
        and (not args.duplicates or len(domains_by_value[row.value]) > 1)
    ]
    if args.json:
        print(json.dumps({
            "blocks": len(blocks),
            "constants": [asdict(row) for row in selected],
        }, indent=2))
    elif args.value or args.duplicates:
        _print_tsv(selected)
    if not args.no_report:
        write_report(REPORT, constants)
        write_collision_report(COLLISION_REPORT, constants, BARE_CONSTANTS)
        write_pair_report(PAIR_REPORT, constants)
        write_role_pair_report(ROLE_PAIR_REPORT, constants)
    if args.init_ledger:
        try:
            init_ledger(LEDGER, constants, blocks)
        except FileExistsError as exc:
            print(f"[enum-reuse] FATAL: {exc}")
            return 2
        print(f"[enum-reuse] initialized {LEDGER.relative_to(REPO)} with "
              f"{len(blocks)} pending row(s)", file=sys.stderr)
    findings = check_ledger(LEDGER, constants)
    for finding in findings[:20]:
        print(f"   {finding}", file=sys.stderr)
    if len(findings) > 20:
        print(f"   ... {len(findings) - 20} more", file=sys.stderr)
    duplicate_values = sum(len(domains) > 1 for domains in domains_by_value.values())
    print(f"[enum-reuse] {len(blocks)} block(s), {len(constants)} member(s), "
          f"{len(by_value)} value(s), {duplicate_values} cross-domain collision value(s)",
          file=sys.stderr)
    if not args.no_report:
        print(f"[enum-reuse] reports: {REPORT.relative_to(REPO)}, "
              f"{COLLISION_REPORT.relative_to(REPO)}, "
              f"{PAIR_REPORT.relative_to(REPO)}, "
              f"{ROLE_PAIR_REPORT.relative_to(REPO)}", file=sys.stderr)
    if findings:
        print(f"[enum-reuse] FAIL: {len(findings)} ledger finding(s)", file=sys.stderr)
        return 1
    with LEDGER.open(newline="") as stream:
        reviewed = sum(1 for _row in csv.DictReader(stream, dialect="excel-tab"))
    print(f"[enum-reuse] OK: all {reviewed} starting block(s) reviewed and "
          "all current members accounted for", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
