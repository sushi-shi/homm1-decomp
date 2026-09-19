"""homm1.verify.undefined_closure - fake views + declared-only aliases (normal).

The MERGED declared_only + view_debt check: one mechanism, one gate. objdiff's
% is gameable by a fake VIEW - a declared-only method makes a call compile and
reloc-masking hides the target - but the SYMBOL survives: an undefined
external that NO base obj defines and retail's delinked namespace never
names. Two reports off one closure:

  * PURE-PHANTOM CLASSES (view_debt): a class with >= 1 such phantom method,
    ZERO defined bodies anywhere, no admitted RTTI/vtable row, and not a
    library class - a fake view by construction. Plus project-local
    definitions that SHADOW a library class. Both FATAL.
  * DECLARED-ONLY debt (declared_only): the rest - free functions, methods on
    real classes, data externs - undefined everywhere, absent from the
    delinked target namespace, and resolvable from no toolchain .LIB.
    Ratcheted against config/cleanliness/declared-only-baseline.tsv (absent
    file = empty set = pure fail-closed).

    python3 -m homm1.verify.undefined_closure [--list] [--update]
"""

from __future__ import annotations

import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

from homm1.core.coff import Coff
from homm1.core.paths import BUILD, CONFIG, REPO

TARGET = BUILD / "delink/named"
BASE = BUILD / "objdiff/base"
BASELINE = CONFIG / "cleanliness/declared-only-baseline.tsv"
INCOMPLETE_TYPES = CONFIG / "cleanliness/types.toml"
LIB_CACHE = BUILD / "gen/lib_symbols.txt"

LIBRARY_CLASSES = {
    "CString", "CObject", "CWnd", "CDialog", "CDC", "CGdiObject", "CFile",
    "CArchive", "CException", "CMemoryException", "CFileException",
    "CNotSupportedException", "CPtrList", "CObList", "CObArray", "CStringList",
    "CPtrArray", "CByteArray", "CDWordArray", "CMapStringToOb",
    "CMapStringToPtr", "CMapPtrToPtr", "CList", "CMap", "CArray", "CRgn",
    "CBitmap", "CFont", "CPen", "CBrush", "CPalette", "CMenu", "CImageList",
    "CTime", "CTimeSpan", "CPoint", "CRect", "CSize", "CRuntimeClass",
    "CCmdTarget", "CWinApp", "CWinThread", "CFrameWnd", "CView", "CDocument",
    "CControlBar", "CStatic", "CButton", "CEdit", "CListBox", "CComboBox",
    "CScrollBar",
}
LIBRARY_EXTRA = {
    "ios", "istream", "ostream", "iostream", "ifstream", "ofstream", "fstream",
    "filebuf", "streambuf", "strstreambuf", "istrstream", "ostrstream",
    "strstream", "stdiobuf", "Iostream_init", "exception", "type_info",
    "bad_cast", "bad_typeid", "CMemFile", "CStdioFile", "CPaintDC",
    "CDumpContext", "CArchiveException", "CSize", "AFX_MODULE_STATE",
    "CStringArray", "CWordArray", "CUIntArray",
}

_EXTERNAL = 2


def live_base_objs() -> list[Path]:
    from homm1 import manifest
    live = {u["unit"] for u in manifest.units()}
    return [p for p in sorted(BASE.glob("*.obj")) if p.stem in live]


def _sym_sets(paths) -> tuple[set[str], set[str]]:
    """(defined, undefined) across `paths` (COMMONs count as defined)."""
    defined, undef = set(), set()
    for p in paths:
        try:
            c = Coff(p)
        except (ValueError, OSError, struct.error):
            continue
        for name, value, sec, st in c.symbols:
            if name.startswith("."):
                continue
            if sec > 0:
                defined.add(name)
            elif sec == 0 and st == _EXTERNAL:
                (defined if value else undef).add(name)
    return defined, undef


#: first line of LIB_CACHE - the archive set the cached symbols came from
_CACHE_STAMP = "# libs "


def _toolchain_libs() -> list[Path]:
    """Every archive the REAL link line searches.

    The toolchain/SDK dirs, PLUS the import libs we synthesize into
    build/lib and pass to link.exe by substitution (homm1.graph.link
    LINK_LIBS/implib): mss32 and smackw32 ship no .LIB, so their
    `__imp__AIL_*` / `__imp__Smack*` imports resolve from ours. Scanning
    only the toolchain made the closure check answer for a link line we do
    not use, and report 26 symbols as guaranteed-unresolved that the real
    link resolves.
    """
    import os
    libs: list[Path] = []
    for env in ("MSVC_DIR", "DXSDK_DIR"):
        d = os.environ.get(env)
        if d:
            for sub in ("lib", "Lib"):
                p = Path(d) / sub
                if p.is_dir():
                    libs += [q for q in p.iterdir()
                             if q.suffix.lower() == ".lib"]
    from homm1.graph import implib
    libs += [p for p in implib.on_disk() if p.is_file()]
    return sorted(libs)


