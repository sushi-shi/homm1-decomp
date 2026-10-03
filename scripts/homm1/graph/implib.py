"""homm1.graph.implib - synthesise the import LIBs the toolchain does not ship.

    python3 -m homm1.graph.implib            # synthesise every missing lib
    python3 -m homm1.graph.implib --list     # report coverage, build nothing

Retail HEROES.EXE load-time-imports nine DLLs. The Win32 set has import libs in
VC4; WinG and the two RAD-era vendor DLLs do not. Named imports are rebuilt
from the retail import table itself. Two facts the table cannot state come from
the reviewed import-thunk rows of config/retail/function_referents.tsv, joined
to an IAT slot through the thunk's own `jmp [slot]` bytes:

  * the caller-visible decoration of an UNDECORATED export (WinG exports
    `WinGBitBlt`; its callers and its import lib say `_WinGBitBlt@32`);
  * the name of an ORDINAL-only import (smkwai32 imports `#14`; the reviewed
    thunk row names it `_SmackClose`).

Both are expressed through a `.def` the stub link consumes: a bare entry
`WinGBitBlt` lets LINK bind the decorated stdcall body and emit the import
under the undecorated name, and `SmackClose @14 NONAME` emits an ordinal
import whose public symbol is the cdecl `_SmackClose`.
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

The library's own shape also reaches the image. config/retail/
import_libraries.tsv records the vendor libraries that retail shows were not
VC4 import libraries: WING32's was the VC 2.0 (LINK 2.50) format, whose own
`NULL_IMPORT_DESCRIPTOR` adds a second .idata$3 terminator, and its archive
members sort after wail32's, which places its ILT/IAT last. Such a library is
linked with the pinned VC 2.0 LINK and its member headers are renamed.

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

    ORDINAL-only imports carry no name and are skipped here; `ordinal_imports`
    lists them and `referent_imports` supplies their reviewed names.
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


def import_slots(pe: Pe | None = None) -> dict[int, tuple[str, str | int]]:
    """{IAT slot rva: (dll, name or ordinal)} over the whole import directory."""
    pe = pe or image()
    d = pe.data
    opt = struct.unpack_from("<I", d, 0x3C)[0] + 24
    rva = struct.unpack_from("<I", d, opt + 96 + 1 * 8)[0]
    out: dict[int, tuple[str, str | int]] = {}
    o = _off(pe, rva)
    while True:
        olt, _ts, _fc, nm, fta = struct.unpack_from("<IIIII", d, o)
        if not (olt or nm or fta):
            break
        dll = _cstr(pe, nm)
        t = _off(pe, olt or fta)
        k = 0
        while True:
            v = struct.unpack_from("<I", d, t + 4 * k)[0]
            if v == 0:
                break
            out[fta + 4 * k] = (dll, v & 0xFFFF if v & 0x80000000
                                else _cstr(pe, (v & 0x7FFFFFFF) + 2))
            k += 1
        o += 20
    return out


def referent_imports(pe: Pe | None = None
                     ) -> dict[str, dict[str | int, str]]:
    """{dll: {retail name or ordinal: reviewed decorated symbol}}.

    Each reviewed row of FUNCTION_REFERENTS whose retail bytes are an import
    thunk (`FF 25 <slot>`) names what that slot's callers link against. A row
    whose slot is no import, or two names for one slot, is a review defect and
    fails rather than being guessed around.
    """
    from homm1.core.paths import RETAIL
    pe = pe or image()
    slots = import_slots(pe)
    out: dict[str, dict[str | int, str]] = {}
    for ln in (RETAIL / "function_referents.tsv").read_text().splitlines():
        if not ln or ln.startswith("#") or ln.startswith("rva\t"):
            continue
        rva_s, sym = ln.split("\t")[:2]
        code = pe.read(int(rva_s, 16), 6)
        if not code or code[:2] != b"\xff\x25":
            continue
        slot = struct.unpack_from("<I", code, 2)[0] - pe.image_base
        if slot not in slots:
            raise ToolError(f"function_referents {rva_s} {sym}: thunk slot "
                            f"0x{slot:x} is no import-table entry")
        dll, key = slots[slot]
        have = out.setdefault(dll, {}).setdefault(key, sym)
        if have != sym:
            raise ToolError(f"{dll} {key!r}: reviewed as both {have} and {sym}")
    return out


# --------------------------------------------------------------------------- #
# what the toolchain already covers
# --------------------------------------------------------------------------- #
def lib_dirs() -> list[Path]:
    """The pinned vendor SDKs' lib/ first (their real import libraries, e.g.
    WinG 1.0's wing32.lib), then DX6, then VC - the precedence init_prefix
    writes into wine's LIB."""
    from homm1.core.paths import sdk_lib_dirs
    dirs = list(sdk_lib_dirs())
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


