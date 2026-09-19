"""homm1.verify.board - the cleanliness scoreboard (fast tier).

Cast / placeholder / view counts that trend to 0 as the type/call/name layer
is cleaned up, measured against the committed floors in
config/cleanliness/cleanliness-{text,semantic}-baseline.tsv.

Gate semantics (ported): a RATCHETED metric may never RISE above its committed
floor; other metrics are tracked. The gate never writes the baselines - a
floor is only ever lowered by the manual `--update` bless. A metric that could
not be measured keeps its committed floor and is reported as unmeasured,
never blessed away (the silently-dropped-floor lesson).

    python3 -m homm1.verify.board            # counts + delta vs the floors
    python3 -m homm1.verify.board --gate     # exit 1 on any ratchet rise
    python3 -m homm1.verify.board --semantic # include the build-derived rows
    python3 -m homm1.verify.board --update   # bless: write the floor files
"""

from __future__ import annotations

import re
import sys

from homm1.core.paths import CONFIG, REPO
from homm1.verify.srcscan import blank_comments

ROOTS = ("src", "include")
EXTS = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".inl"}
_CPP = {".cpp", ".cc", ".cxx"}
TEXT_BASELINE = CONFIG / "cleanliness/cleanliness-text-baseline.tsv"
SEMANTIC_BASELINE = CONFIG / "cleanliness/cleanliness-semantic-baseline.tsv"

_STR = re.compile(r'"(?:\\.|[^"\\\n])*"')
_CHR = re.compile(r"'(?:\\.|[^'\\\n])*'")
_DEAD = re.compile(r"//\s*@dead-code\b")


def _blank_dead(text: str) -> str:
    """Blank each `// @dead-code`-marked function (marker .. matching `}`):
    a PROVEN-zero-ref body's placeholder name is a permanent non-actionable
    artifact, excluded exactly like a library carve-out."""
    out = list(text)
    for m in _DEAD.finditer(text):
        b = text.find("{", m.start())
        if b < 0:
            continue
        depth, i, n = 0, b, len(text)
        while i < n:
            c = text[i]
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        for k in range(m.start(), min(i + 1, n)):
            if out[k] != "\n":
                out[k] = " "
    return "".join(out)


def _strip(text: str) -> str:
    text = _blank_dead(text)
    text = blank_comments(text)
    text = _STR.sub(" ", text)
    text = _CHR.sub(" ", text)
    return text


# --- structural counters (ported verbatim; each earned its shape) ----------- #
_TYPEDEF = re.compile(r"\b(?:struct|class)\s+(\w+)")
_HEXRUN = re.compile(r"[0-9a-f]{4,}")


def _is_placeholder(name: str) -> bool:
    return any(any(c.isdigit() for c in run) for run in _HEXRUN.findall(name))


def _count_placeholders(code: str) -> int:
    return sum(1 for n in _TYPEDEF.findall(code) if _is_placeholder(n))


_ADDRESS_DERIVED_IDENTIFIER = re.compile(
    r"\b(?:local_[0-9a-f]+"
    r"|m_[0-9][0-9a-f]*(?:[A-Za-z_]\w*)?"
    r"|[gsm]_[A-Za-z_]\w*_[0-9a-f]{4,})\b")


def _count_address_derived_identifiers(code: str) -> int:
    return sum(1 for name in _ADDRESS_DERIVED_IDENTIFIER.findall(code)
               if any(c.isdigit() for c in name))


_TYPEDEF_DEF = re.compile(
    r"\b(?:struct|class)\s+(\w+)\b(?:\s+final)?\s*(?::[^;{]*)?\{")


def _count_cpp_local_defs(code: str) -> int:
    return len(_TYPEDEF_DEF.findall(code))


_VTSLOT = re.compile(
    r"virtual\b[^;{}\n]*\b(?:dummy[0-9]+|vfunc[0-9]*|[Ss]lot[0-9]+"
    r"|[sv][0-9a-f]{2,3}(?![0-9a-z])"
    r"|[SsVv]f[0-9a-f]+)\s*\(")

