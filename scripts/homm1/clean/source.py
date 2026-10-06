"""Source transforms for the generated clean tree.

Every rule reproduces the production expansion that the pinned VC6 compiler
already sees when it builds the matching objects; nothing here changes what
the game does. The lexer never reaches inside string or character literals,
removes comments without joining tokens, and expands macro arguments
innermost first. See docs/clean-source.md.
"""

from __future__ import annotations

import re

#: Sentinels: a construct that expanded to nothing, and a removed comment.
#: Line cleanup uses them to drop lines that held nothing else.
DROPPED = "\x01"
COMMENT = "\x02"

#: Annotation and enum-machinery headers. The tree omits them and every
#: `#include` of them; their macros are resolved at each use.
DROP_HEADERS = ("match.h", "Domains.h", "H1/Macros.h")
DROP_FILES = tuple(f"include/{name}" for name in DROP_HEADERS)
#: Real declarations a dropped header also carries, and where they live in the
#: clean tree: match.h defines the integer aliases of H1/Ints.h so every unit
#: has them without opening another header (a /Gi path-state concern only).
REPLACE_HEADERS = {"match.h": "H1/Ints.h"}


def _drop(args: list[str]) -> str:
    return DROPPED


def _arg(index: int, template: str = "{}"):
    def rule(args: list[str]) -> str:
        return template.format(args[index])
    return rule


_SIMPLE_OPERAND = re.compile(r"[A-Za-z_][\w.]*(?:->[\w.]+|\[[^\[\]]*\])*|\d+")


def _parenthesized(index: int):
    """The retail `(value)` expansion, without parentheses a lone operand
    does not need."""
    def rule(args: list[str]) -> str:
        value = args[index].strip()
        return value if _SIMPLE_OPERAND.fullmatch(value) else f"({value})"
    return rule


def _enum_open(args: list[str]) -> str:
    return f"enum {args[0]} {{"


def _enum_close(args: list[str]) -> str:
    return "};"


#: Macro invocation -> its production expansion. Keep in step with
#: include/match.h, include/Domains.h and include/H1/Macros.h.
CALL_RULES = {
    # Retail-address and delinker metadata: no expansion at all.
    "VA": (2, _drop),
    "VA_AT": (3, _drop),
    "VA_DECL": (1, _drop),
    "VA_AT": (3, _drop),
    "DATA": (1, _drop),
    "VA_COMPGEN": (4, _drop),
    "RVA_DYNINIT": (3, _drop),
    # Enum domains: the named enum declaration, and the retail storage type
    # that the domain's audited fields, parameters and returns occupy.
    "H1_ENUM_BEGIN": (1, _enum_open),
    "H1_ENUM_END": (1, _enum_close),
    "H1_ENUM_BEGIN_SPLIT": (2, _enum_open),
    "H1_ENUM_END_SPLIT": (1, _enum_close),
    "H1_ENUM_FLAGS_BEGIN": (2, _enum_open),
    "H1_ENUM_FLAGS_END": (1, _enum_close),
    "H1_ENUM_CONST_BEGIN": (1, _enum_open),
    "H1_ENUM_CONST_END": (1, _enum_close),
    "H1_ENUM_ID_BEGIN": (1, _enum_open),
    "H1_ENUM_ID_END": (1, _enum_close),
    "H1_ENUM_ARRAY": (4, lambda args: f"{args[0]} {args[1]}[{args[3]}]"),
    "H1_ENUM_ARRAY2": (6, lambda args: f"{args[0]} {args[1]}[{args[3]}][{args[5]}]"),
    "H1_ENUM_ARRAY_ROWS": (5, lambda args: f"{args[0]} {args[1]}[{args[3]}][{args[4]}]"),
    "H1_ENUM_STEPPED": (1, _drop),
    "H1_ENUM_SHARED": (2, _arg(1)),
    "H1_ENUM_BIT": (2, lambda args: f"(1 << {_parenthesized(1)(args)})"),
    "H1_ENUM_DECODE": (2, _parenthesized(1)),
    "H1_ENUM_ENCODE": (2, _parenthesized(1)),
    "H1_ENUM_PARAM": (2, _arg(1)),
    "H1_ENUM_RETURN": (2, _arg(1)),
    "H1_ENUM_LOCAL": (2, _arg(1)),
    "H1_ENUM_STORAGE": (2, _arg(1)),
    "H1_ENUM_CAST": (3, lambda args: f"static_cast<{args[1]}>({args[2]})"),
    # Domain-indexed arrays: the plain array of the retail storage type.
    "H1_ENUM_ARRAY": (4, lambda args: f"{args[0]} {args[1]}[{args[3]}]"),
    "H1_ENUM_ARRAY2": (6, lambda args: f"{args[0]} {args[1]}[{args[3]}][{args[5]}]"),
    "H1_ENUM_STEPPED": (1, _drop),
}

