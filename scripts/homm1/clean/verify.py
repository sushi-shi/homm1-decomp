"""Build the clean tree with the pinned VC4 toolchain and compare it with the
matching build (kf1's "verify against native linker outputs").

Every unit compiles through the same `homm1.tool.cl` -> fixedroot path, with
the same profile, retail file name and include view as the matching build;
MASM units assemble as the retail link's OMF and as comparison COFF. The
objects then link through `homm1.graph.link` exactly like `homm1 link`
(retail object order, BASE library, no /FORCE).

Two trees are built. The clean tree must compile and link. Its code is not
expected to equal the matching build: under retail's /Gi, VC4's symbol
handles follow the path strings of every opened file and the source line
numbers, so dropping the scaffolding includes, `#line` pins and comment lines
moves register and operand choices. The control tree isolates the transforms
from that compiler state: the same macro expansions and comment removal, with
every line, `#line` pin, scaffolding header and include kept. The control must reproduce every matching object section and the
candidate HEROES.EXE byte for byte, LINK's TimeDateStamps aside.

Finally the tree's own `build.py` runs through its flake, which fetches the
hash-pinned toolchain release, and must produce HEROES.EXE.

Objects are compared section by section outside `.debug$*` (raw bytes,
relocation offsets/types/target names, flags). Nothing is patched or banked;
this proves the generated source compiles to the matching program, not a
retail match.
"""

from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import shutil
import struct
import sys

from homm1 import graph
from homm1.core.paths import REPO

#: Concurrent compiles, capped like the build graph's wine pool.
WINE_JOBS = graph.WINE_POOL_DEPTH


# --------------------------------------------------------------------------
# COFF section comparison
# --------------------------------------------------------------------------

def _sections(path: Path) -> tuple[list[dict], list[tuple[str, int]]]:
    data = path.read_bytes()
    machine, count, _stamp, symptr, nsym, optional, _flags = struct.unpack_from("<HHIIIHH", data, 0)
    if machine != 0x14C:
        raise ValueError(f"{path}: not an i386 COFF object")
    strtab = symptr + nsym * 18

    def name_at(raw: bytes) -> str:
        if raw[:4] == b"\0\0\0\0":
            offset = struct.unpack_from("<I", raw, 4)[0]
            end = data.index(b"\0", strtab + offset)
            return data[strtab + offset:end].decode("latin-1")
        return raw.rstrip(b"\0").decode("latin-1")

    symbols: list[tuple[str, int]] = []
    index = 0
    while index < nsym:
        entry = data[symptr + index * 18:symptr + index * 18 + 18]
        section = struct.unpack_from("<h", entry, 12)[0]
        symbols.append((name_at(entry[:8]), section))
        for _ in range(entry[17]):
            symbols.append(("", 0))
        index += 1 + entry[17]

    sections = []
    for number in range(count):
        base = 20 + optional + number * 40
        raw_name = data[base:base + 8]
        name = raw_name.rstrip(b"\0").decode("latin-1")
        if name.startswith("/"):
            offset = int(name[1:])
            name = data[strtab + offset:data.index(b"\0", strtab + offset)].decode("latin-1")
        size, pointer, relocs = struct.unpack_from("<III", data, base + 16)
        nrelocs = struct.unpack_from("<H", data, base + 32)[0]
        flags = struct.unpack_from("<I", data, base + 36)[0]
        relocations = []
        for r in range(nrelocs):
            offset, symbol, kind = struct.unpack_from("<IIH", data, relocs + r * 10)
            relocations.append((offset, kind, symbols[symbol][0]))
        sections.append({
            "name": name, "flags": flags,
            "data": data[pointer:pointer + size] if pointer else size,
            "relocations": relocations,
            "defines": [s for s, sec in symbols if sec == number + 1 and s and not s.startswith(".")],
        })
    return sections, symbols


def compare_objects(clean: Path, matching: Path) -> list[str]:
    """Differences between two objects' non-debug sections; [] when identical."""
    ours, _ = _sections(clean)
    theirs, _ = _sections(matching)
    ours = [s for s in ours if not s["name"].startswith(".debug")]
    theirs = [s for s in theirs if not s["name"].startswith(".debug")]
    if len(ours) != len(theirs):
        return [f"{len(ours)} sections against {len(theirs)}"]
    differences = []
    for a, b in zip(ours, theirs):
        if (a["name"], a["flags"], a["data"], a["relocations"]) != \
                (b["name"], b["flags"], b["data"], b["relocations"]):
            owner = ", ".join(b["defines"][:3]) or a["name"]
            differences.append(owner)
    return differences


