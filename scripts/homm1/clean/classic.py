"""The classic view: the generated source with its text spelled out in Russian.

The source tree names every piece of game text by catalog ID
(`localization::Tr("id")`, `localization::Chars("id")`) and selects the
Russian-only code paths with `HOMM1_RUSSIAN`; its build resolves both. The
classic tree resolves them once, for the Russian retail program: each
reference becomes the Russian text as a readable UTF-8 literal, each
`HOMM1_RUSSIAN` conditional keeps its Russian branch, and the resource script
carries its Russian strings under a UTF-8 code page. The enum and integer
model of the source tree is unchanged.

The view is for reading. Its literals are UTF-8, so a compiler that copies
literal bytes (VC6) would not reproduce the retail Windows-1251 strings;
`homm1 clean --verify` re-encodes them and checks the result against the source
tree's Russian compiler input token by token (verify.classic_equivalence).
"""

from __future__ import annotations

import re

from homm1.graph.catalog import Catalog

#: Escapes that keep a rendered literal on one line and unambiguous.
_ESCAPES = {"\n": "\\n", "\t": "\\t", "\r": "\\r", '"': '\\"', "\\": "\\\\"}
RUSSIAN_SYMBOL = "HOMM1_RUSSIAN"
RESOURCE_LANGUAGE = "HOMM1_RESOURCE_LANGUAGE"
#: LANG_RUSSIAN, the resource language of the retail Buka executable.
LANG_RUSSIAN = "0x19"


def readable_literal(value: str) -> str:
    """A C string literal that shows `value` as text; controls stay escaped
    (fixed-width octal, so a following digit is never absorbed)."""
    out = []
    for ch in value:
        if ch in _ESCAPES:
            out.append(_ESCAPES[ch])
        elif ord(ch) < 32 or ord(ch) == 127:
            out.append("\\%03o" % ord(ch))
        else:
            out.append(ch)
    return '"' + "".join(out) + '"'


def resource_literal(value: str) -> str:
    """An RC string literal: `""` doubles a quote, newlines stay `\\n`."""
    return '"' + value.replace('"', '""').replace("\n", "\\n").replace("\t", "\\t") + '"'


_CONDITIONAL = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$")


def resolve_conditionals(text: str, symbol: str = RUSSIAN_SYMBOL) -> str:
    """Keep the Russian branch of every `#if symbol` / `#if !symbol`.

    Other conditionals pass through untouched (nested ones are tracked so
    their #else/#endif are not mistaken for ours). Verification passes the
    `1` a Russian build substitutes for HOMM1_RUSSIAN."""
    out: list[str] = []
    stack: list[tuple[bool, bool]] = []   # (ours, keeping current branch)
    for line in text.split("\n"):
        match = _CONDITIONAL.match(line)
        keeping = all(keep for ours, keep in stack if ours)
        if match:
            kind, rest = match.group(1), match.group(2).strip()
            if kind == "if" and rest in (symbol, "!" + symbol):
                stack.append((True, rest == symbol))
                continue
            if kind in ("if", "ifdef", "ifndef"):
                stack.append((False, True))
            elif stack and stack[-1][0]:
                if kind == "elif":
                    raise ValueError(f"#elif after #if {RUSSIAN_SYMBOL} is not supported")
                if kind == "else":
                    stack[-1] = (True, not stack[-1][1])
                    continue
                if kind == "endif":
                    stack.pop()
                    continue
            elif kind == "endif":
                if not stack:
                    raise ValueError("unbalanced #endif")
                stack.pop()
        if keeping:
            out.append(line)
    if stack:
        raise ValueError("unterminated conditional")
    if any(RUSSIAN_SYMBOL in line for line in out):
        raise ValueError(f"{RUSSIAN_SYMBOL} survives outside an #if")
    return "\n".join(out)


def render_cpp(text: str, catalog: Catalog) -> str:
    """Catalog references as Russian literals; Russian conditionals resolved."""
    out = text
    for start, end, key, _method in reversed(list(catalog.typed_calls(text))):
        out = out[:start] + readable_literal(catalog.russian[key]) + out[end:]
    return resolve_conditionals(out)


def render_rc(text: str, catalog: Catalog) -> str:
    out = text
    for start, end, key, method in reversed(list(catalog.typed_calls(text))):
        if method != "Tr":
            raise ValueError("character arrays are not resource strings")
        out = out[:start] + resource_literal(catalog.russian[key]) + out[end:]
    out = re.sub(r"\b" + RESOURCE_LANGUAGE + r"\b", LANG_RUSSIAN, out)
    return "#pragma code_page(65001)\n\n" + out