#: Bare identifiers.
WORD_RULES = {
    "OVERRIDE": DROPPED,
    "H1_C_LINKAGE": 'extern "C"',
}

#: Names that must not survive anywhere outside literals.
RESIDUE_NAMES = frozenset(CALL_RULES) | frozenset(WORD_RULES) | {
    "HOMM1_MATCH_H", "HOMM1_DOMAINS_H", "HOMM1_H1_MACROS_H", "H1EnumStorage",
    "H1EnumShared", "H1EnumArray", "H1EnumBit", "H1EnumDecode", "H1EnumEncode",
    "H1_STRICT_DOMAINS", "H1Bool",
}
RESIDUE_PREFIXES = ("H1_ENUM_",)


# --------------------------------------------------------------------------
# Lexer
# --------------------------------------------------------------------------

def tokens(text: str, *, asm: bool = False, rc: bool = False):
    """Yield (kind, spelling): comment, literal, word, number, space, punct.

    `asm` lexes MASM (`;` comments, quoted strings); `rc` lexes a resource
    script, whose `"` strings double an embedded quote instead of escaping it.
    """
    i, n = 0, len(text)
    while i < n:
        start = i
        ch = text[i]
        if asm and ch == ";":
            i = text.find("\n", i)
            i = n if i < 0 else i
            yield "comment", text[start:i]
        elif not asm and text.startswith("//", i):
            i = text.find("\n", i)
            i = n if i < 0 else i
            # A trailing backslash continues a line comment onto the next line.
            while not rc and i < n and text[i - 1] == "\\":
                following = text.find("\n", i + 1)
                i = n if following < 0 else following
            yield "comment", text[start:i]
        elif not asm and text.startswith("/*", i):
            end = text.find("*/", i + 2)
            if end < 0:
                raise ValueError("unterminated block comment")
            i = end + 2
            yield "comment", text[start:i]
        elif ch == '"' or (ch == "'" and not rc):
            i += 1
            while i < n and text[i] != ch:
                if text[i] == "\n":
                    raise ValueError("unterminated literal")
                if text[i] == "\\" and not asm and not rc:
                    i += 1
                i += 1
            if i >= n:
                raise ValueError("unterminated literal")
            i += 1
            if rc:
                while i < n and text[i] == '"':   # "" is an embedded quote
                    i = text.index('"', i + 1) + 1
            yield "literal", text[start:i]
        elif ch.isalpha() or ch == "_":
            while i < n and (text[i].isalnum() or text[i] == "_"):
                i += 1
            yield "word", text[start:i]
        elif ch.isdigit() or (ch == "." and i + 1 < n and text[i + 1].isdigit()):
            # A preprocessing number, so `0x1Fu` is never read as a name.
            i += 1
            while i < n and (text[i].isalnum() or text[i] in "_."
                             or (text[i] in "+-" and text[i - 1] in "eEpP")):
                i += 1
            yield "number", text[start:i]
        elif ch in " \t\r\n":
            while i < n and text[i] in " \t\r\n":
                i += 1
            yield "space", text[start:i]
        else:
            i += 1
            yield "punct", ch


