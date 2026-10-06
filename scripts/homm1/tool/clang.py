"""homm1.tool.clang - the native front-end (extraction's tool).

Three probes over one TU, all under the MSVC-compat flag set:
    emit_ir()   textual LLVM IR - @llvm.global.annotations pairs each VA()
                annotation DIRECTLY with the function's mangled symbol
    ast_dump()  JSON AST - VarDecls for the DATA() join
    var_facts() pylibclang - exact byte extents and storage of main-file globals

Per-TU flags come from the clangd compdb (`/imsvc` lowercase-mirror include
dirs that make header lookup work on case-sensitive Linux), falling back to
the bare MS flag set. clang's mangled name is a PROPOSAL in one narrow sense
only: VC4's own spelling is a deterministic rewrite of it, applied by
core/msvc_names.
"""

from __future__ import annotations

import json
import os
import subprocess
import tempfile
from pathlib import Path

from homm1.core.paths import BUILD, IMAGE_BUILD, INCLUDE, REPO, vendor_include_dirs

COMPDB = IMAGE_BUILD / "clangd/compile_commands.json"

TARGET = "i686-pc-windows-msvc"
MSC_COMPAT = "1000"
# Two era-MSVC constructs are hard errors in clang: `&Temporary()`
# (MSVC C4238) and a signed switch whose SDK case macro is an unsigned
# 0x80000000-range `long`. Demote both so the probes read the same dialect.
MS_WARN = ["-Wno-address-of-temporary", "-Wno-c++11-narrowing"]
MS_FLAGS = [f"--target={TARGET}", f"-fms-compatibility-version={MSC_COMPAT}",
            "-fms-extensions", *MS_WARN]


def _include_dirs() -> list[str]:
    dirs = [str(INCLUDE)]
    dirs += [str(d) for _name, d in vendor_include_dirs()]
    return dirs


def inc_cl() -> list[str]:
    return [f"/I{d}" for d in _include_dirs()]


def inc_gcc() -> list[str]:
    return [f"-I{d}" for d in _include_dirs()]


def compdb(path: Path = COMPDB) -> dict[str, list[str]]:
    """{realpath(source): [clang-cl flags]} - driver, `/c` and the TU dropped
    (each probe re-adds its own driver mode, action, and source)."""
    try:
        db = json.loads(Path(path).read_text())
    except (OSError, json.JSONDecodeError):
        return {}
    out = {}
    for entry in db:
        args = entry.get("arguments") or []
        flags = [a for a in args[1:]
                 if a not in ("/c", "-c") and a != entry.get("file")
                 and not a.startswith("/Fo")]
        src = os.path.realpath(os.path.join(entry.get("directory", "."),
                                            entry["file"]))
        out[src] = flags
    return out


def localization_args(source: str) -> list[str]:
    from homm1.graph.localization import clang_args
    return clang_args(REPO, source)


def _clang() -> str:
    return os.environ.get("HOMM1_CLANG") or "clang"


def emit_ir(tu: str, cl_flags: list[str] | None) -> str | None:
    """Textual LLVM IR, or None (an error the caller must surface - a TU that
    compiles under cl but yields no IR would silently drop every label)."""
    if cl_flags is not None:
        # clang-cl rejects `-S -o -`; -emit-llvm writes only to a real file.
        # Retried once: under parallel extraction the temp .ll can vanish.
        for _attempt in range(2):
            with tempfile.NamedTemporaryFile(suffix=".ll", delete=False) as tf:
                ll = tf.name
            try:
                cmd = [_clang(), "--driver-mode=cl", "/c", "/DHOMM1_EMIT_META",
                       *cl_flags, *MS_WARN, *inc_cl(), *localization_args(tu),
                       "-Xclang", "-emit-llvm", "-o", ll, tu]
                res = subprocess.run(cmd, capture_output=True, text=True)
                ir = Path(ll).read_text() \
                    if os.path.exists(ll) and os.path.getsize(ll) else ""
            finally:
                try:
                    os.unlink(ll)
                except OSError:
                    pass
            if ir:
                return ir
        return None  # caller surfaces this; res.stderr is intentionally short-lived
    cmd = [_clang(), "-DHOMM1_EMIT_META", *MS_FLAGS, *inc_gcc(), *localization_args(tu),
           "-S", "-emit-llvm", "-o", "-", tu]
    res = subprocess.run(cmd, capture_output=True, text=True)
    return res.stdout or None


def ast_dump(tu: str, cl_flags: list[str] | None) -> dict | None:
    if cl_flags is not None:
        cmd = [_clang(), "--driver-mode=cl", "/DHOMM1_EMIT_META", *cl_flags,
               *inc_cl(), *localization_args(tu), tu, "-fsyntax-only", "-Xclang", "-ast-dump=json"]
    else:
        cmd = [_clang(), "-DHOMM1_EMIT_META", *MS_FLAGS, *inc_gcc(), *localization_args(tu), tu,
               "-fsyntax-only", "-Xclang", "-ast-dump=json"]
    res = subprocess.run(cmd, capture_output=True, text=True)
    try:
        return json.loads(res.stdout)
    except json.JSONDecodeError:
        return None


