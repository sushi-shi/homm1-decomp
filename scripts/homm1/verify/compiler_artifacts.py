"""Reject source-written stand-ins for compiler-generated C++ machinery.

The compiler owns allocation calls, deleting destructors, vtables/RTTI, static
initialization helpers and EH/vector helpers.  A few source-level lifetime
operations remain real: placement construction, destructor-only calls over raw
storage, the source-proven ZTools placement allocation overload, and an original
typed collection's destructor callback before the collection separately
deallocates the object. Their path/type/count signatures
are closed here so a new site is reviewed instead of silently joining that
exception. Recorded rule exceptions (docs/todos/rule-exceptions.tsv) are
admitted the same way, by exact path and count.

``homm1 verify compiler-artifacts`` also prints external code definitions that
exist only in base objects.  That list is derived from the current COFFs and
objdiff report; suspicious realization/emission helpers are fatal, while normal
template and library COMDATs remain an investigation report.
"""

from __future__ import annotations

from homm1.core.usage import logged

import json
import re
import struct
import sys
from collections import Counter
from pathlib import Path

from homm1.core.coff import Coff, IMAGE_SCN_CNT_CODE
from homm1.core.paths import BUILD
from homm1.verify.srcscan import blank_comments, rel, source_files


# Reviewed allowances, one entry per (file, construct): each is a rule
# exception with a row in docs/todos/rule-exceptions.tsv. Empty until one is
# admitted.
PLACEMENT_ALLOW = Counter()

ALLOCATION_DEFINITION_ALLOW = Counter()

# The complete authored definition of a project placement new, not a call or
# an arbitrary allocator override.
ZTOOLS_PLACEMENT_DEFINITION_RE = re.compile(
    r"\binline\s+void\s*\*\s*operator\s+new\s*\(\s*"
    r"size_t\s+size\s*,\s*void\s*\*\s*ptr\s*,\s*"
    r"int\s+dummy1\s*,\s*int\s+dummy2\s*\)\s*"
    r"\{\s*return\s+ptr\s*;\s*\}"
)

DTOR_CALL_ALLOW = Counter()

ALLOCATION_CALL_ALLOW = Counter()

LOW_LEVEL_ALLOW = Counter()

OPERATOR_CALL_RE = re.compile(
    r"(?<![A-Za-z_])(?:::)?operator\s+(?:new|delete)(?:\s*\[\s*\])?\s*\("
)
PLACEMENT_RE = re.compile(
    r"(?<![A-Za-z_])(?:::)?new\s*\([^;\n]*\)\s*([A-Za-z_]\w*)"
)
DTOR_CALL_RE = re.compile(
    r"(?:->|\.)\s*(?:[A-Za-z_]\w*::)?~([A-Za-z_]\w*)\s*\("
)
CTOR_CALL_RE = re.compile(r"(?:->|\.)\s*([A-Za-z_]\w*)\s*::\s*\1\s*\(")
FORCE_HELPER_RE = re.compile(
    r"\b(Realize[A-Z]\w*|ForceEmit\w*|EmitCompiler\w*)\s*"
    r"\([^;{}]*\)\s*\{",
    re.DOTALL,
)
STATIC_INIT_RE = re.compile(r"\b(?:atexit|_atexit|_onexit)\s*\(")


def instantiation_only(text: str) -> bool:
    """Recognize emission-only source files, not instantiations beside real code.

    Includes, address bindings and explicit class instantiations do not establish
    an authored TU. Leave storage definitions and ordinary implementations alone.
    """
    text = blank_comments(text)
    text = re.sub(r'^\s*#include\s*[<"][^>"\n]+[>"]\s*$', '', text, flags=re.M)
    text = re.sub(r'\bRVA_COMPGEN\([^\n]*\)\s*;?', '', text)
    text, count = re.subn(r'\btemplate\s+(?:class|struct)\s+[^;{}]+;', '', text)
    return count > 0 and not text.strip()


def _counter_findings(label: str, actual: Counter, allowed: Counter) -> list[str]:
    out = []
    for key in sorted(set(actual) | set(allowed)):
        got, want = actual[key], allowed[key]
        if got != want:
            path, kind = key
            out.append(f"{label}: {path}: {kind}: found {got}, expected {want}")
    return out


