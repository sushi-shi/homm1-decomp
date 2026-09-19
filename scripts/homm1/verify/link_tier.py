"""homm1.verify.link_tier - the candidate-EXE audits (link tier, opt-in).

Needs `homm1 link`'s outputs (HOMM1.candidate.EXE + .map). Three ported
checks folded into one tier module:

  LINK DEFECTS   the link must be REAL: 0 unresolved externals (the linker's
                 own unresolved.txt), and a pre-link closure over the base
                 objs - every undefined external resolves from a base obj
                 definition/COMMON/weak-alias, a toolchain .LIB, or an
                 import. A symbol none of those supply is a guaranteed
                 `unresolved external symbol` (the declared-only family).
  SECTION CENSUS the candidate's section table vs retail's - per-section raw
                 sizes and deltas (the link-order study's byte budget); a
                 section retail has that the candidate lacks is FATAL.
  IMAGE DIFF     for every claimed function the compare report scores at
                 100%, the candidate's bytes at its LINK-ASSIGNED rva (the
                 .map) must equal retail's at the retail rva under reloc
                 masking (both sides' 4-byte fixup fields blanked via each
                 image's own .reloc table). The linked-image check is what
                 objdiff's per-obj scoring cannot see: final placement.

    homm1 verify link-tier             # the three checks, exit 1 on any
    homm1 verify link-tier --census    # the section table only
"""

from __future__ import annotations

import re
import struct
import sys

from homm1.core.paths import BUILD

CAND = BUILD / "exe/HOMM1.candidate.EXE"
CMAP = BUILD / "exe/HOMM1.candidate.map"
UNRESOLVED = BUILD / "exe/HOMM1.candidate.unresolved.txt"

_MAP_ROW = re.compile(r"^ (\d{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})")


def parse_map() -> dict[str, int]:
    """{symbol: candidate rva} from the .map's Publics-by-Value table."""
    out: dict[str, int] = {}
    if not CMAP.is_file():
        return out
    base = None
    in_publics = False
    for ln in CMAP.read_text(errors="replace").splitlines():
        if "Publics by Value" in ln:
            in_publics = True
            continue
        if not in_publics:
            continue
        m = _MAP_ROW.match(ln)
        if not m:
            continue
        va = int(m.group(4), 16)
        if base is None:
            base = va - (int(m.group(2), 16) + 0x1000)  # 0001 section at 0x1000
        out.setdefault(m.group(3), va - 0x400000)
    return out


def _candidate_pe():
    from homm1.core.pe import Pe
    return Pe(CAND)


def link_defect_findings() -> list[str]:
    out = []
    if not CAND.is_file():
        return [f"link-tier: no candidate EXE ({CAND}) - run `homm1 link` "
                f"first (the link tier is opt-in)"]
    if UNRESOLVED.is_file():
        text = UNRESOLVED.read_text(errors="replace").strip()
        if text:
            for ln in text.splitlines()[:20]:
                out.append(f"link-defects: unresolved external: {ln}")
    # pre-link closure over the base objs
    from homm1.verify.undefined_closure import (_sym_sets, lib_symbols,
                                                 live_base_objs)
    bdef, bund = _sym_sets(live_base_objs())
    libs = lib_symbols()
    for s in sorted(bund - bdef):
        if s in libs or s.lstrip("_") in libs:
            continue
        if s.startswith("__imp_"):
            base = s[len("__imp_"):]
            if base in libs or base.lstrip("_") in libs:
                continue
        out.append(f"link-defects: {s} resolves from no base obj and from no "
                   f"archive under $MSVC_DIR/$DXSDK_DIR - either a declared-"
                   f"only symbol (fix the declaration/definition) or a "
                   f"library this closure does not scan")
    return out


def census() -> list[tuple[str, int, int]]:
    """[(section, retail_rsize, candidate_rsize)] (0 for a missing side)."""
    from homm1.core.pe import image
    retail = image()
    cand = _candidate_pe()
    names = [s["name"] for s in retail.sections]
    names += [s["name"] for s in cand.sections if s["name"] not in names]
    rows = []
    for n in names:
        r = next((s["rsize"] for s in retail.sections if s["name"] == n), 0)
        c = next((s["rsize"] for s in cand.sections if s["name"] == n), 0)
        rows.append((n, r, c))
    return rows


def census_findings() -> list[str]:
    if not CAND.is_file():
        return []
    return [f"link-sections: retail section {n} ({r:#x} B) is ABSENT from "
            f"the candidate" for n, r, c in census() if r and not c
            and n != ".reloc"]


def _reloc_sites(pe) -> set[int]:
    sites = set()
    try:
        sec = pe.section(".reloc")
    except KeyError:
        return sites
    blob = pe.data[sec["rptr"]:sec["rptr"] + sec["rsize"]]
    p, end = 0, min(sec["rsize"], sec["vsize"])
    while p + 8 <= end:
        page, blk = struct.unpack_from("<II", blob, p)
        if blk < 8:
            break
        for i in range(8, min(blk, end - p), 2):
            ent = struct.unpack_from("<H", blob, p + i)[0]
            if ent >> 12 == 3:
                sites.add(page + (ent & 0xFFF))
        p += blk
    return sites


