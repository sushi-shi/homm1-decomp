"""Build the generated trees with the pinned VC6 toolchain and compare them
with the matching build.

Each unit compiles with the same pinned CL, the same profile and the same
include view as the matching build, from a copy of the tree whose catalog
references are resolved to Windows-1251 literals (Russian, the retail
program's language); MASM units assemble as the retail link's OMF and as
comparison COFF. The objects then link through `homm1.graph.link` exactly like
`homm1 link` (retail object order, BASE library, no /FORCE).

Four checks, all of which must pass:

* The control tree applies the same macro expansions and comment removal as
  the source tree but keeps every line, the `#line` pins and the scaffolding
  headers. It must reproduce every non-debug object section (bytes,
  relocations, symbol names) and the candidate HEROES.EXE byte for byte,
  LINK's TimeDateStamps aside: the transforms change no code.
* The source tree must compile and link without /FORCE. Without the `#line`
  pins its assertions carry their own line numbers and file names, and VC6's
  local label counters ($L, $T, $SG) shift with the removed scaffolding
  headers, so its objects are compared after numbering those compiler-local
  names by first appearance; every remaining difference is listed.
* For the classic variant, every classic file must equal the source tree's
  Russian compiler input token for token once its UTF-8 literals are read as
  the Windows-1251 bytes they show (classic_equivalence).
* The source tree's own build.py, run through its flake (which fetches the
  hash-pinned toolchain release), must produce HEROES.EXE in both languages.

Nothing is patched or banked; this proves the generated source compiles to
the matching program, not a retail match.
"""

from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import shutil
import re
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


#: VC6's compiler-local names: branch labels, EH state tables, string
#: literals, dynamic-initializer thunks and `goto` labels ($name$N), numbered
#: by a per-compilation counter that every macro the compilation defines
#: advances (the control and the matching build define the same ones).
_LOCAL_NAME = re.compile(r"^_?\$(?:L|T|SG|E|S)\d+$|^\$\w+\$\d+$")


def _canonical(sections: list[dict], symbols: list[tuple[str, int]]) -> list[dict]:
    """`sections` with compiler-local names renumbered by first appearance in
    the symbol table, so a shifted counter compares equal."""
    names: dict[str, str] = {}
    for name, _section in symbols:
        if _LOCAL_NAME.match(name) and name not in names:
            names[name] = f"$local{len(names)}"
    for section in sections:
        section["relocations"] = [(offset, kind, names.get(name, name))
                                  for offset, kind, name in section["relocations"]]
        section["defines"] = [names.get(name, name) for name in section["defines"]]
    return sections


def compare_objects(clean: Path, matching: Path, *, local_names: bool = True) -> list[str]:
    """Differences between two objects' non-debug sections; [] when identical.

    `local_names=False` numbers compiler-local labels by first appearance
    before comparing (see _LOCAL_NAME)."""
    ours, ours_symbols = _sections(clean)
    theirs, their_symbols = _sections(matching)
    if not local_names:
        ours, theirs = _canonical(ours, ours_symbols), _canonical(theirs, their_symbols)
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

def _debug_stamps(data: bytes, offset_of) -> set[int]:
    """The debug directory's TimeDateStamps and each CodeView NB10 record's
    signature (LINK derives both from the build time)."""
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    rva, size = struct.unpack_from("<II", data, pe + 24 + 96 + 6 * 8)
    stamps: set[int] = set()
    if not rva:
        return stamps
    directory = offset_of(rva)
    for entry in range(directory, directory + size, 28):
        stamps |= set(range(entry + 4, entry + 8))
        raw = struct.unpack_from("<I", data, entry + 24)[0]
        if raw and data[raw:raw + 4] == b"NB10":
            stamps |= set(range(raw + 8, raw + 12))
    return stamps


def _stamp_offsets(data: bytes) -> set[int]:
    """File offsets of the PE header, export-directory, resource-directory and
    debug TimeDateStamps (LINK and CVTRES stamp each with the build time)."""
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

    def offset_of(rva: int) -> int:
        for i in range(count):
            base = first + i * 40
            vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", data, base + 8)
            if vaddr <= rva < vaddr + max(vsize, rawsize):
                return rawptr + rva - vaddr
        raise ValueError(f"RVA 0x{rva:x} is outside every section")
    return stamps | _debug_stamps(data, offset_of)


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