def _lib_stamp(libs: list[Path]) -> str:
    """Identity of the archive SET: path + size of every .LIB.

    The cache is keyed on this because $MSVC_DIR is a nix store path: a
    toolchain bump changes it, and a cache keyed on nothing survives the bump
    and answers for the OLD toolchain forever. Measured 2026-08-16: the r2
    cache was still answering under r3 (46,866 live symbols vs 56,474 cached;
    7,106 live-only, 16,714 cache-only), so `verify link-tier` called 42
    resolvable Win32 imports "a guaranteed unresolved external" while the
    candidate link itself reported ZERO unresolved.
    """
    import hashlib
    h = hashlib.sha1()
    for p in libs:
        try:
            h.update(f"{p}:{p.stat().st_size}\n".encode())
        except OSError:
            h.update(f"{p}:?\n".encode())
    return h.hexdigest()


def lib_symbols() -> set[str]:
    """Every symbol the toolchain .LIB archives can supply, read in-process
    from each archive's first linker member (the linker's own answer to 'can
    this resolve').

    Cached, but keyed on the archive set (see _lib_stamp) so a toolchain bump
    re-scans instead of answering for the previous one. An EMPTY scan is never
    cached and never replaces a populated cache: outside the dev shell
    $MSVC_DIR is unset, and an empty answer poisons every consumer.
    """
    libs = _toolchain_libs()
    stamp = _lib_stamp(libs)
    cached: set[str] = set()
    if LIB_CACHE.is_file():
        lines = LIB_CACHE.read_text(errors="ignore").split("\n")
        head = lines[0] if lines else ""
        cached = {s for s in lines if s and not s.startswith("#")}
        if cached and head == _CACHE_STAMP + stamp:
            return cached
    syms: set[str] = set()
    for lib in libs:
        try:
            b = lib.read_bytes()
        except OSError:
            continue
        if not b.startswith(b"!<arch>\n"):
            continue
        # first linker member: big-endian count, offsets, then NUL-joined names
        off = 8
        if b[off:off + 1] != b"/":
            continue
        size = int(b[off + 48:off + 58].split()[0])
        body = b[off + 60:off + 60 + size]
        if len(body) < 4:
            continue
        n = struct.unpack_from(">I", body, 0)[0]
        names = body[4 + 4 * n:].split(b"\0")
        syms.update(nm.decode("latin-1") for nm in names[:n] if nm)
    if not syms:
        return cached          # no toolchain reachable: keep the last answer
    LIB_CACHE.parent.mkdir(parents=True, exist_ok=True)
    LIB_CACHE.write_text(_CACHE_STAMP + stamp + "\n" + "\n".join(sorted(syms)))
    return syms


def _class_of_method(sym: str) -> str | None:
    """?Method@CClass@@<member-code>... -> CClass (methods only, not data)."""
    m = re.match(r"\?[^@]+((?:@[A-Za-z_]\w*)+)@@([A-Za-z])", sym)
    return m.group(1).split("@")[1] if m else None


def _sym_class(sym: str) -> str | None:
    m = re.match(r"\?\?(?:_?[0-9A-Z])([A-Za-z_]\w*)?@@", sym)
    if m:
        return m.group(1)
    m = re.match(r"\?[^@]+((?:@[A-Za-z_]\w*)+)@@", sym)
    if m:
        return m.group(1).split("@")[1]
    return None


def _is_library(sym: str, libs: set[str]) -> bool:
    if not sym.startswith("?"):
        return True                      # C symbol / __imp_ - the link audit's turf
    if sym in libs or sym.lstrip("_") in libs:
        return True
    c = _sym_class(sym)
    if c is not None:
        return c in LIBRARY_CLASSES or c in LIBRARY_EXTRA or c.startswith("std")
    if sym.startswith("??"):
        return True                      # class-less global operator
    return bool(re.match(r"\?_?Afx", sym))


def _rtti_classes() -> set[str]:
    """Classes with an admitted vtable row (the Model's data_vtables names)."""
    from homm1.model import resolve
    out = set()
    for b in resolve().data:
        if b.kind == "vtable" and b.name:
            m = re.search(r"\?\?_7([A-Za-z_]\w*)@", b.name)
            if m:
                out.add(m.group(1))
    return out


CLASS_DEF_RE = re.compile(r"\b(?:class|struct)\s+([A-Za-z_]\w*)\s*(?::[^;{}]*)?\{",
                          re.S)