# --------------------------------------------------------------------------
# PE comparison
# --------------------------------------------------------------------------

def _stamp_offsets(data: bytes) -> set[int]:
    """File offsets of the PE header, export-directory and resource-directory
    TimeDateStamps (LINK and CVTRES stamp each with the build time)."""
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    stamps = set(range(pe + 8, pe + 12))
    optional = pe + 24
    count = struct.unpack_from("<H", data, pe + 6)[0]
    first = optional + struct.unpack_from("<H", data, pe + 20)[0]
    export_rva = struct.unpack_from("<I", data, optional + 96)[0]       # DataDirectory[0]
    resource_rva = struct.unpack_from("<I", data, optional + 112)[0]    # DataDirectory[2]
    for i in range(count):
        base = first + i * 40
        vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", data, base + 8)
        if export_rva and vaddr <= export_rva < vaddr + max(vsize, rawsize):
            at = rawptr + export_rva - vaddr + 4
            stamps |= set(range(at, at + 4))
        if resource_rva and vaddr <= resource_rva < vaddr + max(vsize, rawsize):
            root = rawptr + resource_rva - vaddr
            pending = [0]
            while pending:
                directory = root + pending.pop()
                stamps |= set(range(directory + 4, directory + 8))
                named, ids = struct.unpack_from("<HH", data, directory + 12)
                for k in range(named + ids):
                    target = struct.unpack_from("<I", data, directory + 20 + 8 * k)[0]
                    if target & 0x80000000:
                        pending.append(target & 0x7FFFFFFF)
    return stamps


def _section_of(data: bytes, offset: int) -> str:
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    first = pe + 24 + struct.unpack_from("<H", data, pe + 20)[0]
    for i in range(count):
        base = first + i * 40
        rawsize, rawptr = struct.unpack_from("<II", data, base + 16)
        if rawptr <= offset < rawptr + rawsize:
            return data[base:base + 8].rstrip(b"\0").decode()
    return "headers"


def compare_images(clean: Path, matching: Path) -> tuple[bool, dict[str, int]]:
    """(byte-identical apart from timestamps, {section: differing bytes})."""
    a, b = clean.read_bytes(), matching.read_bytes()
    if len(a) != len(b):
        return False, {"size": abs(len(a) - len(b))}
    ignored = _stamp_offsets(a) | _stamp_offsets(b)
    counts: dict[str, int] = {}
    for offset, (x, y) in enumerate(zip(a, b)):
        if x != y and offset not in ignored:
            section = _section_of(a, offset)
            counts[section] = counts.get(section, 0) + 1
    return not counts, counts


# --------------------------------------------------------------------------
# Build
# --------------------------------------------------------------------------

def _build_unit(tree: Path, work: Path, record: dict, flags: list[str]) -> tuple[str, Path, Path]:
    """Compile one unit; returns (unit, comparison object, link object)."""
    from homm1.graph.cc import retail_name, stabilise
    from homm1.graph.fixed_asm import unit as fixed_asm_unit
    from homm1.tool import fixedroot, ml

    unit, src = record["unit"], tree / record["source"]
    obj = work / "obj" / f"{unit}.obj"
    if src.suffix.lower() == ".asm":
        ml.assemble(src, obj, coff=True)
        link_obj = obj
        if fixed_asm_unit(unit, record["source"]) is not None:
            link_obj = work / "omf" / f"{unit}.obj"
            ml.assemble(src, link_obj, coff=False)
        return unit, obj, link_obj
    if "/Gi" not in flags:
        raise ValueError(f"{unit}: verification expects the fixedroot (/Gi) profiles")
    fixedroot.compile(src, obj, flags, retail_name=retail_name(unit, record["source"]),
                      unit=unit, repo=tree)
    obj.write_bytes(stabilise(obj.read_bytes()))
    return unit, obj, obj


def _build_tree(tree: Path, work: Path, label: str) -> tuple[list, dict[str, list[str]]]:
    """Compile and compare every unit of `tree`; returns (built, differences)."""
    from homm1.manifest import flag_profiles, units

    profiles = flag_profiles()
    with ThreadPoolExecutor(WINE_JOBS) as pool:
        built = list(pool.map(lambda r: _build_unit(tree, work, r, profiles[r["flags"]]),
                              units()))
    different = {}
    for unit, obj, _link_obj in built:
        differences = compare_objects(obj, REPO / graph.BASE_DIR / f"{unit}.obj")
        if differences:
            different[unit] = differences
    print(f"[clean] verify: {label}: {len(built) - len(different)}/{len(built)} unit objects "
          "identical to the matching build in every non-debug section")
    return built, different