def localize(tree: Path, out: Path, locale: str = "ru") -> Path:
    """Copy src/, include/ and vendor/ of `tree` to `out` with every catalog
    reference resolved to `locale`'s Windows-1251 literals, as the tree's own
    build.py does; returns `out`."""
    from homm1.graph.catalog import Catalog
    catalog = Catalog.load(tree)
    for directory in ("src", "include", "vendor"):
        for path in sorted((tree / directory).rglob("*")):
            if not path.is_file():
                continue
            target = out / path.relative_to(tree)
            target.parent.mkdir(parents=True, exist_ok=True)
            if path.suffix in (".cpp", ".h", ".c", ".hpp", ".inc"):
                target.write_text(catalog.render(path.read_text(), locale=locale, expanded=True))
            else:
                shutil.copyfile(path, target)
    return out


def _include_flags(localized: Path) -> list[str]:
    """The matching build's /I view (include/, vendor trees, pinned SDKs)."""
    from homm1.core.paths import vendor_include_dirs
    from homm1.tool.wine import winepath
    dirs = [localized / "include", *(d for _n, d in vendor_include_dirs(localized / "vendor"))]
    return [f"/I{winepath(d)}" for d in dirs]


def _build_unit(localized: Path, work: Path, record: dict, flags: list[str],
                includes: list[str], *, matching_view: bool = False
                ) -> tuple[str, Path, Path]:
    """Compile one unit; returns (unit, comparison object, link object).

    `matching_view` compiles `localized` (then the tree itself) the way the
    matching build does: catalog references become length-padded macro names
    defined by a forced-include header (homm1.graph.localization.prepare), so
    every line and column stays where the authored source has it."""
    import tempfile
    from homm1.graph.cc import stabilise
    from homm1.graph.fixed_asm import unit as fixed_asm_unit
    from homm1.tool import ToolError, ml
    from homm1.tool.wine import era_tool, run, winepath

    unit, src = record["unit"], localized / record["source"]
    obj = work / "obj" / f"{unit}.obj"
    obj.parent.mkdir(parents=True, exist_ok=True)
    if src.suffix.lower() == ".asm":
        ml.assemble(src, obj, coff=True)
        link_obj = obj
        if fixed_asm_unit(unit, record["source"]) is not None:
            link_obj = work / "omf" / f"{unit}.obj"
            link_obj.parent.mkdir(parents=True, exist_ok=True)
            ml.assemble(src, link_obj, coff=False)
        return unit, obj, link_obj
    if "/Gi" in flags:
        raise ValueError(f"{unit}: /Gi profiles need the fixed-root view; this "
                         "verifier builds the VC6 profiles")
    obj.unlink(missing_ok=True)
    if matching_view:
        from homm1.graph.localization import prepare
        compiled, header, _overlay, _deps = prepare(localized, src)
        if header is not None:
            flags = [*flags, f"/FI{winepath(header)}", f"/I{winepath(src.parent)}"]
            if header.name == "messages.h":
                flags = [f"/I{winepath(header.parent / 'include')}", *flags]
        src = compiled
    with tempfile.TemporaryDirectory(dir=work) as scratch:      # per-unit vc60.idb
        argv = ["wine", str(era_tool("cl.exe")), *flags, *includes,
                f"/Fo{winepath(obj)}", winepath(src)]
        output, rc = run(argv, cwd=Path(scratch), success=obj)
    if not obj.exists():
        tail = "\n".join(output.strip().splitlines()[-12:])
        raise ToolError(f"{unit}: cl produced no object (rc={rc}):\n{tail}")
    obj.write_bytes(stabilise(obj.read_bytes()))
    return unit, obj, obj


def _build_tree(tree: Path, work: Path, label: str, *, exact: bool
                ) -> tuple[list, dict[str, list[str]]]:
    """Compile every unit of `tree` and compare each object with the matching
    build; `exact` keeps compiler-local names in the comparison."""
    from homm1.manifest import flag_profiles, units

    # The control compiles as the matching build does; the source tree as its
    # own build.py does (resolved literals in a copy).
    localized = tree if exact else localize(tree, work / "localized")
    includes = _include_flags(localized)
    profiles = flag_profiles()
    with ThreadPoolExecutor(WINE_JOBS) as pool:
        built = list(pool.map(
            lambda r: _build_unit(localized, work, r, profiles[r["flags"]], includes,
                                  matching_view=exact),
            units()))
    different = {}
    for unit, obj, _link_obj in built:
        differences = compare_objects(obj, REPO / graph.BASE_DIR / f"{unit}.obj",
                                      local_names=exact)
        if differences:
            different[unit] = differences
    what = ("in every non-debug section" if exact else
            "in every non-debug section, compiler-local label numbers aside")
    print(f"[clean] verify: {label}: {len(built) - len(different)}/{len(built)} unit objects "
          f"identical to the matching build {what}")
    return built, different


