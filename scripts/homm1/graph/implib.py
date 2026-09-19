"""homm1.graph.implib - synthesise the import LIBs the toolchain does not ship.

    python3 -m homm1.graph.implib            # synthesise every missing lib
    python3 -m homm1.graph.implib --list     # report coverage, build nothing

Retail HEROES.EXE load-time-imports nine DLLs. The Win32 set has import libs in
VC4; WinG and the two RAD-era vendor DLLs do not. Named imports can be rebuilt
from the retail import table itself. smkwai32's ordinal-only imports remain a
reported link input gap until its original import library or reviewed map is
available.
This module rebuilds the missing `.lib` from the RETAIL IMPORT TABLE, which is
ground truth: the names stored there (`_AIL_startup@0`) are exactly what the
original import lib produced, decoration and all.

Why not `LIB /DEF:`: LIB.EXE derives an import lib's public symbol by PREFIXING
an underscore, so a def naming the true export `_AIL_startup@0` yields
`__imp___AIL_startup@0` (one underscore too many), while a def naming
`AIL_startup@0` yields the right symbol but the wrong hint/name string in
`.idata$6`. Neither is faithful. Instead we do what the SDK vendor did: compile
a throwaway stub DLL whose exports are `__declspec(dllexport) __stdcall`
functions with the matching argument-byte count, and keep the `/IMPLIB:` link
emits for it.

Hints are reproduced too. A `.idata$6` hint is the export's index in the DLL's
SORTED export-name table, so the vendor's lib carries the index each name had in
the real DLL's full export list, and retail's import table stores those values
byte-for-byte - which makes retail itself the evidence for the vendor DLL's name
table. The stub reproduces it with `__cdecl` FILLER exports (export name = the
bare identifier) that sort strictly between the real decorated names, one per
unclaimed index. Retail's hints are strictly ascending in sorted-name order for
both DLLs (asserted), which is what "indices into one sorted name table" implies,
so the interleave always exists. Fillers never reach the image: nothing
references them, so no member of theirs is pulled. `_verify_hints` re-reads the
produced lib's `.idata$6` and fails on any mismatch.

The stub DLL is discarded; only the `.lib` is a build input, and nothing here
needs the real MSS32/SMACKW32 DLLs (those are runtime-only).
"""

from __future__ import annotations

import re
import struct
from pathlib import Path

from homm1.core.paths import BUILD, dxsdk_dir, msvc_dir
from homm1.core.pe import Pe, image
from homm1.tool import ToolError
from homm1.tool.wine import find_ci, winepath

OUT_DIR = BUILD / "lib"

#: `_name@n` = __stdcall (n = argument bytes); a bare `name` = __cdecl/data.
STDCALL = re.compile(r"^_(?P<name>[A-Za-z_][A-Za-z0-9_]*)@(?P<bytes>\d+)$")
PLAIN = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")

#: Filler export names are valid C identifiers - they are compiled as `__cdecl`
#: functions, and a cdecl dllexport's export-table string is the identifier as
#: written - generated to sort strictly between two decorated real names.
_IDENT = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz"


# --------------------------------------------------------------------------- #
# retail's import table
# --------------------------------------------------------------------------- #
def _off(pe: Pe, rva: int) -> int:
    for s in pe.sections:
        if s["va"] <= rva < s["va"] + max(s["vsize"], s["rsize"]):
            return s["rptr"] + rva - s["va"]
    raise ValueError(f"rva 0x{rva:x} is in no section")


def _cstr(pe: Pe, rva: int) -> str:
    off = _off(pe, rva)
    end = pe.data.index(b"\0", off)
    return pe.data[off:end].decode("latin-1")