def _stdcall_body(name: str, nbytes: int, dll: str, sym: str) -> str:
    nargs, rem = divmod(nbytes, 4)
    if rem:
        raise ToolError(f"{dll}: {sym} has a non-dword argument size - "
                        "cannot express as a __stdcall prototype")
    # C definitions need NAMED formals (C2055) though nothing uses them.
    args = ", ".join(f"int a{i}" for i in range(nargs)) or "void"
    return f"void __stdcall {name}({args}) {{}}"


def def_source(dll: str, entries: list[str]) -> str:
    """The `.def` naming the exports a C declaration cannot express."""
    return "\n".join([f"; GENERATED by homm1.graph.implib for {dll}",
                      f"LIBRARY {Path(dll).stem}", "EXPORTS",
                      *[f"    {e}" for e in entries]]) + "\n"


def stub_source(dll: str, names, hints: dict[str, int] | None = None,
                decorated: dict[str | int, str] | None = None
                ) -> tuple[str, list[str]]:
    """C for a stub DLL whose exports decorate to exactly `names`, padded with
    fillers so each name's sorted-name-table index matches retail's hint,
    plus the `.def` entries for exports whose caller-visible symbol is not
    the export name (`decorated`: retail name or ordinal -> reviewed symbol).
    """
    decorated = decorated or {}
    lines = [f"/* GENERATED by homm1.graph.implib - stub exports for {dll}.",
             "   Bodies are irrelevant: only the DECORATED export names and their",
             "   sorted-name-table INDICES (the hints) matter, and both come from",
             "   retail HOMM1.EXE's own import table and its reviewed thunks. */"]
    entries: list[str] = []
    table = (export_table(names, hints) if hints and all(n in hints for n in names)
             else [(n, False) for n in sorted(names)])
    for n, filler in table:
        if filler:
            lines.append(f"__declspec(dllexport) void {n}(void) {{}}")
            continue
        sym = decorated.get(n)
        if sym is not None and sym != n and PLAIN.match(n):
            # An undecorated export with a decorated caller-side symbol: a
            # bare .def entry binds `_n@N` and keeps the export name `n`.
            m = STDCALL.match(sym)
            if not m or m.group("name") != n:
                raise ToolError(f"{dll}: reviewed symbol {sym} does not "
                                f"decorate the export {n}")
            lines.append(_stdcall_body(n, int(m.group("bytes")), dll, sym))
            entries.append(n)
            continue
        m = STDCALL.match(n)
        if m:
            lines.append("__declspec(dllexport) " + _stdcall_body(
                m.group("name"), int(m.group("bytes")), dll, n))
        elif PLAIN.match(n):
            lines.append(f"__declspec(dllexport) void {n}(void) {{}}")
        else:
            raise ToolError(f"{dll}: cannot synthesise an export for {n!r} "
                            "(fastcall needs a hand-written .def)")
    for ordinal, sym in sorted((k, v) for k, v in decorated.items()
                               if isinstance(k, int)):
        # Ordinal-only import: no name reaches the image, so the export name
        # is the reviewed symbol's undecorated identifier.
        m = STDCALL.match(sym)
        if m:
            lines.append(_stdcall_body(m.group("name"), int(m.group("bytes")),
                                       dll, sym))
            name = m.group("name")
        elif sym.startswith("_") and PLAIN.match(sym[1:]):
            name = sym[1:]
            lines.append(f"void {name}(void) {{}}")
        else:
            raise ToolError(f"{dll}: ordinal {ordinal} reviewed as {sym!r}, "
                            "which is no C/stdcall symbol")
        entries.append(f"{name} @{ordinal} NONAME")
    return "\n".join(lines) + "\n", entries


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


