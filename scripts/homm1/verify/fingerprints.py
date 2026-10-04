"""homm1.verify.fingerprints - per-function source fingerprints (src_hash).

The ported mechanism: a function's src_hash is the 12-hex sha1 of ITS OWN
source extent, recovered by asking clangd for the unit's hierarchical
documentSymbol tree. Repeated qualified names are joined to exact mangled
AST definitions using the existing source-label spelling/location rules.
Their hashes carry ``overload1:``: the legacy qualified-name union cannot
prove whether an individual overload changed during migration.
Hashing a whole .cpp is too coarse - editing one function would reset
the high-water of every sibling in the unit, hiding collateral regressions.

Bridge from the report's MANGLED names to clangd's source-level names:
  * C++ (?...):  llvm-undname -> "Class::Method" -> clangd qualified symbol.
  * C   (_name): strip the cdecl/stdcall decoration (leading `_`, trailing
                 `@N`) -> clangd source name.
A function clangd cannot resolve gets NO entry, and the fingerprinter falls
back to the unit's whole-.cpp hash, tagged `cpp:` = "no per-function
fingerprint available". A fallback is UNKNOWN and must never count as an
edit (else a stale cache hides regressions and corrupts the baseline).

The cache is DERIVED, incremental (only units whose .cpp hash moved get
re-parsed), and lives in build/gen/ (this slice's scratch). The old
pipeline's cache at build/clangd/func_fingerprints.tsv is read as a SEED
when the new one is absent, only when its cache version matches. Legacy
union caches are invalidated and rebuilt; the score ledger is never rewritten
by cache regeneration.
"""

from __future__ import annotations

import hashlib
import re
import subprocess
from pathlib import Path

from homm1.core.paths import BUILD, REPO

CACHE = BUILD / "gen/func_fingerprints.tsv"
SEED = BUILD / "clangd/func_fingerprints.tsv"   # old pipeline's cache (read-only)
CDB_DIR = BUILD / "clangd"                       # compile_commands.json home

FALLBACK = "cpp:"  # marks a fingerprint we could NOT resolve per-function
OVERLOAD = "overload1:"  # exact-symbol body; legacy overload hashes were unions
CACHE_VERSION = "2"


def is_fallback(h: str) -> bool:
    return h.startswith(FALLBACK)


def real_edit(prev_fp: str, cur_fp: str) -> bool:
    """True only when BOTH sides are real (non-fallback) fingerprints that
    differ - a genuine source edit, not a cache hash-domain change."""
    # A legacy union cannot establish which overload changed. Its transition
    # to an individual body is UNKNOWN, just like other hash-domain changes.
    return not is_fallback(prev_fp) and not is_fallback(cur_fp) \
        and prev_fp.startswith(OVERLOAD) == cur_fp.startswith(OVERLOAD) \
        and prev_fp != cur_fp


def cpp_hash(source: str) -> str:
    """12-hex sha1 of a unit's whole source file (the unit-level fallback)."""
    p = REPO / source
    return _sha12(p.read_text(encoding="utf-8")) if p.is_file() \
        else "nosrc"


def _sha12(text: str) -> str:
    from homm1.graph.catalog import Catalog
    if (REPO / "locales/messages.def").is_file():
        from homm1.graph.localization import matching_locale
        text = Catalog.load(REPO).render(text, expanded=True, locale=matching_locale(REPO))
    return hashlib.sha1(text.encode("utf-8", "replace")).hexdigest()[:12]


def unit_sources() -> dict[str, str]:
    from homm1 import manifest
    return {u["unit"].rsplit("/", 1)[-1]: u.get("source", "")
            for u in manifest.units()}


def unit_mangled() -> dict[str, set]:
    """unit -> {mangled names}, from the Model (the one rva/unit/name join)."""
    from homm1.model import resolve
    out: dict[str, set] = {}
    for b in resolve().functions:
        if b.name and b.unit:
            out.setdefault(b.unit.rsplit("/", 1)[-1], set()).add(b.name)
    return out


# --------------------------------------------------------------------------- #
# cache I/O                                                                   #
# --------------------------------------------------------------------------- #
def load_cache() -> tuple[dict[str, dict], dict[tuple[str, str], str]]:
    """(units{unit: {cpp_hash, source}}, funcs{(unit, mangled): src_hash})."""
    units: dict[str, dict] = {}
    funcs: dict[tuple[str, str], str] = {}
    path = CACHE if CACHE.is_file() else SEED
    if not path.is_file():
        return units, funcs
    text = path.read_text()
    if f"# fingerprint-version: {CACHE_VERSION}" not in text.splitlines():
        return units, funcs  # invalidate donor union caches, including the seed
    section = None
    for line in text.splitlines():
        if line.startswith("# [units]"):
            section = "u"
            continue
        if line.startswith("# [functions]"):
            section = "f"
            continue
        if not line or line.startswith("#"):
            continue
        c = line.split("\t")
        if section == "u" and len(c) >= 3:
            units[c[0]] = {"cpp_hash": c[1], "source": c[2]}
        elif section == "f" and len(c) >= 3:
            funcs[(c[0], c[1])] = c[2]
    return units, funcs