def source_findings(files=None, *, placement_allow=PLACEMENT_ALLOW,
                    dtor_allow=DTOR_CALL_ALLOW,
                    low_level_allow=LOW_LEVEL_ALLOW,
                    allocation_definition_allow=ALLOCATION_DEFINITION_ALLOW,
                    allocation_call_allow=ALLOCATION_CALL_ALLOW) -> list[str]:
    placements: Counter = Counter()
    allocation_definitions: Counter = Counter()
    allocation_calls: dict[tuple[str, str], list[int]] = {}
    dtor_calls: Counter = Counter()
    low_level: Counter = Counter()
    findings: list[str] = []
    paths = files if files is not None else source_files()
    for path in paths:
        text = blank_comments(path.read_text(errors="replace"))
        site = rel(path)
        if path.suffix in (".cpp", ".cc", ".cxx") and instantiation_only(text):
            findings.append(f"instantiation-only translation unit: {site}")
        definitions = list(ZTOOLS_PLACEMENT_DEFINITION_RE.finditer(text))
        allocation_definitions[(site, "ZTools placement new")] += len(definitions)
        for match in OPERATOR_CALL_RE.finditer(text):
            if any(definition.start() <= match.start() < definition.end()
                   for definition in definitions):
                continue
            line = text.count("\n", 0, match.start()) + 1
            key = (site, re.sub(r"\s+", "", match.group(0)).replace("operator", "operator "))
            allocation_calls.setdefault(key, []).append(line)
        for match in CTOR_CALL_RE.finditer(text):
            line = text.count("\n", 0, match.start()) + 1
            findings.append(f"explicit constructor call: {site}:{line}: {match.group(1)}")
        for match in FORCE_HELPER_RE.finditer(text):
            line = text.count("\n", 0, match.start()) + 1
            findings.append(
                f"forced-emission helper: {site}:{line}: {match.group(1)}"
            )
        for match in STATIC_INIT_RE.finditer(text):
            line = text.count("\n", 0, match.start()) + 1
            findings.append(
                f"manual static-init hook: {site}:{line}: {match.group(0).strip()}"
            )
        placements.update((site, match.group(1)) for match in PLACEMENT_RE.finditer(text))
        dtor_calls.update((site, match.group(1)) for match in DTOR_CALL_RE.finditer(text))
        low_level[(site, "naked")] += len(re.findall(r"__declspec\s*\(\s*naked\s*\)", text))
        low_level[(site, "asm")] += len(re.findall(r"\b__asm\b", text))
    for key in sorted(set(allocation_calls) | set(allocation_call_allow)):
        lines = allocation_calls.get(key, [])
        if len(lines) == allocation_call_allow[key]:
            continue
        if not lines:
            findings.append(f"compiler allocation call: {key[0]}: {key[1]}: found 0, "
                            f"expected {allocation_call_allow[key]}")
        for line in lines:
            findings.append(f"compiler allocation call: {key[0]}:{line}: {key[1]}")
    findings += _counter_findings("placement construction", placements, placement_allow)
    findings += _counter_findings("allocation definition", allocation_definitions,
                                  allocation_definition_allow)
    findings += _counter_findings("explicit destructor call", dtor_calls, dtor_allow)
    findings += _counter_findings("low-level compiler seam", low_level, low_level_allow)
    return findings


def _report_path() -> Path | None:
    for path in (
        BUILD / "objdiff/compare-new/report.json",
        BUILD / "objdiff/report.json",
    ):
        if path.is_file():
            return path
    return None


def base_only_code() -> list[tuple[str, str]]:
    """Return unique external code definitions absent from objdiff pairing."""
    report = _report_path()
    base = BUILD / "objdiff/base"
    if report is None or not base.is_dir():
        return []
    data = json.loads(report.read_text())
    paired = {
        fn.get("name", "")
        for unit in data.get("units", [])
        for fn in unit.get("functions", [])
    }
    found: set[tuple[str, str]] = set()
    for path in sorted(base.glob("*.obj")):
        try:
            obj = Coff(path)
        except (OSError, ValueError, struct.error):
            continue
        for name, _value, section, storage in obj.symbols:
            if storage != 2 or name in paired or name.startswith("."):
                continue
            if not 1 <= section <= len(obj.section_chars):
                continue
            if obj.section_chars[section - 1] & IMAGE_SCN_CNT_CODE:
                found.add((path.stem, name))
    return sorted(found)


def base_only_suspicious(rows=None) -> list[str]:
    rows = base_only_code() if rows is None else rows
    return [
        f"base-only forced-emission symbol: {unit}: {name}"
        for unit, name in rows
        if re.search(r"(?:Realize[A-Z]|ForceEmit|EmitCompiler|UnusedWindowQuery)", name)
    ]


def gate_findings(files=None) -> list[str]:
    return source_findings(files) + base_only_suspicious()


@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="homm1 verify compiler-artifacts", description=__doc__
    )
    ap.add_argument(
        "--base-only", action="store_true",
        help="list every external code definition absent from objdiff pairing",
    )
    args = ap.parse_args(argv)
    findings = gate_findings()
    for finding in findings:
        print(finding, file=sys.stderr)
    rows = base_only_code() if args.base_only else []
    if args.base_only:
        print(f"base-only external code: {len(rows)}")
        for unit, name in rows:
            print(f"  {unit}\t{name}")
    if findings:
        print(f"compiler-artifacts: FAIL - {len(findings)} finding(s)", file=sys.stderr)
        return 1
    print("compiler-artifacts: OK - no explicit compiler machinery outside "
          "the reviewed raw-storage/low-level seams")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