def _lib_publics(lib: Path) -> set[str]:
    """Public symbols of an archive, from its first linker member."""
    data = lib.read_bytes()
    size = int(data[8 + 48:8 + 58].decode().strip() or "0")
    m = data[68:68 + size]
    n = struct.unpack_from(">I", m, 0)[0]
    names = m[4 + 4 * n:].split(b"\0")[:n]
    return {x.decode("latin-1") for x in names}


def _verify_ordinals(lib: Path, want: dict[int, str]) -> None:
    """Fail unless each reviewed symbol's member imports exactly its ordinal."""
    data = lib.read_bytes()
    got: dict[str, int] = {}
    off = 8
    while off + 60 <= len(data):
        size = int(data[off + 48:off + 58].decode().strip() or "0")
        m = data[off + 60:off + 60 + size]
        if len(m) > 20 and m[:4] != b"\xff\xff\0\0":
            try:
                nsec = struct.unpack_from("<H", m, 2)[0]
                symp, nsym = struct.unpack_from("<II", m, 8)
                ordinal = None
                for i in range(nsec):
                    raw = m[20 + 40 * i:20 + 40 * (i + 1)]
                    if raw[:8].rstrip(b"\0") == b".idata$5":
                        rsz, rp = struct.unpack_from("<II", raw, 16)
                        v = struct.unpack_from("<I", m, rp)[0] if rsz >= 4 else 0
                        if v & 0x80000000:
                            ordinal = v & 0xFFFF
                if ordinal is not None and symp and nsym:
                    strt = symp + nsym * 18
                    for k in range(nsym):
                        o = symp + 18 * k
                        raw = m[o:o + 8]
                        if raw[:4] == b"\0\0\0\0":
                            so = strt + struct.unpack_from("<I", raw, 4)[0]
                            name = m[so:m.index(b"\0", so)].decode("latin-1")
                        else:
                            name = raw.rstrip(b"\0").decode("latin-1")
                        if name in want.values():
                            got[name] = ordinal
            except (struct.error, ValueError):
                pass
        off = off + 60 + size + (size & 1)
    bad = {s: (o, got.get(s)) for o, s in want.items() if got.get(s) != o}
    if bad:
        raise ToolError(f"{lib.name}: ordinal mismatch after synthesis: {bad}")


def lib_shapes() -> dict[str, dict[str, str]]:
    """{dll: {format, member}} from config/retail/import_libraries.tsv.

    A vendor library absent from the table takes the pinned VC4 LINK's
    import format and its default member name (the DLL name).
    """
    from homm1.core.paths import RETAIL
    path = RETAIL / "import_libraries.tsv"
    out: dict[str, dict[str, str]] = {}
    if not path.exists():
        return out
    for ln in path.read_text().splitlines():
        if not ln or ln.startswith("#") or ln.startswith("dll\t"):
            continue
        dll, fmt, member = ln.split("\t")[:3]
        if fmt not in SHAPE_LINKERS:
            raise ToolError(f"import_libraries.tsv {dll}: unknown format {fmt!r}")
        if not 0 < len(member) < 16 or "/" in member:
            raise ToolError(f"import_libraries.tsv {dll}: member name "
                            f"{member!r} does not fit an archive header")
        out[dll] = {"format": fmt, "member": member}
    return out