def write_cache(units: dict[str, dict],
                funcs: dict[tuple[str, str], str]) -> None:
    CACHE.parent.mkdir(parents=True, exist_ok=True)
    out = ["# func source fingerprints - generated by homm1.verify."
           "fingerprints.\n",
           "# Derived cache (gitignored). Consumed by homm1.verify.\n",
           f"# fingerprint-version: {CACHE_VERSION}\n",
           "# [units]\tunit\tcpp_hash\tsource\n"]
    for u in sorted(units):
        out.append(f"{u}\t{units[u]['cpp_hash']}\t{units[u]['source']}\n")
    out.append("# [functions]\tunit\tmangled\tsrc_hash\n")
    for k in sorted(funcs):
        out.append(f"{k[0]}\t{k[1]}\t{funcs[k]}\n")
    CACHE.write_text("".join(out))


# --------------------------------------------------------------------------- #
# the fingerprinter (what bank/check consume)                                 #
# --------------------------------------------------------------------------- #
def fingerprinter():
    """Return (fp, cpp_of, stale): fp(unit, mangled) -> CURRENT fingerprint.

    The cache's per-function range hash when the cache is fresh for the unit
    AND has the function; otherwise a FALLBACK-tagged whole-.cpp hash. `stale`
    collects units whose cache is stale/absent (the dangerous fallback).
    """
    sources = unit_sources()
    cache_units, cache_funcs = load_cache()
    cur_cpp: dict[str, str] = {}

    def cpp_of(unit: str) -> str:
        unit = unit.rsplit("/", 1)[-1]
        if unit not in cur_cpp:
            cur_cpp[unit] = cpp_hash(sources.get(unit, ""))
        return cur_cpp[unit]

    stale: set = set()

    def fp(unit: str, mangled: str) -> str:
        unit = unit.rsplit("/", 1)[-1]
        h = cpp_of(unit)
        cu = cache_units.get(unit)
        if cu and cu.get("cpp_hash") == h:
            real = cache_funcs.get((unit, mangled))
            if real is not None:
                return real                     # real per-function range hash
            return FALLBACK + h                 # fresh unit, fn unmappable
        stale.add(unit)                         # unit's cache stale/absent
        return FALLBACK + h

    return fp, cpp_of, stale


# --------------------------------------------------------------------------- #
# mangled -> qualified source name bridge                                     #
# --------------------------------------------------------------------------- #
def demangle_map(names: set) -> dict[str, str]:
    """C++ mangled -> 'Class::Method' qualified key, one batched llvm-undname."""
    cpp = sorted(n for n in names if n.startswith("?"))
    if not cpp:
        return {}
    # NOTE the trailing newline: without it llvm-undname drops the final name.
    proc = subprocess.run(["llvm-undname"], input="\n".join(cpp) + "\n",
                          capture_output=True, text=True)
    out: dict[str, str] = {}
    for block in proc.stdout.split("\n\n"):
        lines = block.splitlines()
        if len(lines) >= 2 and lines[0] in names:
            q = _qualified_of(lines[1])
            if q:
                out[lines[0]] = q
    return out


def _qualified_of(demangled: str) -> str | None:
    """'public: int __thiscall CFileIO::Open(char const *,...)' -> 'CFileIO::Open'."""
    # A function-pointer return type wraps the declared name in parentheses:
    #   void (__cdecl * __thiscall C::Get(...))(char *, int)
    # The first `(` therefore belongs to the return type, not the parameter
    # list.  Prefer the qualified identifier which is itself followed by `(`.
    wrapped = re.search(
        r"(?<![\w:])((?:~?[A-Za-z_]\w*::)+~?[A-Za-z_]\w*)\s*\(",
        demangled)
    if wrapped:
        return wrapped.group(1)
    head = demangled.split("(", 1)[0].strip()    # drop the parameter list
    if not head:
        return None
    tok = head.split()[-1]                       # name follows retty/callconv
    if "`" in tok or "'" in tok:                 # compiler-generated -> no source
        return None
    return tok


def _candidates(mangled: str) -> list:
    """Source-name candidates for a C symbol: undo the cdecl/stdcall
    decoration (leading `_`, trailing `@N` stdcall arg-byte count)."""
    cands = []
    for n in (mangled, mangled.split("@", 1)[0]):
        cands += [n, n[1:] if n.startswith("_") else n, n.lstrip("_")]
    return list(dict.fromkeys(cands))


