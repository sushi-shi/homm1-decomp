"""Check that compiled source tokens use explicit bytes for non-ASCII text.

HoMM1 text must follow its own retail bytes.
This audit is opt-in and makes no encoding claim about HoMM1 resources.
"""

from __future__ import annotations

from homm1.core.usage import logged


from homm1.verify.srcscan import blank_comments, rel, source_files


def gate_findings() -> list[str]:
    out = []
    for path in source_files():
        text = path.read_text(encoding="utf-8", errors="replace")
        code = blank_comments(text)
        for lineno, line in enumerate(code.splitlines(), 1):
            bad = [ch for ch in line if ord(ch) > 0x7f]
            if bad:
                out.append(f"{rel(path)}:{lineno}: non-ASCII {''.join(bad)[:12]!r} "
                           f"outside a comment - write verified retail bytes as "
                           f"octal escapes with the text in a comment")
    return out


@logged
def main(argv=None) -> int:
    import argparse
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    findings = gate_findings()
    for f in findings:
        print(f"[source-encoding] {f}")
    print(f"[source-encoding] {len(findings)} finding(s)")
    return 1 if findings else 0


if __name__ == "__main__":
    raise SystemExit(main())