#: Import-library formats and the pinned toolchain whose LINK emits them.
#: vc4: `__IMPORT_DESCRIPTOR_<DLL>` + `__NULL_IMPORT_DESCRIPTOR` (LINK 3.00).
#: vc2: `<DLL>_IMPORT_DESCRIPTOR` + `NULL_IMPORT_DESCRIPTOR` (LINK 2.50, the
#: format of the 1994 SDK libraries still in VC4's lib/, e.g. ctl3d32.lib).
#: The two null-descriptor symbols differ, so a vc2 library linked beside the
#: VC4 Win32 libraries adds a second 20-byte .idata$3 terminator.
SHAPE_LINKERS = {"vc4": "vc40", "vc2": "vc20"}


def _shape_linker(fmt: str, dll: str, verbose: bool) -> Path | None:
    """The LINK.EXE for `fmt`, or None for the default VC4 linker.

    A missing VC 2.0 toolchain falls back to VC4 with a warning, so the
    candidate still links; its .idata then lacks the vendor shape.
    """
    if fmt == "vc4":
        return None
    from homm1 import toolchain
    from homm1.tool.wine import find_ci
    name = SHAPE_LINKERS[fmt]
    try:
        toolchain.verify(name)
    except (KeyError, ValueError) as e:
        if verbose:
            print(f"[implib] {dll}: {fmt} import format needs the pinned "
                  f"{name} LINK ({e}); using VC4's format instead, so the "
                  f"candidate .idata will not match retail")
        return None
    return find_ci(toolchain.root(name) / "bin", "link.exe")


def _rename_members(lib: Path, member: str) -> None:
    """Rename every import member of `lib` to `member` (header field only).

    LINK 3.00 orders the .idata$4/$5 groups by archive member name. Only the
    16-byte header name changes: symbols, offsets and sizes stay as emitted.
    """
    data = bytearray(lib.read_bytes())
    field = (member + "/").ljust(16).encode("ascii")
    off = 8
    renamed = 0
    while off + 60 <= len(data):
        size = int(data[off + 48:off + 58].decode().strip() or "0")
        name = bytes(data[off:off + 16]).rstrip()
        if not name.startswith(b"/"):
            data[off:off + 16] = field
            renamed += 1
        off += 60 + size + (size & 1)
    if not renamed:
        raise ToolError(f"{lib.name}: no import member to rename")
    lib.write_bytes(bytes(data))


