"""The classic view: the generated source with its text spelled out.

The source tree names every piece of game text by catalog ID
(`localization::Tr("id")`, `localization::Chars("id")`), and its build
resolves them for the selected language. The classic tree resolves them once,
for the retail program's language (Russian): each reference becomes that text
as a readable UTF-8 literal, and the resource script carries its strings and
language under a UTF-8 code page. The enum and integer model of the source
tree is unchanged.

The view is for reading. Its literals are UTF-8, so a compiler that copies
literal bytes (VC6) would not reproduce the retail Windows-1251 strings;
`homm1 clean --verify` re-encodes them and checks the result against the source
tree's compiler input token by token (verify.classic_equivalence).
"""

from __future__ import annotations

import re

from homm1.graph.catalog import RESOURCE_LANGUAGE, RESOURCE_SUBLANGUAGE, Catalog

#: Escapes that keep a rendered literal on one line and unambiguous.
_ESCAPES = {"\n": "\\n", "\t": "\\t", "\r": "\\r", '"': '\\"', "\\": "\\\\"}


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


def resource_language(catalog: Catalog, locale: str) -> dict[str, str]:
    """The values the rendered resource script gives its LANGUAGE operands."""
    language = catalog.locale(locale).resource_language
    return {RESOURCE_LANGUAGE: f"0x{language & 0x3ff:02x}",
            RESOURCE_SUBLANGUAGE: f"0x{language >> 10:x}"}


def render_cpp(text: str, catalog: Catalog, locale: str) -> str:
    """Catalog references as readable literals of `locale`."""
    messages = catalog.messages(locale)
    out = text
    for start, end, key, _method in reversed(list(catalog.typed_calls(text))):
        out = out[:start] + readable_literal(messages[key]) + out[end:]
    return out


def render_rc(text: str, catalog: Catalog, locale: str) -> str:
    messages = catalog.messages(locale)
    out = text
    for start, end, key, method in reversed(list(catalog.typed_calls(text))):
        if method != "Tr":
            raise ValueError("character arrays are not resource strings")
        out = out[:start] + resource_literal(messages[key]) + out[end:]
    for symbol, value in resource_language(catalog, locale).items():
        out = re.sub(r"\b" + symbol + r"\b", value, out)
    return "#pragma code_page(65001)\n\n" + out