def import_table(pe: Pe | None = None) -> dict[str, dict[str, int]]:
    """{dll: {hint/name string: hint}} over the whole import directory.

    ORDINAL-only imports carry no name and are skipped - retail imports every
    RAD entry by name, and an ordinal one could not be expressed as a stub
    export anyway (it would need a hand-written .def).
    """
    pe = pe or image()
    d = pe.data
    opt = struct.unpack_from("<I", d, 0x3C)[0] + 24
    rva = struct.unpack_from("<I", d, opt + 96 + 1 * 8)[0]   # DataDirectory[1]
    out: dict[str, dict[str, int]] = {}
    o = _off(pe, rva)
    while True:
        olt, _ts, _fc, nm, fta = struct.unpack_from("<IIIII", d, o)
        if not (olt or nm or fta):
            break
        names: dict[str, int] = {}
        t = _off(pe, olt or fta)
        while True:
            v = struct.unpack_from("<I", d, t)[0]
            if v == 0:
                break
            if not (v & 0x80000000):
                hn = v & 0x7FFFFFFF
                names[_cstr(pe, hn + 2)] = struct.unpack_from("<H", d, _off(pe, hn))[0]
            t += 4
        out[_cstr(pe, nm)] = names
        o += 20
    return out


# --------------------------------------------------------------------------- #
# what the toolchain already covers
# --------------------------------------------------------------------------- #
def lib_dirs() -> list[Path]:
    """DX6 first, then VC5 - the precedence init_prefix writes into wine's LIB."""
    dirs = []
    for root, sub in ((dxsdk_dir, "Lib"), (msvc_dir, "lib")):
        try:
            dirs.append(root() / sub)
        except RuntimeError:
            continue          # outside `nix develop`: that half is uncovered
    return [d for d in dirs if d.is_dir()]


def toolchain_lib(stem: str) -> Path | None:
    for d in lib_dirs():
        hit = find_ci(d, f"{stem}.lib")
        if hit:
            return hit
    return None


def survey() -> list[tuple[str, dict[str, int], Path | None]]:
    """[(dll, {name: hint}, existing_lib_or_None)] over retail's imports."""
    return [(dll, names, toolchain_lib(Path(dll).stem))
            for dll, names in import_table().items()]


# --------------------------------------------------------------------------- #
# the stub DLL
# --------------------------------------------------------------------------- #
def _gap_base(a: str | None, b: str) -> str:
    """An identifier `base` with a < base + <digits> < b bytewise.

    `a` may be None (any base < b works). Walk the common prefix; at the first
    divergence take a character strictly between; when the two are adjacent,
    extend past `a`'s next character instead. The prefix of a decorated import
    name up to any divergence below '@' is identifier-clean (asserted).
    """
    ident = sorted(_IDENT)
    if a is None:
        for j in range(len(b)):
            lo = [c for c in ident if c < b[j]]
            if lo:
                base = b[:j] + lo[-1]
                if all(c in _IDENT for c in base):
                    return base
        raise ToolError(f"no filler name sorts below {b!r}")
    i = 0
    while i < len(a) and i < len(b) and a[i] == b[i]:
        i += 1
    assert i < len(b), f"{a!r} !< {b!r}"
    mid = [c for c in ident if (i >= len(a) or c > a[i]) and c < b[i]]
    if mid:
        base = a[:i] + mid[0]
    else:
        # adjacent characters: step inside `a` and clear its tail instead
        nxt = [c for c in ident if c > (a[i + 1] if i + 1 < len(a) else "")]
        assert nxt, f"cannot split the gap {a!r} .. {b!r}"
        base = a[:i + 1] + nxt[0]
    assert all(c in _IDENT for c in base), (a, b, base)
    assert a < base < b, (a, b, base)
    return base


def export_table(names, hints: dict[str, int]) -> list[tuple[str, bool]]:
    """[(export_name, is_filler)] sorted, fillers padding every index below the
    retail hint of each real name so the stub's sorted name table puts each real
    export at exactly its retail index."""
    real = sorted(names)
    hs = [hints[n] for n in real]
    if hs != sorted(hs) or len(set(hs)) != len(hs):
        raise ToolError("retail hints are not ascending in sorted-name order - "
                        "not one sorted name table?")
    table: list[tuple[str, bool]] = []
    prev = None
    pos = 0
    for n, h in zip(real, hs):
        k = h - pos
        if k:
            base = _gap_base(prev, n)
            width = len(str(k - 1))
            fillers = [f"{base}{i:0{width}d}" for i in range(k)]
            assert fillers == sorted(fillers) and fillers[-1] < n, (prev, n, k)
            table += [(f, True) for f in fillers]
            pos += k
        table.append((n, False))
        prev = n
        pos += 1
    flat = [x for x, _f in table]
    assert all(x < y for x, y in zip(flat, flat[1:])), "table not strictly sorted"
    return table