def var_facts(tu: str, cl_flags: list[str] | None) -> dict[str, dict] | None:
    """{mangled VarDecl name: {'size': bytes, 'internal': bool}} for main-file
    globals - THE DATA-extent authority (laid out under the TU's real
    i386/MSVC flags) and the storage a claim's cl 5.0 spelling depends on.

    The key is libclang's mangled name, which already carries the i386 COFF
    global prefix. `internal` is true for anything cl gives TU-local storage:
    a file static, a namespace-scope `const`, and a function-local static (no
    linkage at all) alike. None when pylibclang could not parse cleanly;
    incomplete types (negative get_size) and cross-decl size conflicts are
    omitted."""
    try:
        import clang.cindex as cidx
    except ImportError:
        return None
    args = (["--driver-mode=cl", "/DHOMM1_EMIT_META", *cl_flags, *inc_cl(), *localization_args(tu)]
            if cl_flags is not None
            else ["-DHOMM1_EMIT_META", *MS_FLAGS, *inc_gcc(), *localization_args(tu)])
    try:
        parsed = cidx.Index.create().parse(tu, args=args)
    except cidx.LibclangError:
        return None
    if any(d.severity >= cidx.Diagnostic.Error for d in parsed.diagnostics):
        return None
    main_real = os.path.realpath(tu)
    facts: dict[str, dict] = {}
    conflicts = set()
    for cursor in parsed.cursor.walk_preorder():
        if cursor.kind != cidx.CursorKind.VAR_DECL or cursor.location.file is None:
            continue
        if os.path.realpath(cursor.location.file.name) != main_real:
            continue
        name, size = cursor.mangled_name, cursor.type.get_size()
        if not name or size < 0:
            continue
        internal = cursor.linkage != cidx.LinkageKind.EXTERNAL
        if name in facts and facts[name] != {"size": size, "internal": internal}:
            conflicts.add(name)
        else:
            facts[name] = {"size": size, "internal": internal}
    for name in conflicts:
        facts.pop(name, None)
    return facts


def annotated_decls(tu: str, cl_flags: list[str] | None) -> list[dict] | None:
    """Every annotated function/variable declaration the TU sees in a repo
    file (the TU itself or a project header), one record per redeclaration:
    {'kind': 'func'|'var', 'name': libclang's mangled name, 'annotations':
    [str], 'defined': bool, 'file': realpath, 'internal': bool}.

    A clang annotation on a declaration never reaches IR, so the declaration
    channel (`RVA_DECL`) reads it here. Only namespace/record/linkage scopes
    are descended (never a function body), and a cursor outside the repo
    (SDK, CRT) is skipped before its children are read. None when pylibclang
    could not parse cleanly."""
    try:
        import clang.cindex as cidx
    except ImportError:
        return None
    args = (["--driver-mode=cl", "/DHOMM1_EMIT_META", *cl_flags, *inc_cl(), *localization_args(tu)]
            if cl_flags is not None
            else ["-DHOMM1_EMIT_META", *MS_FLAGS, *inc_gcc(), *localization_args(tu)])
    try:
        parsed = cidx.Index.create().parse(tu, args=args)
    except cidx.LibclangError:
        return None
    if any(d.severity >= cidx.Diagnostic.Error for d in parsed.diagnostics):
        return None
    K = cidx.CursorKind
    scopes = {K.NAMESPACE, K.CLASS_DECL, K.STRUCT_DECL, K.UNION_DECL,
              K.LINKAGE_SPEC, K.UNEXPOSED_DECL}
    funcs = {K.FUNCTION_DECL, K.CXX_METHOD, K.CONSTRUCTOR, K.DESTRUCTOR}
    repo = os.path.realpath(REPO) + os.sep
    real: dict[str, str] = {}
    out: list[dict] = []

    def in_repo(cursor) -> str | None:
        f = cursor.location.file
        if f is None:
            return None
        path = real.get(f.name)
        if path is None:
            path = real[f.name] = os.path.realpath(f.name)
        return path if path.startswith(repo) else None

    def visit(parent):
        for cursor in parent.get_children():
            if cursor.kind in scopes:
                visit(cursor)
                continue
            if cursor.kind not in funcs and cursor.kind != K.VAR_DECL:
                continue
            path = in_repo(cursor)
            if path is None:
                continue
            anns = [c.spelling for c in cursor.get_children()
                    if c.kind == K.ANNOTATE_ATTR]
            if not anns:
                continue
            out.append({"kind": "func" if cursor.kind in funcs else "var",
                        "name": cursor.mangled_name,
                        "annotations": anns,
                        "defined": cursor.is_definition(),
                        "file": path,
                        "internal": cursor.linkage != cidx.LinkageKind.EXTERNAL})

    visit(parsed.cursor)
    return out