def synthesize(dll: str, hints: dict[str, int], out_dir: Path = OUT_DIR,
               verbose: bool = True,
               decorated: dict[str | int, str] | None = None,
               shape: dict[str, str] | None = None) -> Path:
    """Build `<out_dir>/<stem>.lib` for `dll`; returns the lib path.

    `decorated` (from `referent_imports`) supplies the reviewed caller-side
    symbol of undecorated and ordinal-only imports. `shape` (from
    `lib_shapes`) selects the vendor library's import format and member name.
    """
    from homm1.tool import cl, link
    from homm1.tool.wine import era_tool

    decorated = decorated or {}
    shape = shape or {}
    linker = _shape_linker(shape.get("format", "vc4"), dll, verbose)
    out_dir.mkdir(parents=True, exist_ok=True)
    stem = Path(dll).stem
    names = sorted(hints)
    src, obj = out_dir / f"{stem}_stub.c", out_dir / f"{stem}_stub.obj"
    deff = out_dir / f"{stem}_stub.def"
    lib, stub_dll = out_dir / f"{stem}.lib", out_dir / dll   # /OUT name == the
    code, entries = stub_source(dll, names, hints, decorated)  # recorded DLL
    src.write_text(code)
    for f in (obj, stub_dll, deff):
        f.unlink(missing_ok=True)
    if entries:
        deff.write_text(def_source(dll, entries))
    # link into a temp name so a failed synthesis never destroys a good lib
    tmp_lib = lib.with_suffix(".lib.tmp")
    tmp_lib.unlink(missing_ok=True)

    era_tool("cl.exe")                       # fail early with the toolchain hint
    cl.compile(src, obj, ["/nologo", "/c"])
    link.link(["/NOLOGO", "/DLL", "/NOENTRY", "/NODEFAULTLIB",
               *([f"/DEF:{winepath(deff)}"] if entries else []),
               f"/OUT:{winepath(stub_dll)}",
               f"/IMPLIB:{winepath(tmp_lib)}", winepath(obj)],
              cwd=out_dir, expect=[tmp_lib], exe=linker)
    if shape.get("member") and linker is not None:
        _rename_members(tmp_lib, shape["member"])
    tmp_lib.replace(lib)
    # The stub DLL and its .exp are scaffolding; only the .lib is a build input.
    # link.exe names the .exp after the /IMPLIB path, so the temp lib's name is
    # what it carries - `<stem>.exp` is a file that never existed, and the two
    # real ones sat in build/lib/ forever.
    for f in (stub_dll, tmp_lib.with_suffix(".exp"), out_dir / f"{stem}.exp",
              obj):
        f.unlink(missing_ok=True)
    _verify_hints(lib, hints)
    _verify_ordinals(lib, {k: v for k, v in decorated.items()
                           if isinstance(k, int)})
    publics = _lib_publics(lib)
    missing = sorted(v for v in decorated.values() if v not in publics)
    if missing:
        raise ToolError(f"{lib.name}: reviewed symbol(s) not public after "
                        f"synthesis: {missing}")
    if verbose:
        nord = sum(isinstance(k, int) for k in decorated)
        form = (f", {shape['format']} format, members {shape['member']}"
                if linker is not None else "")
        print(f"[implib] {dll}: {len(names)} named + {nord} ordinal "
              f"import(s) -> {lib} (hints/ordinals verified against retail"
              f"{form})")
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
    from homm1.core.paths import RETAIL, retail_exe
    stamp = max(p.stat().st_mtime
                for p in (Path(__file__), retail_exe(),
                          RETAIL / "function_referents.tsv",
                          RETAIL / "import_libraries.tsv") if p.exists())
    referents = referent_imports()
    shapes = lib_shapes()
    ordinals = ordinal_imports()
    libs = []
    for dll, hints, existing in survey():
        if existing:
            continue
        decorated = referents.get(dll, {})
        unnamed = sorted(o for o in ordinals.get(dll, ()) if o not in decorated)
        if unnamed and verbose:
            print(f"[implib] {dll}: ordinal(s) {unnamed} have no reviewed "
                  "thunk name in function_referents.tsv; left out")
        if not hints and not any(isinstance(k, int) for k in decorated):
            if verbose:
                print(f"[implib] {dll}: no named or reviewed ordinal imports; "
                      "synthesis is deferred")
            continue
        lib = out_dir / f"{Path(dll).stem}.lib"
        if lib.exists() and lib.stat().st_mtime >= stamp:
            libs.append(lib)
            continue
        libs.append(synthesize(dll, hints, out_dir, verbose, decorated,
                               shapes.get(dll)))
    return libs


def ordinal_imports(pe: Pe | None = None) -> dict[str, set[int]]:
    """{dll: {ordinal}} - the ordinal-only entries `import_table` skips."""
    out: dict[str, set[int]] = {}
    for dll, key in import_slots(pe).values():
        if isinstance(key, int):
            out.setdefault(dll, set()).add(key)
    return out


from homm1.core.usage import logged


@logged
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
            ordinals = ordinal_imports()
            for dll, names, existing in survey():
                where = existing or "** no lib - synthesised **"
                nord = len(ordinals.get(dll, ()))
                print(f"{dll:16s} {len(names):4d} named + {nord:3d} ordinal "
                      f"import(s)  {where}")
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