def stub_source(dll: str, names, hints: dict[str, int] | None = None) -> str:
    """C for a stub DLL whose exports decorate to exactly `names`, padded with
    fillers so each name's sorted-name-table index matches retail's hint."""
    lines = [f"/* GENERATED by homm1.graph.implib - stub exports for {dll}.",
             "   Bodies are irrelevant: only the DECORATED export names and their",
             "   sorted-name-table INDICES (the hints) matter, and both come from",
             "   retail HOMM1.EXE's own import table. */"]
    table = (export_table(names, hints) if hints and all(n in hints for n in names)
             else [(n, False) for n in sorted(names)])
    for n, filler in table:
        if filler:
            lines.append(f"__declspec(dllexport) void {n}(void) {{}}")
            continue
        m = STDCALL.match(n)
        if m:
            nargs, rem = divmod(int(m.group("bytes")), 4)
            if rem:
                raise ToolError(f"{dll}: {n} has a non-dword argument size - "
                                "cannot express as a __stdcall prototype")
            # C definitions need NAMED formals (C2055) though nothing uses them.
            args = ", ".join(f"int a{i}" for i in range(nargs)) or "void"
            lines.append(f"__declspec(dllexport) void __stdcall "
                         f"{m.group('name')}({args}) {{}}")
        elif PLAIN.match(n):
            lines.append(f"__declspec(dllexport) void {n}(void) {{}}")
        else:
            raise ToolError(f"{dll}: cannot synthesise an export for {n!r} "
                            "(ordinal-only or fastcall needs a hand-written .def)")
    return "\n".join(lines) + "\n"


def _verify_hints(lib: Path, want: dict[str, int]) -> None:
    """Fail unless every hint/name blob in `lib`'s .idata$6 matches `want`.

    The hint a member carries is what the linker copies into the image, so this
    re-reads the produced archive rather than trusting the export-table maths.
    """
    data = lib.read_bytes()
    if data[:8] != b"!<arch>\n":
        raise ToolError(f"{lib}: not an archive")
    got: dict[str, int] = {}
    off = 8
    while off + 60 <= len(data):
        size = int(data[off + 48:off + 58].decode().strip() or "0")
        body = off + 60
        m = data[body:body + size]
        if len(m) > 20 and m[:4] != b"\xff\xff\0\0":      # skip linker members
            try:
                nsec = struct.unpack_from("<H", m, 2)[0]
                for i in range(nsec):
                    raw = m[20 + 40 * i:20 + 40 * (i + 1)]
                    if raw[:8].rstrip(b"\0") != b".idata$6":
                        continue
                    rsz, rp = struct.unpack_from("<II", raw, 16)
                    blob = m[rp:rp + rsz]
                    if len(blob) > 3:
                        hint = struct.unpack_from("<H", blob, 0)[0]
                        name = blob[2:blob.find(b"\0", 2)].decode("latin-1")
                        if name in want:
                            got[name] = hint
            except (struct.error, ValueError):
                pass
        off = body + size + (size & 1)
    bad = {n: (want[n], got.get(n)) for n in want if got.get(n) != want[n]}
    if bad:
        raise ToolError(f"{lib.name}: hint mismatch after synthesis: {bad}")