def _link(work: Path, built: list, res: Path | None, label: str) -> tuple[bool, dict]:
    from homm1.graph.link import candidate
    exe = work / "HEROES.EXE"
    result = candidate(exe, work / "obj", explicit=[str(o) for _u, _o, o in built], res=res)
    if result["unresolved"]:
        raise ValueError(f"{label}: {len(result['unresolved'])} unresolved external(s): "
                         + ", ".join(sorted(result["unresolved"])[:6]))
    same, counts = compare_images(exe, REPO / graph.CANDIDATE_EXE)
    shown = exe.relative_to(REPO) if exe.is_relative_to(REPO) else exe
    print(f"[clean] verify: {label}: linked {shown} (no unresolved externals, no /FORCE); "
          + ("byte-identical to the matching candidate apart from build timestamps"
             if same else "differs from the matching candidate in "
             + ", ".join(f"{name} {n} B" for name, n in sorted(counts.items()))))
    return same, counts


#: Spellings whose value moves when `#line` pins and blank lines go.
_POSITIONAL = re.compile(r"\b(?:H1_ASSERT|__FILE__|__LINE__)\b")


def unexplained_differences(tree: Path, different: dict[str, list[str]]) -> list[str]:
    """Differing units whose source names no assertion, file or line."""
    from homm1.manifest import units
    sources = {u["unit"]: u["source"] for u in units()}
    return sorted(unit for unit in different
                  if not _POSITIONAL.search((tree / sources[unit]).read_text(encoding="utf-8")))


def _write(tree: Path, files: dict[str, bytes]) -> Path:
    if tree.exists():
        shutil.rmtree(tree)
    for name, data in files.items():
        path = tree / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    return tree


def verify(tree: Path, inputs: dict[str, bytes], variant: str = "source") -> int:
    """Build the control and source trees and check the variant's tree; 0 when
    every check in the module docstring passes."""
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
    control = _write(work / "control-tree", generate(inputs, control=True)[0])
    source_tree = tree
    if variant != "source":
        source_tree = _write(work / "source-tree", generate(inputs, variant="source")[0])

    res = None
    status = 0
    report = []
    try:
        if (REPO / graph.RESOURCE_RES).is_file():
            from homm1.core.paths import retail_exe
            from homm1.tool import rc
            res = work / "heroes.res"
            rc.compile(source_tree / graph.RESOURCE_SCRIPT, res, retail=retail_exe())
            print("[clean] verify: Heroes.rc compiles to the retail resource payloads")
        print(f"[clean] verify: compiling the control and source trees with the pinned "
              f"VC6 toolchain ({WINE_JOBS} jobs)")
        for label, root, out, exact in (("control", control, work / "control", True),
                                        ("source", source_tree, work / "source", False)):
            built, different = _build_tree(root, out, label, exact=exact)
            same, _counts = _link(out, built, res, label)
            report += [f"{label}\t{unit}\t{section}"
                       for unit, sections in sorted(different.items()) for section in sections]
            if label == "control" and (different or not same):
                print("[clean] verify: FAIL: the control must reproduce the matching build; "
                      "a transform changed code", file=sys.stderr)
                status = 1
            if label == "source":
                unexplained = unexplained_differences(root, different)
                if unexplained:
                    print("[clean] verify: FAIL: source objects differ without an assertion "
                          "line or file name to explain it: " + ", ".join(unexplained),
                          file=sys.stderr)
                    status = 1
                elif different:
                    print(f"[clean] verify: source: the {len(different)} differing units are "
                          "those whose assertions now carry their own line numbers and file "
                          "names: " + ", ".join(sorted(different)))
    except (ToolError, ValueError, OSError) as error:
        print(f"[clean] verify: FAIL: {error}", file=sys.stderr)
        return 1
    (work / "differences.tsv").write_text("tree\tunit\tsection\n" + "".join(
        line + "\n" for line in report))
    print(f"[clean] verify: per-section differences: {work / 'differences.tsv'}")
    if variant == "classic":
        problems = classic_equivalence(tree, work / "source" / "localized", source_tree)
        for problem in problems[:20]:
            print(f"[clean] verify: classic: {problem}", file=sys.stderr)
        if problems:
            print(f"[clean] verify: FAIL: {len(problems)} classic difference(s)", file=sys.stderr)
            status = 1
        else:
            print("[clean] verify: classic: every file equals the source tree's Russian "
                  "compiler input, its UTF-8 literals read as Windows-1251")
    else:
        status = standalone(tree) or status
    return status


