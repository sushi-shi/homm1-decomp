"""Verify that ``// @dead-code`` exactly tracks retail reachability.

Every explicitly reconstructed function with no effective incoming rel32 or
relocated reference must carry the marker and its proof line.  Linker thunks
are transparent: a vtable/callback reference to a thunk keeps the forwarded
body live, while an unreferenced incremental thunk does not.
"""

from __future__ import annotations

import re

from homm1.core.paths import REPO
from homm1.sema.index import index
from homm1.sema.xref import is_effectively_reached
from homm1.verify.srcscan import RVA_RE, source_files

MARKER_RE = re.compile(r"^\s*//\s*@dead-code\b")
PROOF_RE = re.compile(r"\bZero-ref:")
LOOKAHEAD = 6


def source_markers(files=None):
    """Return ``(marked_rva -> [(path, line)], rva -> site, problems)``."""
    marked: dict[int, list[tuple[str, int]]] = {}
    rva_sites: dict[int, tuple[str, int]] = {}
    problems: list[str] = []
    for path in files if files is not None else source_files((".cpp",)):
        rel = str(path.relative_to(REPO)) if path.is_relative_to(REPO) else str(path)
        lines = path.read_text(errors="replace").splitlines()
        for i, line in enumerate(lines):
            m = RVA_RE.search(line)
            if m:
                rva_sites[int(m.group(1), 16)] = (rel, i + 1)
            if not MARKER_RE.match(line):
                continue
            claim = None
            stop = min(len(lines), i + LOOKAHEAD + 1)
            for j in range(i + 1, stop):
                m = RVA_RE.search(lines[j])
                if m:
                    claim = (int(m.group(1), 16), j)
                    break
            if claim is None:
                problems.append(f"{rel}:{i + 1}: @dead-code has no following "
                                f"RVA within {LOOKAHEAD} lines")
                continue
            rva, claim_line = claim
            if not any(PROOF_RE.search(lines[j])
                       for j in range(i + 1, claim_line)):
                problems.append(f"{rel}:{i + 1}: @dead-code lacks its "
                                "Zero-ref: proof line")
            marked.setdefault(rva, []).append((rel, i + 1))
    return marked, rva_sites, problems


def compare(marked, rva_sites, explicit, dead):
    findings: list[str] = []
    for rva, sites in sorted(marked.items()):
        if len(sites) > 1:
            findings.append(f"0x{rva:06x}: duplicate @dead-code markers: "
                            + ", ".join(f"{p}:{ln}" for p, ln in sites))
        if rva not in explicit:
            p, ln = sites[0]
            findings.append(f"{p}:{ln}: @dead-code is not attached to an "
                            "explicit reconstructed function")
        elif rva not in dead:
            p, ln = sites[0]
            findings.append(f"{p}:{ln}: stale @dead-code on reachable "
                            f"function 0x{rva:06x}")
    for rva in sorted(dead - set(marked)):
        p, ln = rva_sites.get(rva, ("<source site unavailable>", 0))
        findings.append(f"{p}:{ln}: zero-reference function 0x{rva:06x} "
                        "is missing // @dead-code")
    return findings


def gate_findings(files=None) -> list[str]:
    marked, rva_sites, problems = source_markers(files)
    rows = [b for b in index().functions if b.channel == "src"]
    explicit = {b.rva for b in rows}
    dead = {b.rva for b in rows if not is_effectively_reached(b.rva, b.size)}
    return problems + compare(marked, rva_sites, explicit, dead)


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 verify dead-code",
                                 description=__doc__.split("\n\n")[0])
    ap.parse_args(argv)
    findings = gate_findings()
    for finding in findings:
        print(finding)
    if findings:
        print(f"dead-code: {len(findings)} reachability/marker finding(s)")
        return 1
    print("dead-code: OK - every explicit zero-reference function is marked "
          "and every marker is proven")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