# --------------------------------------------------------------------------- #
# minimal clangd LSP client (documentSymbol only)                             #
# --------------------------------------------------------------------------- #
class Clangd:
    def __init__(self):
        import os
        self.proc = subprocess.Popen(
            ["clangd", "--log=error", "--header-insertion=never",
             "--background-index=false",
             f"--compile-commands-dir={CDB_DIR}"],
            cwd=REPO, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL)
        self._id = 0
        self._request("initialize", {
            "processId": os.getpid(), "rootUri": REPO.as_uri(),
            "capabilities": {"textDocument": {"documentSymbol": {
                "hierarchicalDocumentSymbolSupport": True}}}})
        self._notify("initialized", {})

    def _send(self, payload: dict) -> None:
        import json
        body = json.dumps({"jsonrpc": "2.0", **payload}).encode()
        self.proc.stdin.write(
            f"Content-Length: {len(body)}\r\n\r\n".encode() + body)
        self.proc.stdin.flush()

    def _recv(self) -> dict:
        import json
        import re
        headers = b""
        while not headers.endswith(b"\r\n\r\n"):
            b1 = self.proc.stdout.read(1)
            if not b1:
                raise RuntimeError("clangd exited unexpectedly (EOF)")
            headers += b1
        length = int(re.search(rb"Content-Length: (\d+)", headers).group(1))
        return json.loads(self.proc.stdout.read(length))

    def _notify(self, method: str, params: dict) -> None:
        self._send({"method": method, "params": params})

    def _request(self, method: str, params: dict, timeout: float = 120.0):
        import time
        self._id += 1
        self._send({"id": self._id, "method": method, "params": params})
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            msg = self._recv()
            if msg.get("method") == "window/workDoneProgress/create":
                self._send({"id": msg["id"], "result": None})
                continue
            if msg.get("id") == self._id:
                if "error" in msg:
                    raise RuntimeError(
                        f"clangd error: {msg['error'].get('message')}")
                return msg.get("result")
        raise RuntimeError(f"clangd: no reply to {method} within {timeout}s")

    def document_symbols(self, path: Path) -> list:
        self._notify("textDocument/didOpen", {"textDocument": {
            "uri": path.as_uri(), "languageId": "cpp", "version": 1,
            "text": path.read_text(errors="replace")}})
        return self._request("textDocument/documentSymbol", {
            "textDocument": {"uri": path.as_uri()}}) or []

    def close(self) -> None:
        try:
            self._request("shutdown", {}, timeout=5)
            self._notify("exit", {})
        except Exception:  # noqa: BLE001 - best-effort teardown
            pass
        self.proc.terminate()


def body_ranges(symbols: list) -> dict[str, list]:
    """Hierarchical DocumentSymbol[] -> {qualified_name: [(start, end)]}.

    Keep all occurrences: a one-line overload can be a real definition.
    Repeated qualified names require the exact-symbol AST join below.
    """
    acc: dict[str, list] = {}

    def walk(syms, prefix):
        for s in syms:
            k = s.get("kind")
            nm = s.get("name", "")
            if k in (6, 9, 12):  # Method / Constructor / Function
                r = s["range"]
                acc.setdefault(prefix + nm, []).append(
                    (r["start"]["line"], r["end"]["line"]))
            child_prefix = prefix + nm + "::" if k in (3, 5, 11, 23) else prefix
            walk(s.get("children") or [], child_prefix)

    walk(symbols, "")
    return {name: sorted(set(ranges)) for name, ranges in acc.items()}


def exact_body_ranges(ast: dict, main_file: str) -> dict[str, list]:
    """clang definitions -> exact VC4 symbol / main-file line ranges.

    Reuse extraction's AST location and spelling rules. Declarations without
    bodies and inline definitions in headers cannot steal a main-file body.
    """
    import os
    from homm1.core import msvc_names
    from homm1.retail_labels.source import _loc_file

    main_real = os.path.realpath(main_file)
    source = Path(main_file).read_bytes()
    out: dict[str, list] = {}
    in_main = True

    def visit(node):
        nonlocal in_main
        if not isinstance(node, dict):
            return
        rg = node.get("range") or {}
        for loc in (node.get("loc"), rg.get("begin")):
            filename = _loc_file(loc)
            if filename is not None:
                in_main = os.path.realpath(filename) == main_real
        children = node.get("inner") or []
        if node.get("kind") in {
                "FunctionDecl", "CXXMethodDecl", "CXXConstructorDecl",
                "CXXDestructorDecl", "CXXConversionDecl"}:
            if (in_main and node.get("mangledName")
                    and not node.get("isImplicit")
                    and any(c.get("kind") in {"CompoundStmt", "CXXTryStmt"}
                            for c in children)):
                begin = rg.get("begin") or {}
                begin = begin.get("expansionLoc", begin)
                end = rg.get("end") or {}
                end = end.get("expansionLoc", end)
                def line_of(location):
                    if "line" in location:
                        return location["line"] - 1
                    if "offset" in location:
                        return source[:location["offset"]].count(b"\n")
                    return None

                start_line, end_line = line_of(begin), line_of(end)
                if start_line is not None and end_line is not None:
                    name = msvc_names.func(node["mangledName"], decorated=True)
                    out.setdefault(name, []).append(
                        (start_line, end_line))
            return  # body-local references do not change declaration ownership
        for child in children:
            visit(child)

    visit(ast)
    return {name: sorted(set(ranges)) for name, ranges in out.items()}