def strip_comments(text: str, **kind) -> str:
    """Replace each comment with a sentinel, keeping every line break."""
    out = []
    for token, spelling in tokens(text, **kind):
        if token == "comment":
            out.append(COMMENT + "".join("\n" + COMMENT for _ in range(spelling.count("\n"))))
        else:
            out.append(spelling)
    return "".join(out)


def rewrite(text: str, *, keep_lines: bool = False) -> str:
    """Expand every scaffolding macro, innermost first, outside literals.

    `keep_lines` leaves a multi-line invocation's line breaks in place."""
    stream = list(tokens(text))

    def expand(parts: list[tuple[str, str]]) -> str:
        out, i = [], 0
        while i < len(parts):
            kind, spelling = parts[i]
            if kind == "word" and spelling in WORD_RULES:
                out.append(WORD_RULES[spelling])
                i += 1
                continue
            if kind != "word" or spelling not in CALL_RULES:
                out.append(spelling)
                i += 1
                continue
            opening = i + 1
            while opening < len(parts) and parts[opening][0] == "space":
                opening += 1
            if opening == len(parts) or parts[opening][1] != "(":
                raise ValueError(f"{spelling}: expected a macro invocation")
            depth, args, start, cursor = 0, [], opening + 1, opening
            while cursor < len(parts):
                token, piece = parts[cursor]
                if token == "punct" and piece in "([{":
                    depth += 1
                elif token == "punct" and piece in ")]}":
                    depth -= 1
                    if depth == 0:
                        args.append(expand(parts[start:cursor]).strip())
                        break
                elif token == "punct" and piece == "," and depth == 1:
                    args.append(expand(parts[start:cursor]).strip())
                    start = cursor + 1
                cursor += 1
            else:
                raise ValueError(f"{spelling}: unterminated macro invocation")
            arity, rule = CALL_RULES[spelling]
            if len(args) != arity:
                raise ValueError(f"{spelling}: expected {arity} argument(s), got {len(args)}")
            # A multi-line invocation becomes one line; its arguments' own
            # line breaks are kept only inside literals.
            raw = "".join(parts[k][1] for k in range(i, cursor + 1))
            breaks = raw.count("\n")
            # Inside a #define the breaks are continuations: the joined
            # expansion keeps them as backslash-newlines.
            continued = bool(re.search(r"\\[ \t]*\n", raw))
            if continued:
                args = [re.sub(r"\s*\\[ \t]*\n\s*", " ", arg) for arg in args]
            if not keep_lines:
                args = [re.sub(r"\s*\n\s*", " ", arg) for arg in args]
                breaks = 0
            expansion = rule(args)
            newline = " \\\n" if continued else "\n"
            out.append(expansion + newline * (breaks - expansion.count("\n")))
            i = cursor + 1
        return "".join(out)

    return expand(stream)


_INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
_LINE = re.compile(r"^\s*#\s*line\b")


_STRICT_IF = re.compile(r"^\s*#\s*if\s+H1_STRICT_DOMAINS\s*$")
_PP_IF = re.compile(r"^\s*#\s*if(?:n?def)?\b")
_PP_ELSE = re.compile(r"^\s*#\s*else\b")
_PP_ENDIF = re.compile(r"^\s*#\s*endif\b")


def drop_strict_blocks(lines: list[str]) -> list[str]:
    """Keep only the retail branch of each `#if H1_STRICT_DOMAINS` block: the
    strict-domain view is analysis scaffolding (Domains.h), and the retail
    branch is what VC6 compiles. Dropped lines become sentinels, so the
    line-preserving control form keeps its numbering."""
    out, depth, keeping = [], 0, True
    for line in lines:
        if depth == 0:
            if _STRICT_IF.match(line):
                depth, keeping = 1, False
                out.append(DROPPED)
            else:
                out.append(line)
            continue
        if _PP_IF.match(line):
            depth += 1
        elif depth == 1 and _PP_ELSE.match(line):
            keeping = True
            out.append(DROPPED)
            continue
        elif _PP_ENDIF.match(line):
            depth -= 1
            if depth == 0:
                out.append(DROPPED)
                keeping = True
                continue
        out.append(line if keeping else DROPPED)
    return out


