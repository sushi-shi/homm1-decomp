"""homm1.sema - the semantic-navigation surface over the retail image.

    homm1 sema rva     <addr>            address dossier: the winning binding,
                                          its aliases, channel, extent, match%
    homm1 sema disasm  <rva|name>        annotated retail i386 assembly
    homm1 sema dump    <rva|name>        raw bytes + relocation targets + asm
    homm1 sema xref    <rva|name>        callers, callees and referent sites
    homm1 sema strings [<rva>|--find s]  string literals a function reaches
    homm1 sema vtable  <rva>             a vtable's slots / who holds a fn
    homm1 sema class   <Class|fn>        a class's vtables, slot by slot
    homm1 sema gaps    [options]         unclaimed same-file .text gaps
    homm1 sema map     [sub ...]         retail address-space map
    homm1 sema match   <unit|rva|name>   objdiff scores for a unit / function
    homm1 sema frame   <unit|function>   VC4 candidate local names and /Od slots

Every module is also a direct entry: `python3 -m homm1.sema.xref 0x136180`.
`homm1 sema -` is batch mode: newline-delimited view commands on stdin,
answered against ONE loaded Model and image (a 40-query investigation pays one
parse instead of forty).

sema is a READ-ONLY consumer with four inputs and no policy of its own:
the Model (`homm1.model.resolve`) for identity, the retail image
(`homm1.core.pe`) for bytes, the compare slice's current report
(`build/objdiff/compare-new/report.json`, falling back to the older
`build/objdiff/report.json`) for scores and `config/units.toml` for the unit
list. Usage events are appended to build/homm1_usage.jsonl; queries do not
change reconstruction inputs.

Doctrine: assembly only. Nothing here decompiles - views annotate real
instruction bytes with Model labels, and a question the labels cannot answer
is reported as unanswered rather than guessed.

rc convention: 0 answered, 1 answered-NO (valid query, no hit / differs),
2 error (bad input, missing prerequisite).
"""

from __future__ import annotations

import sys

SUBCOMMANDS = {
    "rva": "homm1.sema.rva",
    "disasm": "homm1.sema.disasm",
    "dump": "homm1.sema.dump_target",
    "dump_target": "homm1.sema.dump_target",
    "xref": "homm1.sema.xref",
    "strings": "homm1.sema.strings",
    "vtable": "homm1.sema.vtable",
    "class": "homm1.sema.classof",
    "classof": "homm1.sema.classof",
    "gaps": "homm1.sema.gaps",
    "map": "homm1.sema.map",
    "match": "homm1.sema.match",
    "frame": "homm1.sema.frame",
}


class SemaError(Exception):
    """A real error (bad input, missing prerequisite) - rc 2."""


def die(msg: str):
    """Raise the rc-2 error. Answered-NO paths return 1 instead."""
    raise SemaError(msg)


def parse_rva(text: str) -> int:
    """A hex address, with or without the 0x."""
    try:
        return int(text, 16)
    except ValueError:
        die(f"'{text}' is not a hex RVA (e.g. 0x00153810)")


def resolve_target(token: str) -> list[int]:
    """Candidate rvas for a hex address OR a name spelling. A token that is
    neither is one error, not two: reporting only 'not a hex RVA' for an
    obvious name hides the question the caller actually asked."""
    from homm1.sema.index import index
    hits = index().resolve_name(token)
    if not hits:
        die(f"'{token}' is not a hex RVA and no binding carries that name")
    return hits


def _drop_stdout() -> None:
    """Point stdout at /dev/null so the interpreter's exit-time flush cannot
    raise a second BrokenPipeError (which python reports as rc 120 plus an
    'Exception ignored' line - `homm1 sema ... | head` used to end that way)."""
    import contextlib
    import os
    with contextlib.suppress(OSError, ValueError):
        os.dup2(os.open(os.devnull, os.O_WRONLY), sys.stdout.fileno())


def run(module: str, argv: list[str]) -> int:
    """Run one subcommand module's main(), mapping SemaError to rc 2."""
    import importlib
    try:
        rc = importlib.import_module(module).main(argv)
        sys.stdout.flush()          # surface a closed pipe HERE, not at exit
        return rc
    except SemaError as e:
        print(f"[sema] ERROR: {e}", file=sys.stderr)
        return 2
    except BrokenPipeError:
        _drop_stdout()              # the reader went away: a normal end
        return 0


def batch() -> int:
    """Answer newline-delimited view commands from stdin in one process."""
    import shlex
    rc = 0
    try:
        for line in sys.stdin:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            print(f"== homm1 sema {line}")
            rc = main(shlex.split(line)) or rc
            sys.stdout.flush()
    except BrokenPipeError:
        _drop_stdout()
        return 0
    return rc


from homm1.core.usage import logged


@logged
def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if not argv or argv[0] in ("-h", "--help", "help"):
        print(__doc__.strip())
        return 0 if argv else 2
    if argv[0] == "-":
        return batch()
    sub, rest = argv[0], argv[1:]
    if sub not in SUBCOMMANDS:
        print(f"homm1 sema: unknown view {sub!r} (have: "
              f"{', '.join(sorted(set(SUBCOMMANDS)))})", file=sys.stderr)
        return 2
    return run(SUBCOMMANDS[sub], rest)
