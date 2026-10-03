"""homm1.verify.line_directives - `#line N` must govern the line that uses it.

Retail assert line numbers are reproduced with `#line N` directly above the
statement that expands `__LINE__` (see the line-directives memory and
docs/patterns). A formatter that splits such a statement over several lines
silently moves `__LINE__` to N+k: the object still compiles, only the
immediate pushed to ProcessAssert changes (TOWNMGR BuyBuild and PATH
GetAdjacentCellIndex shipped 1528/325 instead of 1525/322 that way).

The rule: the statement that follows a `#line N` (the next code line up to
the line that ends it with `;`, `{` or `}`) either keeps every `__LINE__` on
its FIRST line, or uses no `__LINE__` at all. Wrap a long call in
`// clang-format off` / `// clang-format on` instead of letting it wrap.

    homm1 verify line-directives
"""

from __future__ import annotations

import re

from homm1.core.usage import logged
from homm1.verify.srcscan import blank_comments, rel, source_files

_LINE = re.compile(r"^\s*#\s*line\s+\d+")
_MAX_STATEMENT = 40


def _statement(lines: list[str], start: int) -> tuple[int, int] | None:
    """(first, last) line indices of the statement after line `start`."""
    i = start + 1
    while i < len(lines) and not lines[i].strip():
        i += 1
    if i >= len(lines) or lines[i].lstrip().startswith("#"):
        return None
    j = i
    while j < len(lines) and j - i < _MAX_STATEMENT:
        if lines[j].rstrip().endswith((";", "{", "}")):
            break
        j += 1
    return i, min(j, len(lines) - 1)


def findings_for(text: str, name: str) -> list[str]:
    lines = blank_comments(text).splitlines()
    out = []
    for n, line in enumerate(lines):
        if not _LINE.match(line):
            continue
        span = _statement(lines, n)
        if span is None:
            continue
        first, last = span
        for k in range(first + 1, last + 1):
            if "__LINE__" in lines[k]:
                out.append(f"{name}:{k + 1}: __LINE__ is {k - first} line(s) below the "
                           f"statement governed by `{line.strip()}` (line {n + 1}) - keep "
                           f"the call on one line (clang-format off/on)")
                break
    return out


def gate_findings() -> list[str]:
    out = []
    for path in source_files((".cpp", ".h")):
        out += findings_for(path.read_text(encoding="utf-8", errors="replace"), rel(path))
    return out


@logged
def main(argv=None) -> int:
    import argparse
    argparse.ArgumentParser(prog="homm1 verify line-directives",
                            description=__doc__,
                            formatter_class=argparse.RawDescriptionHelpFormatter
                            ).parse_args(argv)
    findings = gate_findings()
    for f in findings:
        print(f"[line-directives] {f}")
    print(f"[line-directives] {len(findings)} finding(s)")
    return 1 if findings else 0


if __name__ == "__main__":
    raise SystemExit(main())