def _rel32_offsets(blob: bytes, vma: int) -> list[int]:
    """Window offsets of decoded E8/E9 (call/jmp rel32) instructions."""
    from homm1.tool import objdump
    out = []
    for line in objdump.disassemble(blob, vma=vma).splitlines():
        if ":\t" not in line:
            continue
        addr, rest = line.split(":\t", 1)
        parts = rest.split("\t")
        if len(parts) < 2:
            continue
        raw = parts[0].strip().split()
        if raw and raw[0] in ("e8", "e9"):
            try:
                out.append(int(addr.strip(), 16) - vma)
            except ValueError:
                continue
    return out


def _byte_exact_symbols(pct: dict[tuple[str, str], float]) -> set[str]:
    """Symbols whose object score is literally 100%, not merely displayed
    as exact by the 99.995% navigation threshold.

    The linked-image audit isolates link-assigned placement and referents. A
    below-100 object body already contains an object-local byte difference, so
    admitting it here misreports that known compiler residue as a link defect.
    """
    return {sym for (_unit, sym), score in pct.items() if score == 100.0}


def image_diff_findings(limit: int = 25) -> list[str]:
    """Exact-scored functions whose LINKED candidate bytes diverge from
    retail under reloc masking."""
    if not CAND.is_file():
        return []                       # link_defect_findings already said so
    if not CMAP.is_file():
        # Without the map there is no rva for any candidate body, so the
        # image diff cannot run at all - and main()'s success line would
        # otherwise claim "every exact body byte-identical in the linked
        # image" on the strength of a check that never executed.
        return [f"image-diff: no candidate map ({CMAP}) - the linked-image "
                f"diff could not run; re-run `homm1 link` (it writes the "
                f".map beside the EXE)"]
    # a candidate older than the newest base obj was linked from OTHER
    # bytes: a divergence would be stale-image noise, not a link fact
    newest = max((p.stat().st_mtime
                  for p in (BUILD / "objdiff/base").rglob("*.obj")),
                 default=0.0)
    if newest > CAND.stat().st_mtime:
        return ["image-diff: candidate EXE is STALE (a base obj is newer) - "
                "re-run `homm1 link` before reading the linked-image diff"]
    from homm1.core.pe import image
    from homm1.model import resolve
    from homm1.walls.inventory import report_scores
    retail = image()
    cand = _candidate_pe()
    cand_rvas = parse_map()
    rsites = _reloc_sites(retail)
    csites = _reloc_sites(cand)
    _p, pct = report_scores()
    exact = _byte_exact_symbols(pct)
    out = []
    checked = 0
    for b in resolve().functions:
        if not b.name or b.name not in exact:
            continue
        crva = cand_rvas.get(b.name)
        if crva is None:
            continue
        rb = retail.read(b.rva, b.size)
        cb = cand.read(crva, b.size)
        if rb is None or cb is None:
            continue
        checked += 1
        rm, cm = bytearray(rb), bytearray(cb)
        for s in range(b.rva, b.rva + b.size):
            if s in rsites:
                for k in range(4):
                    if s - b.rva + k < len(rm):
                        rm[s - b.rva + k] = 0
        for s in range(crva, crva + b.size):
            if s in csites:
                for k in range(4):
                    if s - crva + k < len(cm):
                        cm[s - crva + k] = 0
        # REL32 call/jmp displacements re-resolve at link. Masked at DECODED
        # instruction boundaries (a byte scan desyncs on immediates that
        # contain 0xE8 - `mov [esi+0x1c],0x3e8` swallowed the next call's
        # opcode); the retail decode's offsets apply to both sides - if the
        # bodies structurally diverge the mask misapplies and the diff
        # fires, which is the correct verdict.
        for off in _rel32_offsets(rb, b.rva):
            if off + 5 <= len(rm):
                rm[off + 1:off + 5] = b"\0\0\0\0"
                cm[off + 1:off + 5] = b"\0\0\0\0"
        if bytes(rm) != bytes(cm):
            first = next((i for i, (x, y) in enumerate(zip(rm, cm))
                          if x != y), 0)
            out.append(f"image-diff: {b.name[:60]} retail 0x{b.rva:06x} != "
                       f"candidate 0x{crva:06x} at +{first:#x} (100%-scored "
                       f"body diverges in the LINKED image)")
            if len(out) >= limit:
                out.append("image-diff: ... (limit reached)")
                break
    if not checked and exact:
        out.append("image-diff: 0 exact functions located in the candidate "
                   "map - map/report join broken (never vacuous)")
    return out


def gate_findings() -> list[str]:
    return link_defect_findings() + census_findings() + image_diff_findings()


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 verify link-tier",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--census", action="store_true",
                    help="print the retail-vs-candidate section table and stop")
    a = ap.parse_args(argv)
    if a.census:
        print(f"{'section':<10} {'retail':>10} {'candidate':>10} {'delta':>9}")
        for n, r, c in census():
            print(f"{n:<10} {r:>10,} {c:>10,} {c - r:>+9,}")
        return 0
    bad = gate_findings()
    for b in bad:
        print("  " + b, file=sys.stderr)
    if bad:
        print(f"link-tier: FATAL - {len(bad)} finding(s)", file=sys.stderr)
        return 1
    print("link-tier: OK - link closes, sections present, every exact body "
          "byte-identical in the linked image (reloc-masked)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