_M_CAST = re.compile(r"\)m_[A-Za-z0-9_]")
_STR_M_CAST = re.compile(r"\((?:const |unsigned |signed )*char ?\*\)m_[A-Za-z0-9_]")


def _count_nonstring_m_casts(code: str) -> int:
    return len(_M_CAST.findall(code)) - len(_STR_M_CAST.findall(code))


_OFFSET_MACRO_DEF = re.compile(
    r"#define\s+(\w+)\s*\([^)]*\)\s*\(\s*\*\s*\(\s*\w[\w ]*\*\s*\)\s*"
    r"\(\s*\(\s*char\s*\*\s*\)")


def _count_offset_macro_casts(code: str) -> int:
    total = 0
    for m in _OFFSET_MACRO_DEF.finditer(code):
        total += len(re.findall(r"\b" + re.escape(m.group(1)) + r"\s*\(", code))
    return total


_CPP_EXTERN = re.compile(r"^\s*extern\b", re.MULTILINE)
_HEADER_EXTS = {".h", ".hpp", ".inl"}
_EXTERN_DECL = re.compile(r'\bextern\b\s*(?:"[^"]*")?\s*(?![{;])([^;{}]*);',
                          re.DOTALL)
_EXTERN_BLOCK = re.compile(r'\bextern\s*(?:"[^"]*")?\s*\{')
_IDENT = re.compile(r"[A-Za-z_]\w*")
_ARRAY_BOUND = re.compile(r"\[[^\]]*\]")
_DECLSPEC = re.compile(
    r"\b(?:__declspec|__attribute__)\s*\([^()]*(?:\([^()]*\)[^()]*)*\)")


def _declarator_name(part: str) -> str | None:
    head = _DECLSPEC.sub(" ", part)
    head = head.split("(", 1)[0]
    head = _ARRAY_BOUND.sub(" ", head)
    ids = _IDENT.findall(head)
    return ids[-1] if ids else None