def source_library_shadows() -> list[tuple[str, str]]:
    """Project definitions that replace an MFC/CRT class (the allowlist's
    blind spot, closed at the source-definition boundary)."""
    out = []
    for root in (REPO / "include", REPO / "src"):
        for path in root.rglob("*"):
            if path.suffix.lower() not in {".h", ".hpp", ".c", ".cpp"}:
                continue
            text = re.sub(r"/\*.*?\*/|//[^\n]*", "",
                          path.read_text(encoding="utf-8", errors="replace"),
                          flags=re.S)
            for name in CLASS_DEF_RE.findall(text):
                if name in LIBRARY_CLASSES:
                    out.append((path.relative_to(REPO).as_posix(), name))
    return sorted(out)


def reviewed_incomplete_types() -> set[str]:
    """Types explicitly admitted as ABI-only views with cited evidence."""
    if not INCOMPLETE_TYPES.is_file():
        return set()
    import tomllib
    doc = tomllib.loads(INCOMPLETE_TYPES.read_text(encoding="utf-8"))
    return {row["name"] for row in doc.get("incomplete_type", [])
            if row.get("name") and row.get("evidence")}


def analyse():
    """(phantom {class: [syms]}, shadows, declared_only set)."""
    bdef, bund = _sym_sets(live_base_objs())
    tdef, tund = _sym_sets(sorted(TARGET.glob("*.obj")))
    never = bund - bdef
    libs = lib_symbols()
    rtti = _rtti_classes()
    incomplete = reviewed_incomplete_types()
    defined_cls = {c for c in (_class_of_method(s) for s in bdef
                               if s.startswith("?")) if c}

    phantom: dict[str, list] = defaultdict(list)
    for s in never:
        if not s.startswith("?"):
            continue
        c = _class_of_method(s)
        if not c or c in LIBRARY_CLASSES or c in incomplete or c.startswith("std"):
            continue
        if c in defined_cls or c in rtti:
            continue
        phantom[c].append(s)

    alias = never - tdef - tund
    declared = {s for s in alias if not _is_library(s, libs)
                and _sym_class(s) not in incomplete}
    return phantom, source_library_shadows(), declared


def _read_baseline() -> set[str]:
    if not BASELINE.is_file():
        return set()
    return {ln.split("\t")[0].strip() for ln in BASELINE.read_text().splitlines()
            if ln.strip() and not ln.startswith("#")}


def _write_baseline(syms) -> None:
    rows = "\n".join(sorted(syms))
    BASELINE.write_text(
        "# declared-only debt (homm1.verify.undefined_closure): symbols some "
        "base obj\n# references that NO base obj defines and retail's "
        "namespace never names -\n# fabricated aliases / phantom externs. "
        "RATCHET: new rows are FATAL; fix rows,\n# then --update. Drive to 0.\n"
        + rows + ("\n" if rows else ""))


def gate_findings() -> list[str]:
    if not BASE.is_dir() or not any(BASE.glob("*.obj")):
        return ["undefined-closure: no base objs - run `homm1 build` first "
                "(never vacuous)"]
    phantom, shadows, declared = analyse()
    out = []
    for c in sorted(phantom, key=lambda c: -len(phantom[c])):
        out.append(f"pure-phantom class {c} ({len(phantom[c])} declared-only "
                   f"method(s), no body, no RTTI): "
                   + " ".join(sorted(phantom[c])[:4]))
    for path, name in shadows:
        out.append(f"library-class shadow: {path} defines {name}")
    base = _read_baseline()
    for s in sorted(declared - base):
        out.append(f"NEW declared-only alias: {s} (defined nowhere, unknown "
                   f"to retail, resolvable from no .LIB)")
    return out


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 verify undefined-closure",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--list", action="store_true",
                    help="list every phantom class and declared-only symbol")
    ap.add_argument("--update", action="store_true",
                    help="MANUAL bless: rewrite the declared-only baseline")
    a = ap.parse_args(argv)
    if not BASE.is_dir() or not any(BASE.glob("*.obj")):
        print("undefined-closure: no base objs - run `homm1 build` first",
              file=sys.stderr)
        return 1
    phantom, shadows, declared = analyse()
    base = _read_baseline()
    new = declared - base
    fixed = base - declared
    print(f"undefined-closure: {len(phantom)} pure-phantom class(es), "
          f"{sum(len(v) for v in phantom.values())} phantom method(s); "
          f"{len(shadows)} library shadow(s); declared-only {len(declared)} "
          f"(baseline {len(base)}, fixed {len(fixed)}, NEW {len(new)})")
    if a.update:
        _write_baseline(declared)
        print(f"undefined-closure: baseline rewritten ({len(declared)} row(s))")
        return 0
    if a.list:
        for c in sorted(phantom):
            print(f"  {c}:")
            for s in sorted(phantom[c]):
                print(f"      {s}")
        for s in sorted(declared):
            print(("  NEW " if s in new else "      ") + s)
    bad = gate_findings()
    if bad:
        for b in bad:
            print("  " + b, file=sys.stderr)
        return 1
    if fixed:
        print(f"undefined-closure: {len(fixed)} baselined row(s) now resolved "
              f"- ratchet down with --update")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
