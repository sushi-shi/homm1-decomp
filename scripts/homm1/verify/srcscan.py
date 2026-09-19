"""homm1.verify.srcscan - the shared src/+include/ text-scan substrate.

One comment-blanker, one file walk, one class-definition scanner for every
text gate (board, bans, enum domains, label style, vtable tier), so a class
is never in one worklist under a stricter definition than another.

Scoping (ported from the frozen class_meta): scan src/ + include/ only
(vendor/ is pristine third-party, not ours to annotate); a class definition
is a column-0 `class`/`struct Name ... {` (file scope - nested and local
views are indented); a def under a `template` head is skipped (template
specializations are catalogued by mangled name); rva.h and strstrea.h (the
era-CRT shadow header) are excluded.
"""

from __future__ import annotations

import re
from pathlib import Path

from homm1.core.paths import INCLUDE, REPO, SRC

RVA_H = INCLUDE / "rva.h"
_SKIP = {RVA_H.resolve(), (INCLUDE / "strstrea.h").resolve()}

_CLASS_HEAD_RE = re.compile(r"^(class|struct)\s+([A-Za-z_]\w*)\b")

#: The one label-claim regex family (single-line canonical spellings).
RVA_RE = re.compile(r"\bRVA\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*\)")
RVA_COMPGEN_RE = re.compile(r"\bRVA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)")
DATA_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")


def blank_comments(text: str) -> str:
    """`text` with // and /* */ bodies blanked to spaces (newlines kept), so a
    macro or idiom quoted in prose is never read as real. String/char literals
    are honoured (a `//` inside a string is not a comment)."""
    out, n, i, st = list(text), len(text), 0, "code"
    while i < n:
        c = text[i]
        if st == "code":
            if c == "/" and i + 1 < n and text[i + 1] == "/":
                while i < n and text[i] != "\n":
                    out[i] = " "
                    i += 1
                continue
            if c == "/" and i + 1 < n and text[i + 1] == "*":
                while i < n and not (text[i] == "*" and i + 1 < n
                                     and text[i + 1] == "/"):
                    if text[i] != "\n":
                        out[i] = " "
                    i += 1
                continue
            if c in "\"'":
                st = c
        elif c == "\\":
            i += 2
            continue
        elif c == st:
            st = "code"
        i += 1
    return "".join(out)


def source_files(exts: tuple[str, ...] = (".h", ".cpp")):
    """Every scanned source/header under src/ + include/ (sorted)."""
    for root in (INCLUDE, SRC):
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix in exts and path.is_file() \
                    and path.resolve() not in _SKIP:
                yield path


def rel(path: Path) -> str:
    try:
        return str(path.relative_to(REPO))
    except ValueError:
        return str(path)


def iter_class_defs():
    """Yield (name, path, lineno, body) per file-scope class/struct definition."""
    for path in source_files():
        text = blank_comments(path.read_text(errors="ignore"))
        lines = text.splitlines(keepends=True)
        offs, acc = [], 0
        for ln in lines:
            offs.append(acc)
            acc += len(ln)
        for idx, raw in enumerate(lines):
            m = _CLASS_HEAD_RE.match(raw)
            if not m:
                continue
            prev = idx - 1
            while prev >= 0 and not lines[prev].strip():
                prev -= 1
            if prev >= 0 and lines[prev].lstrip().startswith("template"):
                continue
            start = offs[idx]
            brace = text.find("{", start)
            semi = text.find(";", start)
            if brace < 0 or (0 <= semi < brace):
                continue
            depth, j, end = 0, brace, len(text)
            while j < len(text):
                if text[j] == "{":
                    depth += 1
                elif text[j] == "}":
                    depth -= 1
                    if depth == 0:
                        end = j
                        break
                j += 1
            yield m.group(2), path, idx + 1, text[brace:end]


def index_classes() -> dict[str, tuple[int, list[str]]]:
    """{name: (own_virtual_count, [base_names])} over every class definition.

    First def wins; a later fuller def keeps the max own count (the ported
    vtable_virtuality rule)."""
    out: dict[str, tuple[int, list[str]]] = {}
    head_re = re.compile(r"\b(?:struct|class)\s+(\w+)\b([^;{]*)\{")
    for path in source_files():
        text = blank_comments(path.read_text(errors="ignore"))
        for m in head_re.finditer(text):
            name, bases_txt = m.group(1), m.group(2)
            b = text.index("{", m.start())
            depth, j = 0, b
            while j < len(text):
                if text[j] == "{":
                    depth += 1
                elif text[j] == "}":
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            body = text[b:j]
            own = len(re.findall(r"\bvirtual\b", body))
            bases = []
            if ":" in bases_txt:
                bases = [x.split("::")[-1] for x in
                         re.findall(r"(?:public|protected|private|virtual)?\s*([\w:]+)",
                                    bases_txt.split(":", 1)[1])
                         if x and x not in ("public", "protected", "private",
                                            "virtual")]
            prev = out.get(name)
            if prev is None or own > prev[0]:
                out[name] = (own, bases)
    return out