# --------------------------------------------------------------------------
# Classic equivalence
# --------------------------------------------------------------------------

_SIMPLE_ESCAPES = {"n": 10, "t": 9, "r": 13, "a": 7, "b": 8, "f": 12, "v": 11,
                   "\\": 92, "'": 39, '"': 34, "?": 63}


def literal_bytes(spelling: str, encoding: str = "cp1251") -> bytes:
    """The bytes a C string or character literal stores (no terminator);
    unescaped characters are encoded with `encoding`."""
    body = spelling[1:-1]
    out = bytearray()
    i = 0
    while i < len(body):
        ch = body[i]
        if ch != "\\":
            out += ch.encode(encoding)
            i += 1
            continue
        nxt = body[i + 1]
        if nxt in _SIMPLE_ESCAPES:
            out.append(_SIMPLE_ESCAPES[nxt])
            i += 2
        elif nxt in "01234567":
            j = i + 1
            while j < len(body) and j < i + 4 and body[j] in "01234567":
                j += 1
            out.append(int(body[i + 1:j], 8) & 0xFF)
            i = j
        elif nxt == "x":
            j = i + 2
            while j < len(body) and body[j] in "0123456789abcdefABCDEF":
                j += 1
            out.append(int(body[i + 2:j], 16) & 0xFF)
            i = j
        else:
            raise ValueError(f"unknown escape in {spelling}")
    return bytes(out)


def _resource_text(spelling: str) -> str:
    """The text of an RC string: L"\\xNNNN..." (source) or "..." with \"\" (classic)."""
    if spelling.startswith('L"'):
        body = spelling[2:-1]
        return "".join(chr(int(m, 16)) for m in re.findall(r"\\x([0-9a-fA-F]{4})", body))
    return spelling[1:-1].replace('""', '"').replace("\\n", "\n").replace("\\t", "\t")


def _significant(text: str, **kind) -> list[tuple[str, str]]:
    from homm1.clean.source import tokens
    return [(k, s) for k, s in tokens(text, **kind) if k != "space"]


def _equivalent_cpp(classic: str, reference: str) -> str | None:
    """None when `classic` equals `reference` token for token, a classic
    literal standing for the reference literal (or brace-enclosed character
    initializer) with the same Windows-1251 bytes; else a description."""
    ours, theirs = _significant(classic), _significant(reference)
    i = j = 0
    while i < len(ours) and j < len(theirs):
        (kind, spelling), (their_kind, their_spelling) = ours[i], theirs[j]
        if (kind, spelling) == (their_kind, their_spelling):
            i, j = i + 1, j + 1
            continue
        if kind == "literal" and spelling.startswith('"'):
            mine = literal_bytes(spelling)
            if their_kind == "literal" and their_spelling.startswith('"') \
                    and literal_bytes(their_spelling) == mine:
                i, j = i + 1, j + 1
                continue
            if their_spelling == "{":
                # A fixed-width character array: {'a', 'b', ...} without a terminator.
                k, chars = j + 1, bytearray()
                while k < len(theirs) and theirs[k][0] == "literal" \
                        and theirs[k][1].startswith("'"):
                    chars += literal_bytes(theirs[k][1])
                    k += 1
                    if theirs[k][1] == ",":
                        k += 1
                if k < len(theirs) and theirs[k][1] == "}" and bytes(chars) == mine:
                    i, j = i + 1, k + 1
                    continue
        return f"{spelling!r} against {their_spelling!r}"
    if i != len(ours) or j != len(theirs):
        return "different length"
    return None