def _link(work: Path, built: list, res: Path | None, label: str) -> tuple[bool, dict]:
    from homm1.graph.link import candidate
    exe = work / "HEROES.EXE"
    result = candidate(exe, work / "obj", explicit=[str(o) for _u, _o, o in built], res=res)
    same, counts = compare_images(exe, REPO / graph.CANDIDATE_EXE)
    print(f"[clean] verify: {label}: linked {exe.relative_to(REPO) if exe.is_relative_to(REPO) else exe} "
          f"({len(result['unresolved'])} unresolved, no /FORCE); "
          + ("byte-identical to " + graph.CANDIDATE_EXE + " apart from build timestamps"
             if same else "differs from " + graph.CANDIDATE_EXE + " in "
             + ", ".join(f"{name} {n} B" for name, n in sorted(counts.items()))))
    return same, counts


def verify(tree: Path, inputs: dict[str, bytes]) -> int:
    """Build the clean tree and its line-preserving control; 0 when the clean
    tree builds and links and the control reproduces the matching build."""
    from homm1.clean.run import generate
    from homm1.graph.verbs import link_main
    from homm1.tool import ToolError

    tree = Path(tree).resolve()
    print("[clean] verify: bringing the matching candidate up to date (homm1 link)")
    if link_main([]):
        print("[clean] verify: the matching build failed; nothing to compare against",
              file=sys.stderr)
        return 1

    work = tree.parent / f"{tree.name}-verify"
    if work.exists():
        shutil.rmtree(work)
    control = work / "control-tree"
    files, _ = generate(inputs, control=True)
    for name, data in files.items():
        path = control / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    res = None
    status = 0
    report = []
    try:
        if (REPO / graph.RESOURCE_RES).is_file():
            from homm1.core.paths import retail_exe
            from homm1.tool import rc
            res = work / "heroes.res"
            rc.compile(tree / graph.RESOURCE_SCRIPT, res, retail=retail_exe())
            print("[clean] verify: Heroes.rc compiles to the retail resource payloads")
        print(f"[clean] verify: compiling {tree} and its control with the pinned VC4 "
              f"toolchain (fixedroot view, {WINE_JOBS} jobs)")
        for label, root, out in (("control", control, work / "control"),
                                 ("clean", tree, work / "clean")):
            built, different = _build_tree(root, out, label)
            same, _counts = _link(out, built, res, label)
            report += [f"{label}\t{unit}\t{section}"
                       for unit, sections in sorted(different.items()) for section in sections]
            if label == "control" and (different or not same):
                print("[clean] verify: FAIL: the control must reproduce the matching build; "
                      "a transform changed code", file=sys.stderr)
                status = 1
    except (ToolError, ValueError, OSError) as error:
        print(f"[clean] verify: FAIL: {error}", file=sys.stderr)
        return 1
    status = standalone(tree) or status
    (work / "differences.tsv").write_text("tree\tunit\tsection\n" + "".join(
        line + "\n" for line in report))
    print(f"[clean] verify: per-section differences: {work / 'differences.tsv'}")
    return status


def standalone(tree: Path) -> int:
    """Run the tree's own build (its flake's toolchain, Wine and llvm-rc)."""
    import os
    import subprocess
    from homm1.core.paths import retail_exe

    env = {key: value for key, value in os.environ.items()
           if key not in ("WINEPREFIX", "PYTHONPATH", "HOMM1_TOOLCHAIN")}
    command = ["nix", "develop", f"path:{tree}", "-c", "python3", "build.py"]
    if retail_exe().is_file():
        command += ["--icon-from", str(retail_exe())]
    print(f"[clean] verify: standalone: {' '.join(command[:3])} -c python3 build.py")
    result = subprocess.run(command, cwd=tree, env=env, capture_output=True, text=True)
    exe = tree / "build" / "HEROES.EXE"
    if result.returncode or not exe.is_file():
        print("\n".join((result.stdout + result.stderr).strip().splitlines()[-20:]),
              file=sys.stderr)
        print("[clean] verify: FAIL: the tree's own build failed", file=sys.stderr)
        return 1
    print(f"[clean] verify: standalone: built {exe.relative_to(tree)} "
          f"({exe.stat().st_size:,} B) with the flake's pinned toolchain")
    return 0