def rewrite_directives(text: str, *, keep_lines: bool = False) -> str:
    """Drop `#line` pins, the scaffolding headers' includes and the strict
    view's `#if H1_STRICT_DOMAINS` branches.

    `keep_lines` keeps the pins and includes: the control build's path and
    line state."""
    if keep_lines:
        return "\n".join(drop_strict_blocks(text.split("\n")))
    lines = drop_strict_blocks(text.split("\n"))
    included: set[str] = set()
    out = []
    for line in lines:
        if _LINE.match(line):
            out.append(DROPPED)
            continue
        include = _INCLUDE.match(line)
        if include and include.group(1) in DROP_HEADERS:
            replacement = REPLACE_HEADERS.get(include.group(1))
            if replacement and replacement not in included:
                out.append(f"#include <{replacement}>")
                included.add(replacement)
            else:
                out.append(DROPPED)
            continue
        if include and include.group(1) in REPLACE_HEADERS.values():
            if include.group(1) in included:
                out.append(DROPPED)         # already opened where match.h was
                continue
            included.add(include.group(1))
        out.append(line)
    return "\n".join(out)


def tidy(text: str) -> str:
    """Remove the lines dropped constructs and comments emptied, and the
    holes they left; collapse blank runs to one blank line."""
    lines = []
    for line in text.split("\n"):
        if DROPPED in line or COMMENT in line:
            bare = line.replace(DROPPED, "").replace(COMMENT, "").strip()
            if bare == "" or (bare == ";" and DROPPED in line):
                continue
            line = re.sub(DROPPED + r"[ \t]*", "", line)
            # A removed comment between two tokens leaves one space.
            line = re.sub(r"(?<=\S)" + COMMENT + r"+(?=\S)", " ", line)
            line = line.replace(COMMENT, "")
        lines.append(line.rstrip())
    text = "\n".join(lines)
    text = re.sub(r"\n{3,}", "\n\n", text)
    # No blank line just inside a brace or before a closing one.
    text = re.sub(r"\{\n\n", "{\n", text)
    text = re.sub(r"\n\n(\s*\})", r"\n\1", text)
    return text.strip("\n") + "\n"


def blank(text: str) -> str:
    """The control form of `tidy`: remove sentinels, keep every line."""
    text = re.sub(r"(?<=\S)" + COMMENT + r"+(?=\S)", " ", text)
    return "\n".join(line.rstrip() for line in
                     text.replace(DROPPED, "").replace(COMMENT, "").split("\n"))


#: `.bss` layout aliases: `#define <readable> <storage> // spelling fixes .bss
#: order`. The matching build hashes the storage spelling (VC6 orders `.bss` by
#: name key); the generated trees do not reproduce that layout, so they drop
#: the define and keep the readable name the source already spells.
_ALIAS = re.compile(r"^[ \t]*#[ \t]*define[ \t]+(\w+)[ \t]+(\w+)[ \t]*"
                    r"//[ \t]*spelling fixes \.bss order[ \t]*$", re.M)


def aliases(texts) -> dict[str, str]:
    """{storage spelling: readable name} over the given source texts."""
    found: dict[str, str] = {}
    for text in texts:
        for readable, storage in _ALIAS.findall(text):
            found[storage] = readable
    return found


def drop_aliases(text: str) -> str:
    return _ALIAS.sub(DROPPED, text)


