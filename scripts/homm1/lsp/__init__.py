"""homm1.lsp - clangd-backed source navigation and rename (USR-exact).

    homm1 lsp refs   <Sym | file:line[:col]>   # every reference
    homm1 lsp hover  <Sym | file:line[:col]>   # type/doc at point
    homm1 lsp rename <Class::m_old> <m_new> [old=new ...]
    homm1 lsp rename <Class> --map FILE        # bulk member map
    homm1 lsp index                            # build the index + wait

Every verb is also a direct entry: `python3 -m homm1.lsp refs CGrunt`.

A <Sym> is resolved through clangd's workspace-symbol index (`CGrunt` or
`CGrunt::GetAI`); a `file:line[:col]` point is probed directly - without a
column, identifiers on the line are tried right-to-left (the declared name
sits rightmost on C++ declaration lines). Everything is keyed on the symbol's
USR, so a same-named member of a different class is never touched - the
reason these verbs exist instead of grep.

Engine: homm1.tool.clangd (the only layer that spawns the server). Rename
policy - class-body location, WorkspaceEdit verification and application -
lives in homm1.lsp.rename; refs/hover in homm1.lsp.query.

clangd is a READER of this MSVC5 dialect: navigation is reliable, its
diagnostics are NOT build truth - the wine `cl` build and objdiff are.
"""

from __future__ import annotations

from homm1.core.usage import logged

import sys


@logged
def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="homm1 lsp", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="verb", required=True)

    for name, help_ in (("refs", "every reference of the symbol"),
                        ("hover", "type/doc of the symbol")):
        p = sub.add_parser(name, help=help_)
        p.add_argument("target", help="symbol name or file:line[:col]")

    r = sub.add_parser("rename", help="type-aware member rename, tree-wide")
    r.add_argument("target", help="Class::m_old (or bare Class with --map)")
    r.add_argument("rest", nargs="*",
                   help="m_new [old=new ...]  (pairs reuse the class)")
    r.add_argument("--map", dest="map_file",
                   help="mapping file: `old=new` or `old new` per line")
    r.add_argument("--header",
                   help="header defining the class (default: auto under include/)")
    r.add_argument("--dry-run", action="store_true",
                   help="print the edit summary, write nothing")
    r.add_argument("--audit", action="store_true",
                   help="clang-query census of residual OLD-name member accesses")

    sub.add_parser("index", help="build the background index and wait")

    args = ap.parse_args(sys.argv[1:] if argv is None else argv)
    if args.verb in ("refs", "hover"):
        from homm1.lsp.query import run_point_or_symbol
        return run_point_or_symbol(args.verb, args.target)
    if args.verb == "rename":
        from homm1.lsp.rename import run_rename
        return run_rename(args)
    from homm1.lsp.query import run_index
    return run_index()