def function_hashes(lines: list, chosen: dict, names: set, m2q: dict,
                    exact: dict) -> dict[str, str]:
    """One hash per emitted identity; ambiguous/unresolved joins stay unknown."""
    out = {}
    for name in names:
        qualified = m2q.get(name) if name.startswith("?") else next(
            (c for c in _candidates(name) if c in chosen), None)
        ranges = chosen.get(qualified, [])
        if len(ranges) == 1:
            out[name] = _hash_ranges(lines, ranges)
        elif len(ranges) > 1:
            owned = exact.get(name, [])
            if len(owned) == 1:
                out[name] = OVERLOAD + _hash_ranges(lines, owned)
    return out


def _hash_ranges(lines: list, ranges: list) -> str:
    chunks = ["\n".join(lines[a:b + 1]) for (a, b) in ranges]
    return _sha12("\n--\n".join(chunks))


# --------------------------------------------------------------------------- #
# regenerate (incremental)                                                    #
# --------------------------------------------------------------------------- #
def regenerate(force_all: bool = False, verbose: bool = False) -> int:
    sources = unit_sources()
    umang = unit_mangled()
    cache_units, cache_funcs = load_cache()

    cur_cpp: dict[str, str] = {}
    todo: list[str] = []
    for unit, source in sources.items():
        if not source:
            continue
        cur_cpp[unit] = cpp_hash(source)
        if force_all or cache_units.get(unit, {}).get("cpp_hash") != cur_cpp[unit]:
            todo.append(unit)

    new_units: dict[str, dict] = {}
    new_funcs: dict[tuple[str, str], str] = {}
    for unit, source in sources.items():           # carry unchanged units over
        if unit not in cur_cpp or unit in todo:
            continue
        new_units[unit] = {"cpp_hash": cur_cpp[unit], "source": source}
        for k, v in cache_funcs.items():
            if k[0] == unit:
                new_funcs[k] = v

    if todo:
        for unit in todo:
            new_units[unit] = {"cpp_hash": cur_cpp[unit],
                               "source": sources[unit]}
        # Handwritten MASM units have no clangd AST. Keep their unit hash and
        # existing explicit fallback semantics; never invent C++ body hashes.
        cpp_todo = [u for u in todo if Path(sources[u]).suffix.lower()
                    not in (".asm", ".s")]
        allm: set = set()
        for u in cpp_todo:
            allm |= umang.get(u, set())
        if allm:
            m2q = demangle_map(allm)
            lsp = Clangd()
            try:
                for unit in cpp_todo:
                    path = (REPO / sources[unit]).resolve()
                    if not path.is_file():
                        continue
                    chosen = body_ranges(lsp.document_symbols(path))
                    lines = path.read_text(errors="replace").splitlines()
                    exact = {}
                    if any(len(ranges) > 1 for ranges in chosen.values()):
                        from homm1.tool import clang
                        flags = clang.compdb().get(str(path))
                        ast = clang.ast_dump(str(path), flags)
                        if ast is not None:
                            exact = exact_body_ranges(ast, str(path))
                    hashes = function_hashes(
                        lines, chosen, umang.get(unit, set()), m2q, exact)
                    n = 0
                    for m in umang.get(unit, set()):
                        if m in hashes:
                            new_funcs[(unit, m)] = hashes[m]
                            n += 1
                    if verbose:
                        print(f"  {unit}: {n}/{len(umang.get(unit, set()))} "
                              f"fingerprinted ({len(chosen)} defs in clangd)")
            finally:
                lsp.close()

    write_cache(new_units, new_funcs)
    print(f"func fingerprints: {len(new_funcs)} functions  "
          f"({len(todo)} unit(s) re-parsed, {len(new_units) - len(todo)} cached)")
    return 0


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="homm1 verify fingerprints", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--all", action="store_true",
                    help="re-parse every unit (ignore the cache)")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="name each unit as it is re-parsed")
    a = ap.parse_args(argv)
    return regenerate(force_all=a.all, verbose=a.verbose)