def synthesize(dll: str, hints: dict[str, int], out_dir: Path = OUT_DIR,
               verbose: bool = True) -> Path:
    """Build `<out_dir>/<stem>.lib` for `dll`; returns the lib path."""
    from homm1.tool import cl, link
    from homm1.tool.wine import era_tool

    out_dir.mkdir(parents=True, exist_ok=True)
    stem = Path(dll).stem
    names = sorted(hints)
    src, obj = out_dir / f"{stem}_stub.c", out_dir / f"{stem}_stub.obj"
    lib, stub_dll = out_dir / f"{stem}.lib", out_dir / dll   # /OUT name == the
    src.write_text(stub_source(dll, names, hints))           # recorded DLL name
    for f in (obj, stub_dll):
        f.unlink(missing_ok=True)
    # link into a temp name so a failed synthesis never destroys a good lib
    tmp_lib = lib.with_suffix(".lib.tmp")
    tmp_lib.unlink(missing_ok=True)

    era_tool("cl.exe")                       # fail early with the toolchain hint
    cl.compile(src, obj, ["/nologo", "/c"])
    link.link(["/NOLOGO", "/DLL", "/NOENTRY", "/NODEFAULTLIB",
               f"/OUT:{winepath(stub_dll)}",
               f"/IMPLIB:{winepath(tmp_lib)}", winepath(obj)],
              cwd=out_dir, expect=[tmp_lib])
    tmp_lib.replace(lib)
    # The stub DLL and its .exp are scaffolding; only the .lib is a build input.
    # link.exe names the .exp after the /IMPLIB path, so the temp lib's name is
    # what it carries - `<stem>.exp` is a file that never existed, and the two
    # real ones sat in build/lib/ forever.
    for f in (stub_dll, tmp_lib.with_suffix(".exp"), out_dir / f"{stem}.exp",
              obj):
        f.unlink(missing_ok=True)
    _verify_hints(lib, hints)
    if verbose:
        print(f"[implib] {dll}: {len(names)} import(s) -> {lib} "
              f"(hints verified against retail)")
    return lib


def on_disk(out_dir: Path = OUT_DIR) -> list[Path]:
    """The already-synthesised libs, without building anything.

    For inspection paths (a dry-run link) that must not need a linker.
    """
    return [out_dir / f"{Path(dll).stem}.lib"
            for dll, _n, existing in survey()
            if not existing and (out_dir / f"{Path(dll).stem}.lib").exists()]


def ensure_all(out_dir: Path = OUT_DIR, verbose: bool = True) -> list[Path]:
    """Synthesise every import lib the toolchain lacks; returns their paths.

    Cached: a lib newer than both the retail image and this module is reused.
    """
    from homm1.core.paths import retail_exe
    stamp = max(p.stat().st_mtime
                for p in (Path(__file__), retail_exe()) if p.exists())
    libs = []
    for dll, hints, existing in survey():
        if existing:
            continue
        if not hints:
            if verbose:
                print(f"[implib] {dll}: ordinal-only imports; no names in "
                      "retail, so synthesis is deferred")
            continue
        lib = out_dir / f"{Path(dll).stem}.lib"
        if lib.exists() and lib.stat().st_mtime >= stamp:
            libs.append(lib)
            continue
        libs.append(synthesize(dll, hints, out_dir, verbose))
    return libs


def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--list", action="store_true",
                    help="report which imported DLLs have a lib; build nothing")
    ap.add_argument("--out-dir", type=Path, default=OUT_DIR)
    a = ap.parse_args()
    try:
        if a.list:
            for dll, names, existing in survey():
                where = existing or "** no lib - synthesised **"
                print(f"{dll:16s} {len(names):4d} import(s)  {where}")
            return 0
        libs = ensure_all(a.out_dir)
    except (ToolError, RuntimeError) as e:
        print(f"[implib] {e}", file=sys.stderr)
        return 1
    except OSError as e:
        # Every path here reads retail's own import table; without the image
        # this was a FileNotFoundError traceback out of homm1.core.pe.
        print(f"[implib] cannot read the retail image - the import table is "
              f"the ONLY source for these libs: {e}", file=sys.stderr)
        return 1
    print(f"[implib] {len(libs)} synthesised lib(s) in {a.out_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
