"""homm1.verify.bans - hard-fail guard for the four manual-vtable idioms.

The WAP hand-rolled-vtable metrics the vtable campaign drove to 0: `*Vtbl`
structs, `->vtbl` accesses, `g_*Vtbl` globals, `m_vtbl`/`m_vptr` members.
Every explicit vtable must be a real C++ `virtual`, so NONE may reappear.
The regexes come straight from the board's METRICS so the guard and the
score can never drift apart. Comments/strings are blanked first.

    python3 -m homm1.verify.bans          # exit 1 on any hit (FATAL, no floor)
"""

from __future__ import annotations

import sys

from homm1.verify.board import BANNED_LABELS, METRICS
from homm1.verify.srcscan import blank_comments, rel, source_files

BANNED = [(label, rx) for label, rx, _ in METRICS if label in BANNED_LABELS]


def scan():
    for path in source_files():
        text = blank_comments(path.read_text(errors="ignore"))
        for label, rx in BANNED:
            for m in rx.finditer(text):
                yield (label, path, text.count("\n", 0, m.start()) + 1,
                       m.group(0).strip())


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    import argparse
    argparse.ArgumentParser(
        prog="homm1 verify bans", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter).parse_args(argv)
    hits = list(scan())
    if not hits:
        print("vtable-bans: OK - none of the 4 banned manual-vtable idioms "
              "present")
        return 0
    by_label: dict = {}
    for label, path, lineno, tok in hits:
        by_label.setdefault(label, []).append((path, lineno, tok))
    print(f"vtable-bans: FAIL - {len(hits)} banned manual-vtable idiom(s) "
          f"present (these must stay 0 - convert to real virtuals):",
          file=sys.stderr)
    for label, _rx in BANNED:
        xs = by_label.get(label, [])
        if not xs:
            continue
        print(f"  [{label}] {len(xs)}:", file=sys.stderr)
        for path, lineno, tok in xs[:100]:
            print(f"    {rel(path)}:{lineno}: {tok}", file=sys.stderr)
        if len(xs) > 100:
            print(f"    ... and {len(xs) - 100} more", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