#: Function-scoped `/Od` frame-slot aliases. VC6 orders a function's named
#: locals by a hash of their spelling (docs/patterns/vc6-od-frame-slots.md);
#: where the readable name falls in the wrong slot, the function is bracketed
#:
#:     #define minX ourFirstX // frame-slot spelling
#:     void advManager::UpdateRadar() { i32 minX; ... }
#:     #undef minX
#:
#: The body spells the readable name and the matching build compiles the
#: storage spelling. The generated trees drop the pair and keep the readable
#: name, which is what the function then spells throughout.
_LOCAL_ALIAS = re.compile(r"^[ \t]*#[ \t]*define[ \t]+(\w+)[ \t]+(\w+)[ \t]*"
                          r"//[ \t]*frame-slot spelling[ \t]*$")
_LOCAL_MARKER = re.compile(r"//[ \t]*frame-slot spelling")
_UNDEF = re.compile(r"^[ \t]*#[ \t]*undef[ \t]+(\w+)[ \t]*(?://.*)?$")


def local_aliases(text: str) -> list[tuple[int, int, dict[str, str]]]:
    """Each frame-slot alias group as (first define line, last undef line,
    {readable: storage}), 0-based and inclusive. Raises ValueError for an
    unbalanced pair, a group that brackets anything but one function
    definition, or a spelling the define would capture or split."""
    lines = text.split("\n")
    groups: list[tuple[int, int, dict[str, str]]] = []
    closed_names: set[str] = set()
    i = 0
    while i < len(lines):
        if not _LOCAL_MARKER.search(lines[i]):
            undef = _UNDEF.match(lines[i])
            if undef and undef.group(1) in closed_names:
                raise ValueError(f"line {i + 1}: #undef {undef.group(1)} outside its "
                                 "function's frame-slot alias group")
            i += 1
            continue
        start, names = i, {}
        while i < len(lines) and _LOCAL_MARKER.search(lines[i]):
            define = _LOCAL_ALIAS.match(lines[i])
            if not define:
                raise ValueError(f"line {i + 1}: malformed frame-slot alias")
            readable, storage = define.groups()
            if readable in names or storage in names.values() or readable == storage:
                raise ValueError(f"line {i + 1}: duplicate frame-slot alias {readable}")
            names[readable] = storage
            i += 1
        body = i
        while i < len(lines) and not _UNDEF.match(lines[i]) \
                and not _LOCAL_MARKER.search(lines[i]):
            i += 1
        end = i
        closed: set[str] = set()
        while i < len(lines) and (undef := _UNDEF.match(lines[i])) and undef.group(1) in names:
            if undef.group(1) in closed:
                raise ValueError(f"line {i + 1}: #undef {undef.group(1)} repeated")
            closed.add(undef.group(1))
            i += 1
        if closed != set(names):
            missing = ", ".join(sorted(set(names) - closed))
            raise ValueError(f"line {start + 1}: frame-slot alias {missing} is not "
                             "#undef'd right after the function's closing brace")
        _check_extent("\n".join(lines[body:end]), names, start + 1)
        groups.append((start, i - 1, names))
        closed_names |= closed
    return groups


_ELABORATED = ("class", "struct", "union", "enum")


