"""homm1.tool.merge_units - git merge driver for config/units.toml.

    sh scripts/merge-units.sh %O %A %B   (python3 here; git merge-file fallback)

Worktrees each append `[[unit]]` blocks at the end of the manifest, so two
branches adding units always collide textually although they never conflict
in substance. This driver merges the manifest as what it is: a header plus a
set of `[[unit]]` blocks keyed by their `unit = "..."` name.

  * the header ([build]/[flags]) and every block merge three-way: a side that
    changed it wins over one that did not; both changing it differently is a
    real conflict (exit 1, %A untouched, git reports the conflict);
  * a block one side deleted and the other left unchanged is deleted;
  * blocks keep %A's order; blocks only %B added follow, in %B's order.

Registered by the dev shell (`merge.units.driver`) and selected for the file
by `.gitattributes`.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

MARK = "[[unit]]\n"
NAME = re.compile(r'^unit\s*=\s*"([^"]+)"', re.M)


def split(text: str) -> tuple[str, list[tuple[str, str]]]:
    """(header, [(unit name, block text)]); each block starts with MARK."""
    head, *rest = text.split("\n" + MARK)
    if text.startswith(MARK):
        head, rest = "", text[len(MARK):].split("\n" + MARK)
    blocks = []
    for body in rest:
        m = NAME.search(body)
        if not m:
            raise ValueError(f"[[unit]] block without a unit name: {body[:60]!r}")
        if any(name == m.group(1) for name, _ in blocks):
            raise ValueError(f"duplicate unit {m.group(1)!r}")
        blocks.append((m.group(1), MARK + body.strip("\n") + "\n"))
    return head.rstrip("\n") + "\n", blocks


def pick(base, ours, theirs):
    """Three-way value merge; None = absent. Raises on a real conflict."""
    if ours == theirs or theirs == base:
        return ours
    if ours == base:
        return theirs
    raise ValueError("both sides changed it differently")


def merge(base: str, ours: str, theirs: str) -> str:
    bh, bb = split(base)
    oh, ob = split(ours)
    th, tb = split(theirs)
    head = pick(bh, oh, th)
    b, o, t = dict(bb), dict(ob), dict(tb)
    order = [n for n, _ in ob] + [n for n, _ in tb if n not in o]
    out = []
    for name in order:
        try:
            block = pick(b.get(name), o.get(name), t.get(name))
        except ValueError as e:
            raise ValueError(f"unit {name!r}: {e}") from None
        if block is not None:
            out.append(block)
    return head + "".join("\n" + blk for blk in out)


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(description=__doc__)
    for arg in ("base", "ours", "theirs"):
        ap.add_argument(arg, type=Path)
    args = ap.parse_args(argv)
    base, ours, theirs = args.base, args.ours, args.theirs
    try:
        text = merge(base.read_text(), ours.read_text(), theirs.read_text())
    except ValueError as e:
        print(f"[merge_units] conflict: {e}", file=sys.stderr)
        return 1
    ours.write_text(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
