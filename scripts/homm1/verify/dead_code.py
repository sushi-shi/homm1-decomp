"""Verify that ``@dead-code`` source markers track retail reachability.

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
from homm1.verify.srcscan import VA_RE, claim_rva, source_files

MARKER_RE = re.compile(r"^\s*(?://|;)\s*@dead-code\b")
PROOF_RE = re.compile(r"\bZero-ref:")
ASM_PROC_RE = re.compile(r"^\s*([A-Za-z_]\w*)\s+PROC\b", re.IGNORECASE)
LOOKAHEAD = 6


def _fixed_asm_claims():
    """``resolved source -> {PROC spelling -> RVA}`` for reviewed MASM."""
    from homm1.graph.fixed_asm import UNITS

    out = {}
    for record in UNITS.values():
        path = (REPO / record.source).resolve()
        out[path] = {claim.name.lstrip("_"): claim.va - 0x00400000
                     for claim in record.claims}
    return out


def source_markers(files=None, asm_claims=None):
    """Return ``(marked_rva -> [(path, line)], rva -> site, problems)``."""
    marked: dict[int, list[tuple[str, int]]] = {}
    rva_sites: dict[int, tuple[str, int]] = {}
    problems: list[str] = []
    asm_claims = _fixed_asm_claims() if asm_claims is None else asm_claims
    for path in files if files is not None else source_files((".cpp", ".asm")):
        rel = str(path.relative_to(REPO)) if path.is_relative_to(REPO) else str(path)
        lines = path.read_text(errors="replace").splitlines()
        proc_rvas = asm_claims.get(path.resolve(), {}) if path.suffix == ".asm" else {}
        for i, line in enumerate(lines):
            m = VA_RE.search(line)
            if m:
                rva_sites[claim_rva(m)] = (rel, i + 1)
            proc = ASM_PROC_RE.match(line)
            if proc and proc.group(1) in proc_rvas:
                rva_sites[proc_rvas[proc.group(1)]] = (rel, i + 1)
            if not MARKER_RE.match(line):
                continue
            claim = None
            stop = min(len(lines), i + LOOKAHEAD + 1)
            for j in range(i + 1, stop):
                m = VA_RE.search(lines[j])
                if m:
                    claim = (claim_rva(m), j)
                    break
                proc = ASM_PROC_RE.match(lines[j])
                if proc and proc.group(1) in proc_rvas:
                    claim = (proc_rvas[proc.group(1)], j)
                    break
            if claim is None:
                problems.append(f"{rel}:{i + 1}: @dead-code has no following "
                                f"RVA/PROC within {LOOKAHEAD} lines")
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
                        "is missing @dead-code")
    return findings


def gate_findings(files=None) -> list[str]:
    marked, rva_sites, problems = source_markers(files)
    rows = [b for b in index().functions if b.channel == "src"]
    explicit = {b.rva for b in rows}
    dead = {b.rva for b in rows if not is_effectively_reached(b.rva, b.size)}
    return problems + compare(marked, rva_sites, explicit, dead)


from homm1.core.usage import logged


@logged
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