def _check_extent(function: str, names: dict[str, str], line: int) -> None:
    """The bracketed text must be one function definition in which each
    readable name only ever spells that function's own local."""
    words = [(k, s) for k, s in tokens(function) if k not in ("space", "comment")]
    braces = [n for n, (k, s) in enumerate(words) if k == "punct" and s in "{}"]
    if not braces or words[braces[0]][1] != "{" or braces[-1] != len(words) - 1:
        raise ValueError(f"line {line}: a frame-slot alias group must bracket exactly one "
                         "function definition")
    depth = 0
    for n in braces:
        depth += 1 if words[n][1] == "{" else -1
        if depth == 0 and n != braces[-1]:
            raise ValueError(f"line {line}: a frame-slot alias group brackets more than one "
                             "function definition")
    if depth:
        raise ValueError(f"line {line}: unbalanced braces in an aliased function")
    for readable, storage in names.items():
        spelled = [n for n, (k, s) in enumerate(words) if k == "word" and s == readable]
        if not spelled:
            raise ValueError(f"line {line}: frame-slot alias {readable} is never used")
        # The storage spelling may still name a type (`class sample* sound;`
        # behind `#define sound sample`); anything else would be one name in
        # the matching build and two in the generated trees.
        if any(k == "word" and s == storage and words[n - 1][1] not in _ELABORATED
               for n, (k, s) in enumerate(words)):
            raise ValueError(f"line {line}: {storage} is also spelled inside the function; "
                             f"without '#define {readable} {storage}' the two would split")
        for n in spelled:
            before = "".join(s for _k, s in words[max(0, n - 2):n])
            after = "".join(s for _k, s in words[n + 1:n + 3])
            if n < braces[0] or before.endswith((".", "->", "::")) or after == "::" \
                    or words[n - 1][1] in _ELABORATED:
                raise ValueError(f"line {line}: {readable} also spells a parameter, member, "
                                 "qualified name or type that the alias would capture")


def drop_local_aliases(text: str) -> str:
    """`text` without its (validated) frame-slot define and undef lines."""
    lines = text.split("\n")
    for start, end, names in local_aliases(text):
        for n in range(start, end + 1):
            undef = _UNDEF.match(lines[n])
            if _LOCAL_MARKER.search(lines[n]) or (undef and undef.group(1) in names):
                lines[n] = DROPPED
    return "\n".join(lines)


def rename_words(text: str, renames: dict[str, str], **kind) -> str:
    """`text` with each word token in `renames` replaced, outside comments
    and literals."""
    if not renames:
        return text
    return "".join(renames.get(spelling, spelling) if token == "word" else spelling
                   for token, spelling in tokens(text, **kind))


def clean_cpp(text: str, *, keep_lines: bool = False) -> str:
    """The clean form; `keep_lines` gives the line-preserving control form."""
    if keep_lines:
        local_aliases(text)         # validated; the control compiles them as-is
    else:
        text = drop_local_aliases(drop_aliases(text))
    text = rewrite(rewrite_directives(strip_comments(text), keep_lines=keep_lines),
                   keep_lines=keep_lines)
    return blank(text) if keep_lines else tidy(text)


def clean_asm(text: str, *, keep_lines: bool = False,
              renames: dict[str, str] | None = None) -> str:
    """`renames` maps `.bss` storage spellings to their readable names."""
    if not keep_lines:
        text = rename_words(text, renames or {}, asm=True)
    text = strip_comments(text, asm=True)
    return blank(text) if keep_lines else tidy(text)


def clean_rc(text: str, *, keep_lines: bool = False) -> str:
    text = strip_comments(text, rc=True)
    return blank(text) if keep_lines else tidy(text)


def residue(text: str) -> list[str]:
    """Scaffolding and comments that survived, found with the same lexer."""
    found = []
    for kind, spelling in tokens(text):
        if kind == "comment":
            found.append("comment")
        elif kind == "word" and (spelling in RESIDUE_NAMES or spelling.startswith(RESIDUE_PREFIXES)):
            found.append(spelling)
    include = [m.group(1) for m in map(_INCLUDE.match, text.split("\n")) if m]
    found += [f"#include <{name}>" for name in include if name in DROP_HEADERS]
    found += ["#line" for line in text.split("\n") if _LINE.match(line)]
    return found


def _punctuation_only(text: str) -> list[str]:
    return [line.strip() for line in text.split("\n")
            if line.strip() and not line.strip().strip(";,")]


def stranded(source: str, cleaned: str, **kind) -> list[str]:
    """Lines of bare `;`/`,` that a dropped construct left behind, measured as
    a delta against the input (a lone `;` is also a legitimate statement)."""
    before = _punctuation_only(tidy(strip_comments(source, **kind)))
    after = _punctuation_only(cleaned)
    return after[len(before):] if len(after) > len(before) else []