def classic_equivalence(classic_tree: Path, localized: Path, source_tree: Path) -> list[str]:
    """Differences between the classic tree and the source tree's Russian
    compiler input (`localized`, from localize())."""
    from homm1.clean.classic import resolve_conditionals
    from homm1.graph.catalog import Catalog
    problems = []
    classic_files = {p.relative_to(classic_tree).as_posix() for p in classic_tree.rglob("*")
                     if p.is_file()}
    source_files = {p.relative_to(source_tree).as_posix() for p in source_tree.rglob("*")
                    if p.is_file() and not p.relative_to(source_tree).parts[0] == "build"}
    build_only = {"build.py", "build.json", "flake.nix", "flake.lock", "catalog.py"}
    expected = {name for name in source_files
                if name not in build_only and not name.startswith("locales/")}
    for name in sorted(expected ^ (classic_files - {".homm1-clean-generated"})):
        if name in ("README.md", ".homm1-clean-generated"):
            continue
        problems.append(f"{name}: only in {'classic' if name in classic_files else 'source'}")
    catalog = Catalog.load(source_tree)
    for name in sorted(expected & classic_files):
        if name == "README.md":
            continue
        ours = (classic_tree / name).read_text(encoding="utf-8")
        suffix = Path(name).suffix.lower()
        if suffix in (".cpp", ".h", ".c", ".hpp", ".inc") and name.split("/")[0] in (
                "src", "include", "vendor"):
            reference = resolve_conditionals((localized / name).read_text(), "1")
            difference = _equivalent_cpp(ours, reference)
        elif suffix == ".rc":
            reference = catalog.render_resource((source_tree / name).read_text(), locale="ru")
            difference = _equivalent_rc(ours, reference)
        else:
            difference = None if ours == (source_tree / name).read_text(encoding="utf-8") \
                else "content differs"
        if difference:
            problems.append(f"{name}: {difference}")
    return problems


def _rc_tokens(text: str) -> list[tuple[str, str]]:
    """Significant RC tokens with a wide-string prefix joined to its literal
    (the lexer reads `L"..."` as the word `L` and a string)."""
    out: list[tuple[str, str]] = []
    for kind, spelling in _significant(text, rc=True):
        if kind == "literal" and spelling.startswith('"') and out and out[-1] == ("word", "L"):
            out[-1] = (kind, "L" + spelling)
        else:
            out.append((kind, spelling))
    return out


def _equivalent_rc(classic: str, reference: str) -> str | None:
    """The classic script against the source's rendered script: the language
    #define becomes its value, the UTF-8 code page pragma is new, and each
    string must carry the same text."""
    from homm1.clean.classic import LANG_RUSSIAN, RESOURCE_LANGUAGE
    lines = reference.split("\n")
    define = f"#define {RESOURCE_LANGUAGE} {LANG_RUSSIAN}"
    if lines[0] != define:
        return f"unexpected rendered header {lines[0]!r}"
    reference = "\n".join(lines[1:]).replace(RESOURCE_LANGUAGE, LANG_RUSSIAN)
    classic = classic.replace("#pragma code_page(65001)", "", 1)
    ours, theirs = _rc_tokens(classic), _rc_tokens(reference)
    if len(ours) != len(theirs):
        return "different length"
    for (kind, spelling), (_k, their_spelling) in zip(ours, theirs):
        if spelling == their_spelling:
            continue
        if kind == "literal" and _resource_text(spelling) == _resource_text(their_spelling):
            continue
        return f"{spelling!r} against {their_spelling!r}"
    return None


def standalone(tree: Path) -> int:
    """Run the tree's own build (its flake's toolchain, Wine and llvm-rc) for
    both languages."""
    import os
    import subprocess
    from homm1.core.paths import retail_exe

    env = {key: value for key, value in os.environ.items()
           if key not in ("WINEPREFIX", "PYTHONPATH", "HOMM1_TOOLCHAIN", "MSVC_DIR")}
    executable = json.loads((tree / "build.json").read_text())["executable"]
    for locale in ("ru", "en"):
        command = ["nix", "develop", f"path:{tree}", "-c", "python3", "build.py",
                   "--locale", locale]
        if retail_exe().is_file():
            command += ["--icon-from", str(retail_exe())]
        print(f"[clean] verify: standalone: {' '.join(command[:3])} -c python3 build.py "
              f"--locale {locale}")
        result = subprocess.run(command, cwd=tree, env=env, capture_output=True, text=True)
        exe = tree / "build" / locale / executable
        if result.returncode or not exe.is_file():
            print("\n".join((result.stdout + result.stderr).strip().splitlines()[-20:]),
                  file=sys.stderr)
            print("[clean] verify: FAIL: the tree's own build failed", file=sys.stderr)
            return 1
        print(f"[clean] verify: standalone: built {exe.relative_to(tree)} "
              f"({exe.stat().st_size:,} B) with the flake's pinned toolchain")
    return 0