def _header_extern_names(code: str) -> list[str]:
    """Every symbol an `extern` declaration in one header declares (plain,
    `extern "C"`, comma lists, and `extern "C" { ... }` block members)."""
    names: list[str] = []
    spliced = code
    for m in _EXTERN_BLOCK.finditer(code):
        depth, i, n = 0, m.end() - 1, len(code)
        while i < n:
            if code[i] == "{":
                depth += 1
            elif code[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        body = code[m.end():i]
        spliced += "\n" + "\n".join(
            f"extern {stmt};" for stmt in body.split(";")
            if stmt.strip() and "extern" not in stmt)
    for m in _EXTERN_DECL.finditer(spliced):
        decl = m.group(1)
        if "=" in decl:
            continue
        parts, depth, cur = [], 0, ""
        for ch in decl:
            if ch in "([":
                depth += 1
            elif ch in ")]":
                depth -= 1
            if ch == "," and depth == 0:
                parts.append(cur)
                cur = ""
            else:
                cur += ch
        parts.append(cur)
        for part in parts:
            name = _declarator_name(part)
            if name:
                names.append(name)
    return names


def duplicate_header_externs() -> dict[str, list[str]]:
    import collections
    seen: dict[str, list[str]] = collections.defaultdict(list)
    for root in ROOTS:
        base = REPO / root
        if not base.is_dir():
            continue
        for path in sorted(base.rglob("*")):
            if path.suffix not in _HEADER_EXTS or not path.is_file():
                continue
            try:
                code = _strip(path.read_text(errors="ignore"))
            except OSError:
                continue
            r = str(path.relative_to(REPO))
            for name in _header_extern_names(code):
                seen[name].append(r)
    return {k: v for k, v in seen.items() if len(v) > 1}


_CPP_PROTO = re.compile(
    r"^\s*"
    r"(?!typedef\b|using\b|return\b|if\b|for\b|while\b|switch\b)"
    r"(?:(?:extern|static|inline|constexpr|friend)\s+)*"
    r"(?:[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*(?:\s*<[^;{}()]*>)?"
    r"(?:\s+const)?\s*(?:[*&]\s*)?\s+)+"
    r"(?:__cdecl\s+|__stdcall\s+|WINAPI\s+|CALLBACK\s+)*"
    r"[A-Za-z_~]\w*(?:::[A-Za-z_~]\w*)*\s*"
    r"\([^;{}]*\)\s*"
    r"(?:(?:const|volatile|noexcept|OVERRIDE)\s*|=\s*0\s*)*$",
    re.DOTALL)
_CPP_QUALIFIED_DIRECT_INIT = re.compile(
    r"::[A-Za-z_]\w*\s*\(\s*(?:0[xX][0-9A-Fa-f]+|[0-9]+|[A-Z][A-Z0-9_]{2,})"
    r"(?:\s*[,)]|[uUlL]+\s*[,)])")
_NAMESPACE_OPEN = re.compile(
    r"(?:^|[;{}])\s*(?:inline\s+)?namespace(?:\s+\w+(?:::\w+)*)?\s*$")
_EXTERN_BLOCK_OPEN = re.compile(r"(?:^|[;{}])\s*extern\s*$")


def _count_cpp_external_prototypes(code: str) -> int:
    code = re.sub(r"(?m)^\s*#.*$", "", code)
    contexts: list[bool] = []
    statement_start = 0
    total = 0
    for i, ch in enumerate(code):
        allowed = all(contexts)
        if ch == "{":
            prefix = code[statement_start:i]
            opens_decl_scope = bool(_NAMESPACE_OPEN.search(prefix)
                                    or _EXTERN_BLOCK_OPEN.search(prefix))
            contexts.append(allowed and opens_decl_scope)
            statement_start = i + 1
        elif ch == "}":
            if contexts:
                contexts.pop()
            statement_start = i + 1
        elif ch == ";":
            statement = code[statement_start:i]
            if (allowed and _CPP_PROTO.match(statement)
                    and not _CPP_QUALIFIED_DIRECT_INIT.search(statement)):
                total += 1
            statement_start = i + 1
    return total


_C_THIS_CAST = re.compile(r"\)this\b")
_CHAR_STAR_CAST = re.compile(r"(?<![\w>)])\((?:const |unsigned |signed )*char ?\*\)")
_NUMERIC_CAST = re.compile(
    r"(?<![\w>)])\((?:i8|i16|i32|i64|u8|u16|u32|u64|float|double|char|short"
    r"|int|long|unsigned)\)")
_REINTERPRET_CAST = re.compile(r"\breinterpret_cast\s*<")
_FORCED_COMDAT_EMITTER = re.compile(r"\bForceEmit\w*\b|\b\w+_OOL_(?:CTOR|DTOR)\b")


def _count_c_style_casts(code: str) -> int:
    return (len(_C_THIS_CAST.findall(code)) + _count_nonstring_m_casts(code)
            + len(_CHAR_STAR_CAST.findall(code))
            + len(_NUMERIC_CAST.findall(code))
            + _count_offset_macro_casts(code))


def _count_unexplained_casts(code: str) -> int:
    """The reinterpret_casts the cast ledger cannot account for. Takes RAW
    text (_NEEDS_RAW): the reasons live in comments."""
    from homm1.verify.casts import CAST, FORCED, REASON
    lines = code.split("\n")
    n = 0
    for i, line in enumerate(lines):
        for _ in CAST.finditer(line.split("//", 1)[0]):
            ctx = " ".join(lines[max(0, i - 3):i + 2])
            if any(re.search(pat, line) or re.search(pat, ctx)
                   for _nm, pat in FORCED):
                continue
            if REASON.search(ctx):
                continue
            n += 1
    return n


METRICS = (
    ("m_<hex> fields", re.compile(r"\bm_[0-9a-f]{2,}\b"), False),
    ("address-derived identifiers", _count_address_derived_identifiers, False),
    ("Unknown ids", re.compile(r"\b\w*[Uu]nknown\w*\b"), False),
    ("g_<hex> globals", re.compile(r"\bg_[0-9a-f]{4,}\b"), False),
    ("Method/Stub/FUN/Gap",
     re.compile(r"\b(?:(?:Method|Gap|Sub|Stub|Fwd|Func|FUN|Nullsub|Handler"
                r"|LogicHandler|winapi)_?[0-9a-f]{4,}|vfunc_[0-9]+)\b"), False),
    ("virtual slot placeholders",
     re.compile(r"\b(?:Slot[0-9]{1,2}_[0-9a-f]{4,}|Vfunc[0-9a-f]+"
                r"|Vtbl_[0-9a-f]{4,})\b"
                r"|\bvirtual\b[^;{\n]*?\bv[0-9]+\s*\("), False),
    ("positional arg placeholders",
     re.compile(r"\b(?:i8|u8|i16|u16|i32|u32|i64|u64|float|double|bool|char"
                r"|short|int|long|void"
                r"|[A-Z]\w*)\s*(?:\*\s*|&\s*)*\b(?:a|p|arg)[0-9]+\b"), False),
    ("placeholder classes", _count_placeholders, False),
    (".cpp-local views", _count_cpp_local_defs, True),
    ("placeholder vtable slots", _VTSLOT, False),
    ("*Vtbl structs", re.compile(r"\b(?:struct|class)\s+\w*Vtbl\w*"), False),
    ("->vtbl accesses", re.compile(r"->\s*\w*[Vv]tbl\w*"), False),
    ("g_*Vtbl globals", re.compile(r"\bg_\w*[Vv]tbl\w*"), False),
    ("m_vtbl/m_vptr members", re.compile(r"\bm_v(?:tbl|ptr)\w*"), False),
    ("magic case labels",
     re.compile(r"^[ \t]*case[ \t]+(?:0[xX][0-9a-fA-F]+|-?[0-9]+)[ \t]*:",
                re.M), False),
    ("unnamed domain compares",
     re.compile(r"[=!]=[ \t]*(?:0[xX](?!0\b|1\b)[0-9a-fA-F]+"
                r"|(?!0\b|1\b)[0-9]+)\b"), False),
    (".cpp-local enums",
     re.compile(r"\bGZ_ENUM_(?:BEGIN|BEGIN_SPLIT|CONST_BEGIN|FLAGS_BEGIN)\b"
                r"|^[ \t]*(?:typedef[ \t]+)?enum[ \t]+\w*[ \t]*\{", re.M), True),
    ("C-style casts", _count_c_style_casts, False),
    ("reinterpret_casts", _REINTERPRET_CAST, False),
    ("unexplained casts", _count_unexplained_casts, False),
    ("void* m_ members", re.compile(r"\bvoid ?\* m_"), False),
    ("offset-cast macros", _count_offset_macro_casts, False),
    ("forced COMDAT emitters", _FORCED_COMDAT_EMITTER, False),
    ("cpp extern decls", _CPP_EXTERN, True),
    ("cpp external prototypes", _count_cpp_external_prototypes, True),
)

#: the four manual-vtable idioms - the drove-to-0 class homm1.verify.bans
#: keeps at 0 (pulled from here so the guard and the score never drift).
BANNED_LABELS = ("*Vtbl structs", "->vtbl accesses", "g_*Vtbl globals",
                 "m_vtbl/m_vptr members")

_VIEW_METRICS = {"placeholder classes", ".cpp-local views",
                 "placeholder vtable slots", "*Vtbl structs",
                 "->vtbl accesses", "g_*Vtbl globals", "m_vtbl/m_vptr members"}

_SEMANTIC_LABELS = ("caller-callee FAKE-VIEW", "nested static_casts",
                    "truncated masks")
_SEMANTIC_LABEL_SET = set(_SEMANTIC_LABELS)

_RATCHET = _VIEW_METRICS | _SEMANTIC_LABEL_SET | {
    "magic case labels", "unnamed domain compares", ".cpp-local enums",
    "C-style casts", "unexplained casts",
    "forced COMDAT emitters", "cpp extern decls", "cpp external prototypes",
    "duplicate header externs", "positional arg placeholders",
    "m_<hex> fields", "address-derived identifiers",
}

_NEEDS_RAW = {"unexplained casts"}


def _is_scaffolding(path) -> bool:
    return path.name.endswith("Views.h")


def semantic_count() -> list[tuple[str, int]]:
    """The build/AST-derived metrics - full-tier work.

    `truncated masks` is deliberately absent (its instrument,
    mask_immediates, is PARKED with the campaign); the merge keeps its
    committed floor rather than blessing the absence."""
    values: dict[str, int] = {}
    try:
        from homm1.verify.casts import nested_static_casts
        values["nested static_casts"] = len(nested_static_casts())
    except Exception as exc:  # noqa: BLE001 - unmeasured, floor carried
        print(f"  board: nested static_casts UNMEASURED "
              f"({type(exc).__name__}: {exc}) - floor carried forward",
              file=sys.stderr)
    try:
        from homm1.verify.caller_callee import fake_view_count
        values["caller-callee FAKE-VIEW"] = fake_view_count()
    except Exception as exc:  # noqa: BLE001 - unmeasured, floor carried
        print(f"  board: caller-callee FAKE-VIEW UNMEASURED "
              f"({type(exc).__name__}: {exc}) - floor carried forward",
              file=sys.stderr)
    return [(lbl, values[lbl]) for lbl in _SEMANTIC_LABELS if lbl in values]


def count(*, include_semantic: bool = False) -> list[tuple[str, int]]:
    totals = {label: 0 for label, _, _ in METRICS}
    for root in ROOTS:
        base = REPO / root
        if not base.is_dir():
            continue
        for path in base.rglob("*"):
            if path.suffix not in EXTS or not path.is_file():
                continue
            is_cpp = path.suffix in _CPP
            scaffold = _is_scaffolding(path)
            try:
                raw = path.read_text(errors="ignore")
                code = _strip(raw)
            except OSError:
                continue
            for label, matcher, cpp_only in METRICS:
                if cpp_only and not is_cpp:
                    continue
                if scaffold and label in _VIEW_METRICS:
                    continue
                text = raw if label in _NEEDS_RAW else code
                totals[label] += matcher(text) if callable(matcher) \
                    else len(matcher.findall(text))
    rows = [(label, totals[label]) for label, _, _ in METRICS]
    vh = 0
    for root in ROOTS:
        base = REPO / root
        if not base.is_dir():
            continue
        for path in base.rglob("*Views.h"):
            try:
                vh += len(_TYPEDEF_DEF.findall(
                    _strip(path.read_text(errors="ignore"))))
            except OSError:
                pass
    rows.append(("view classes (*Views.h)", vh))
    rows.append(("duplicate header externs",
                 sum(len(v) - 1 for v in duplicate_header_externs().values())))
    if include_semantic:
        rows.extend(semantic_count())
    return rows


def _load_baseline_file(path) -> dict[str, int]:
    out: dict[str, int] = {}
    if not path.is_file():
        return out
    for line in path.read_text().splitlines():
        if "\t" in line:
            lbl, n = line.rsplit("\t", 1)
            try:
                out[lbl] = int(n)
            except ValueError:
                pass
    return out


def load_baseline() -> dict[str, int]:
    text = _load_baseline_file(TEXT_BASELINE)
    semantic = _load_baseline_file(SEMANTIC_BASELINE)
    duplicate = set(text) & set(semantic)
    if duplicate:
        raise ValueError("cleanliness metric occurs in both baselines: "
                         + ", ".join(sorted(duplicate)))
    return {**text, **semantic}


def save_baseline(rows: list[tuple[str, int]], *,
                  include_semantic: bool = True) -> None:
    text_rows = [(lbl, n) for lbl, n in rows if lbl not in _SEMANTIC_LABEL_SET]
    semantic_rows = [(lbl, n) for lbl, n in rows if lbl in _SEMANTIC_LABEL_SET]
    if text_rows or not TEXT_BASELINE.exists():
        TEXT_BASELINE.write_text("".join(f"{lbl}\t{n}\n"
                                         for lbl, n in text_rows))
    if include_semantic and semantic_rows:
        # preserve floors for semantic metrics this run did not measure
        base = _load_baseline_file(SEMANTIC_BASELINE)
        base.update(dict(semantic_rows))
        SEMANTIC_BASELINE.write_text(
            "".join(f"{lbl}\t{base[lbl]}\n" for lbl in base))


def gate(rows: list[tuple[str, int]] | None = None) -> list[str]:
    """Ratchet findings: every RATCHETED metric above its committed floor.

    FAIL-CLOSED: a ratcheted metric with no committed floor is a FINDING,
    not a pass. The floors are the whole gate - deleting the two baseline
    files used to make this tier permanently green, which is the one
    failure mode a ratchet must not have (`caller-callee` and
    `undefined-closure` already refuse to pass vacuously; this matches).
    """
    rows = rows if rows is not None else count()
    base = load_baseline()
    bad = []
    missing = sorted(lbl for lbl, _n in rows if lbl in _RATCHET and lbl not in base)
    if missing:
        bad.append(f"no committed floor for {len(missing)} ratcheted metric(s): "
                   f"{', '.join(missing[:6])} - a ratchet with no floor cannot "
                   f"fail; restore config/cleanliness/cleanliness-*-baseline.tsv "
                   f"or bless with `homm1 verify board --update`")
    for label, n in rows:
        if label in _RATCHET and label in base and n > base[label]:
            bad.append(f"{label}: {base[label]} -> {n} "
                       f"(+{n - base[label]}; ratcheted, never rises)")
    return bad


def report_lines(rows=None) -> list[str]:
    rows = rows if rows is not None else count()
    base = load_baseline()
    nz = [(lbl, n) for lbl, n in rows if n]
    zeros = len(rows) - len(nz)
    if not nz:
        return [f"cleanliness: all {len(rows)} metrics at 0 (clean)."]
    cells = []
    for lbl, n in nz:
        d = n - base[lbl] if lbl in base else 0
        cells.append(f"{lbl} {n}" + (f" ({d:+d})" if d else ""))
    lines = [f"cleanliness (non-zero of {len(rows)}; {zeros} at 0; "
             f"delta vs baseline, down = good):"]
    row = ""
    for c in cells:
        if row and len(row) + len(c) + 4 > 92:
            lines.append("    " + row.rstrip())
            row = ""
        row += c + "    "
    if row:
        lines.append("    " + row.rstrip())
    return lines


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 verify board", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 when a RATCHETED metric rose above its committed floor")
    ap.add_argument("--semantic", action="store_true",
                    help="also measure the build-derived rows (needs the Model/objs)")
    ap.add_argument("--update", action="store_true",
                    help="MANUAL bless: rewrite the committed floor files")
    ap.add_argument("--dup-externs", action="store_true",
                    help="list symbols declared `extern` in more than one header")
    a = ap.parse_args(argv)
    if a.dup_externs:
        dups = duplicate_header_externs()
        for name in sorted(dups, key=lambda k: (-len(dups[k]), k)):
            print(f"{name}  ({len(dups[name])} decls)")
            for path in dups[name]:
                print(f"    {path}")
        print(f"# {len(dups)} symbol(s), "
              f"{sum(len(v) - 1 for v in dups.values())} redundant decl(s)")
        return 0
    rows = count(include_semantic=a.semantic)
    if a.update:
        save_baseline(rows, include_semantic=a.semantic)
        print(f"cleanliness baseline updated ({len(rows)} metrics)")
        return 0
    for line in report_lines(rows):
        print(line)
    bad = gate(rows)
    for b in bad:
        print(f"cleanliness RATCHET VIOLATED: {b}", file=sys.stderr)
    if bad and a.gate:
        return 1
    if not bad:
        print("cleanliness: no ratcheted metric above its committed floor")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
