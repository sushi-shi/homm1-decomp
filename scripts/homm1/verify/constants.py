"""homm1.verify.constants - AST-backed bare numeric constant census.

This is deliberately a standalone audit rather than a default build tier: it
parses every project translation unit.  The report is derived under build/gen
and separates all genuinely numeric source spellings from the small set whose
context proves a semantic replacement:

  * integer zero implicitly converted to a pointer -> NULL
  * integer zero/one used as bool, Win32 BOOL, or project b32 -> false/true
  * integer equality whose other direct operand is an enum with one uniquely
    named value -> member

Everything else remains a review row.  AST-derived review groups separate call
arguments, bitwise/packing expressions, comparisons, arithmetic, array access,
and initializer payloads without claiming a semantic replacement.  In
particular, the scanner never calls an integer status, index, serialized width,
mask, table payload, or arithmetic identity a boolean merely because its value
is zero or one.

    homm1 verify constants             # census + derived TSV
    homm1 verify constants -v          # print every proven replacement
    homm1 verify constants --fix       # apply only compiler-proven replacements
    homm1 verify constants --gate      # nonzero while proven sites remain
"""

from __future__ import annotations

import argparse
import json
import multiprocessing
import re
from collections import Counter
from concurrent.futures import ProcessPoolExecutor
from dataclasses import asdict, dataclass
from pathlib import Path

from homm1.core.paths import BUILD, REPO
from homm1.verify.srcscan import blank_comments


CDB = BUILD / "clangd/compile_commands.json"
REPORT = BUILD / "gen/bare_constants.tsv"
_NUMBER = re.compile(rb"(?:0[xX][0-9A-Fa-f]+|[0-9]+)(?:[uUlL]*)(?![A-Za-z0-9_.])")
_SUFFIX = re.compile(r"[uUlL]+$")
_BOOLEAN_TYPE_SPELLINGS = {"BOOL", "b32"}
_LEGACY_BOOLEAN = re.compile(r"\b(?:FALSE|TRUE)\b")
_STRING = re.compile(r'"(?:\\.|[^"\\\n])*"')
_CHAR = re.compile(r"'(?:\\.|[^'\\\n])*'")
_SOURCE_EXTENSIONS = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".inl"}


@dataclass(frozen=True)
class Site:
    file: str
    line: int
    column: int
    offset: int
    function: str
    scope: str
    spelling: str
    value: int | None
    classification: str
    replacement: str
    context_type: str
    review_group: str
    review_context: str
    reason: str

    @property
    def proven(self) -> bool:
        return self.classification in {"null-pointer", "boolean", "enum"}


def _flags(entry: dict) -> list[str]:
    args = list(entry.get("arguments") or entry["command"].split())
    src = entry["file"]
    out = ["--driver-mode=cl"]
    for arg in args[1:]:
        if arg == "/c" or arg == src or arg.endswith(src):
            continue
        out.append(arg)
    return out


def _require_cl_mode(args: list[str]) -> None:
    if "--driver-mode=cl" not in args:
        raise RuntimeError("constant audit requires --driver-mode=cl; without "
                           "it libclang silently misreads /imsvc and returns "
                           "a false-low census")


def _source_path(entry: dict, repo: Path) -> Path:
    path = Path(entry["file"])
    if not path.is_absolute():
        path = Path(entry.get("directory") or repo) / path
    return path.resolve()


def _raw_number(path: Path, offset: int, cache: dict[Path, bytes]):
    raw = cache.setdefault(path, path.read_bytes())
    if offset < 0 or offset >= len(raw):
        return None
    if offset and (chr(raw[offset - 1]).isalnum() or raw[offset - 1] in b"_."):
        return None
    match = _NUMBER.match(raw, offset)
    if match is None:
        return None
    spelling = match.group().decode("ascii")
    body = _SUFFIX.sub("", spelling)
    try:
        value = int(body, 0)
    except ValueError:
        value = None
    return spelling, value


def _scope(cidx, stack) -> tuple[str, str]:
    function_kinds = {
        cidx.CursorKind.FUNCTION_DECL,
        cidx.CursorKind.CXX_METHOD,
        cidx.CursorKind.CONSTRUCTOR,
        cidx.CursorKind.DESTRUCTOR,
        cidx.CursorKind.CONVERSION_FUNCTION,
        cidx.CursorKind.FUNCTION_TEMPLATE,
    }
    function = next((node for node in reversed(stack)
                     if node.kind in function_kinds), None)
    if any(node.kind == cidx.CursorKind.ENUM_DECL for node in stack):
        return "named-enum-definition", function.spelling if function else ""
    if function is not None:
        return "function-body", function.displayname or function.spelling
    if any(node.kind == cidx.CursorKind.VAR_DECL for node in stack):
        return "data-initializer-or-extent", ""
    if any(node.kind == cidx.CursorKind.FIELD_DECL for node in stack):
        return "field-or-class-extent", ""
    return "other-declaration", ""


def _explicit_cast_ancestor(cidx, stack) -> bool:
    kinds = {
        cidx.CursorKind.CSTYLE_CAST_EXPR,
        cidx.CursorKind.CXX_STATIC_CAST_EXPR,
        cidx.CursorKind.CXX_REINTERPRET_CAST_EXPR,
        cidx.CursorKind.CXX_CONST_CAST_EXPR,
        cidx.CursorKind.CXX_DYNAMIC_CAST_EXPR,
        cidx.CursorKind.CXX_FUNCTIONAL_CAST_EXPR,
    }
    transparent = {
        cidx.CursorKind.PAREN_EXPR,
        cidx.CursorKind.UNEXPOSED_EXPR,
        cidx.CursorKind.UNARY_OPERATOR,
    }
    for node in reversed(stack):
        if node.kind in transparent:
            continue
        return node.kind in kinds
    return False


def _enum_values(cidx, root) -> dict[str, dict[int, set[str]]]:
    out: dict[str, dict[int, set[str]]] = {}
    for node in root.walk_preorder():
        if node.kind != cidx.CursorKind.ENUM_CONSTANT_DECL:
            continue
        parent = node.semantic_parent
        if parent is None or not parent.spelling:
            continue
        out.setdefault(parent.spelling, {}).setdefault(node.enum_value, set()).add(
            node.spelling)
    return out


def _same_cursor(left, right) -> bool:
    left_file = left.location.file
    right_file = right.location.file
    return (left.kind == right.kind
            and left_file is not None and right_file is not None
            and left_file.name == right_file.name
            and left.location.offset == right.location.offset)


def _comparison_sibling(cidx, literal, stack):
    """Return the other direct operand of an equality containing *literal*.

    Parentheses and Clang's implicit-expression wrappers do not change which
    operand owns the literal.  Any real expression on the path does: a zero in
    ``LoadConfig(kind) == 0`` is a call argument, not an equality operand.
    """
    transparent = {
        cidx.CursorKind.PAREN_EXPR,
        cidx.CursorKind.UNEXPOSED_EXPR,
    }
    for pos in range(len(stack) - 1, -1, -1):
        binary = stack[pos]
        if binary.kind != cidx.CursorKind.BINARY_OPERATOR:
            continue
        path = stack[pos + 1:]
        if any(node.kind not in transparent for node in path):
            return None
        if binary.spelling not in ("==", "!="):
            return None
        owner = path[0] if path else literal
        children = list(binary.get_children())
        if len(children) != 2:
            return None
        if _same_cursor(children[0], owner):
            return children[1]
        if _same_cursor(children[1], owner):
            return children[0]
        return None
    return None


def _direct_operand_type(cidx, operand):
    """Recover an operand's semantic type without entering its subexpressions."""
    transparent = {
        cidx.CursorKind.PAREN_EXPR,
        cidx.CursorKind.UNEXPOSED_EXPR,
    }
    node = operand
    while True:
        ty = node.type
        canonical = ty.get_canonical()
        if (canonical.kind in (cidx.TypeKind.ENUM, cidx.TypeKind.BOOL)
                or _type_spelling(ty) in _BOOLEAN_TYPE_SPELLINGS):
            return ty
        if node.kind not in transparent:
            return ty
        children = list(node.get_children())
        if len(children) != 1:
            return ty
        node = children[0]


def _binary_enum_context(cidx, literal, stack, enum_values):
    sibling = _comparison_sibling(cidx, literal, stack)
    if sibling is None:
        return None
    ty = _direct_operand_type(cidx, sibling)
    canonical = ty.get_canonical()
    if canonical.kind == cidx.TypeKind.ENUM:
        spelling = ty.spelling or canonical.spelling
        return spelling, enum_values.get(spelling, {})
    return None


def _binary_bool_context(cidx, literal, stack):
    sibling = _comparison_sibling(cidx, literal, stack)
    if sibling is None:
        return None
    ty = _direct_operand_type(cidx, sibling)
    if _semantic_type_role(cidx, ty) == "boolean":
        return ty.spelling or "bool"
    return None


def _type_spelling(ty) -> str:
    return re.sub(r"\b(?:const|volatile)\b", "", ty.spelling).strip()


def _semantic_type_role(cidx, ty):
    canonical = ty.get_canonical()
    if canonical.kind in (cidx.TypeKind.POINTER, cidx.TypeKind.MEMBERPOINTER):
        return "pointer"
    if (canonical.kind == cidx.TypeKind.BOOL
            or _type_spelling(ty) in _BOOLEAN_TYPE_SPELLINGS):
        return "boolean"
    return None


def _typed_value_classification(cidx, value, ty, null_available, reason):
    role = _semantic_type_role(cidx, ty)
    if value == 0 and role == "pointer":
        if not null_available:
            return ("numeric", "", ty.spelling,
                    "pointer context, but NULL is not visible in this TU")
        return "null-pointer", "NULL", ty.spelling, reason
    if value in (0, 1) and role == "boolean":
        return ("boolean", "true" if value else "false", ty.spelling or "bool",
                reason)
    return None


def _direct_value_path(cidx, node, literal) -> bool:
    """Whether *literal* is the value of *node*, not nested computation/input."""
    if _same_cursor(node, literal):
        return True
    if not _cursor_contains(node, literal):
        return False
    transparent = {
        cidx.CursorKind.PAREN_EXPR,
        cidx.CursorKind.UNEXPOSED_EXPR,
    }
    if node.kind in transparent:
        children = list(node.get_children())
        return len(children) == 1 and _direct_value_path(cidx, children[0], literal)
    if node.kind == cidx.CursorKind.CONDITIONAL_OPERATOR:
        children = list(node.get_children())
        if len(children) != 3 or _cursor_contains(children[0], literal):
            return False
        return any(_cursor_contains(branch, literal)
                   and _direct_value_path(cidx, branch, literal)
                   for branch in children[1:])
    return False


def _expected_semantic_context(cidx, literal, stack):
    """Return the expected type when the literal is a direct semantic value."""
    for pos in range(len(stack) - 1, -1, -1):
        node = stack[pos]
        if node.kind == cidx.CursorKind.CONDITIONAL_OPERATOR:
            if _direct_value_path(cidx, node, literal):
                return node.type, "conditional result"
            return None

        if node.kind == cidx.CursorKind.CALL_EXPR:
            args = list(node.get_arguments())
            target = node.referenced
            params = list(target.get_arguments()) if target is not None else []
            for index, arg in enumerate(args):
                if (_cursor_contains(arg, literal)
                        and _direct_value_path(cidx, arg, literal)):
                    if index < len(params):
                        return params[index].type, f"argument {index + 1} type"
                    return None
            return None

        if node.kind == cidx.CursorKind.RETURN_STMT:
            children = list(node.get_children())
            if len(children) != 1 or not _direct_value_path(cidx, children[0], literal):
                return None
            function_kinds = {
                cidx.CursorKind.FUNCTION_DECL,
                cidx.CursorKind.CXX_METHOD,
                cidx.CursorKind.CONVERSION_FUNCTION,
                cidx.CursorKind.FUNCTION_TEMPLATE,
            }
            function = next((owner for owner in reversed(stack[:pos])
                             if owner.kind in function_kinds), None)
            if function is not None:
                return function.result_type, "function return type"
            return None

        if node.kind in (cidx.CursorKind.VAR_DECL,
                          cidx.CursorKind.PARM_DECL,
                          cidx.CursorKind.FIELD_DECL):
            children = [child for child in node.get_children()
                        if _cursor_contains(child, literal)]
            if any(_direct_value_path(cidx, child, literal) for child in children):
                return node.type, "declared initializer type"
            return None

        if node.kind == cidx.CursorKind.BINARY_OPERATOR and node.spelling == "=":
            children = list(node.get_children())
            if (len(children) == 2 and _cursor_contains(children[1], literal)
                    and _direct_value_path(cidx, children[1], literal)):
                return children[0].type, "assignment target type"
            return None
    return None


def _classify(cidx, literal, stack, value, enum_values, null_available):
    if _explicit_cast_ancestor(cidx, stack):
        return "numeric", "", "", "explicit conversion is an ingest boundary"

    parent = stack[-1] if stack else None
    if parent is not None and parent.kind == cidx.CursorKind.UNEXPOSED_EXPR:
        typed = _typed_value_classification(
            cidx, value, parent.type, null_available, "implicit conversion")
        if typed is not None:
            return typed

    bool_type = _binary_bool_context(cidx, literal, stack)
    if value in (0, 1) and bool_type:
        return ("boolean", "true" if value else "false", bool_type,
                "equality with a bool operand")

    enum_context = _binary_enum_context(cidx, literal, stack, enum_values)
    if enum_context is not None and value is not None:
        enum_type, values = enum_context
        names = values.get(value, set())
        if len(names) == 1:
            name = next(iter(names))
            return ("enum", name, enum_type,
                    f"equality with {enum_type}; value has one enumerator")
        if len(names) > 1:
            return ("numeric", "", enum_type,
                    f"{enum_type} value has aliases: {', '.join(sorted(names))}")

    expected = _expected_semantic_context(cidx, literal, stack)
    if expected is not None:
        ty, reason = expected
        typed = _typed_value_classification(
            cidx, value, ty, null_available, reason)
        if typed is not None:
            return typed

    return "numeric", "", "", "no uniquely typed semantic replacement"


def _cursor_contains(outer, inner) -> bool:
    outer_file = outer.location.file
    inner_file = inner.location.file
    if outer_file is None or inner_file is None or outer_file.name != inner_file.name:
        return False
    return outer.extent.start.offset <= inner.location.offset < outer.extent.end.offset


def _call_argument_context(cidx, literal, stack):
    for node in reversed(stack):
        if node.kind != cidx.CursorKind.CALL_EXPR:
            continue
        args = list(node.get_arguments())
        for index, arg in enumerate(args):
            if not _cursor_contains(arg, literal):
                continue
            target = node.referenced
            name = ""
            if target is not None:
                name = target.displayname or target.spelling
            if not name:
                name = node.displayname or node.spelling or "<indirect-call>"
            return f"{name} argument {index + 1}"
    return None


def _nearest_expression_context(cidx, stack):
    bitwise = {"&", "|", "^", "<<", ">>"}
    comparison = {"==", "!=", "<", "<=", ">", ">="}
    arithmetic = {"+", "-", "*", "/", "%"}
    for node in reversed(stack):
        if node.kind == cidx.CursorKind.ARRAY_SUBSCRIPT_EXPR:
            return "array-index", "array subscript"
        if node.kind != cidx.CursorKind.BINARY_OPERATOR:
            continue
        if node.spelling in bitwise:
            return "bitwise-or-packing", f"operator {node.spelling}"
        if node.spelling in comparison:
            return "comparison-or-bound", f"operator {node.spelling}"
        if node.spelling in arithmetic:
            return "arithmetic", f"operator {node.spelling}"
    return None


def _review_group(cidx, literal, stack, scope, value, classification):
    if classification in {"null-pointer", "boolean", "enum"}:
        return "existing-symbol", classification
    if scope == "named-enum-definition":
        return "named-definition", "enumerator value"
    if scope in {"data-initializer-or-extent", "field-or-class-extent"}:
        return "data-or-extent", scope
    if any(node.kind == cidx.CursorKind.INIT_LIST_EXPR for node in stack):
        return "initializer-payload", "initializer list"

    call = _call_argument_context(cidx, literal, stack)
    if call is not None:
        return "call-argument", call
    if scope == "function-body" and value in (-1, 0, 1):
        return "trivial-function-literal", str(value)
    expression = _nearest_expression_context(cidx, stack)
    if expression is not None:
        return expression
    return "unresolved", "no narrower AST context"


def _scan_entry(payload):
    entry, repo_text = payload
    repo = Path(repo_text)
    import clang.cindex as cidx

    path = _source_path(entry, repo)
    args = _flags(entry)
    try:
        _require_cl_mode(args)
    except RuntimeError as exc:
        return [], f"{path}: {exc}"
    try:
        tu = cidx.Index.create().parse(
            str(path), args=args,
            options=cidx.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
    except cidx.TranslationUnitLoadError as exc:
        return [], f"{path}: libclang could not load TU: {exc}"
    errors = [d for d in tu.diagnostics if d.severity >= cidx.Diagnostic.Error]
    if errors:
        return [], f"{path}: parse error: {errors[0]}"

    enum_values = _enum_values(cidx, tu.cursor)
    null_available = any(
        node.kind == cidx.CursorKind.MACRO_DEFINITION and node.spelling == "NULL"
        for node in tu.cursor.get_children())
    cache: dict[Path, bytes] = {}
    sites: list[Site] = []

    def walk(node, stack=()):
        if node.kind == cidx.CursorKind.INTEGER_LITERAL and node.location.file:
            source = Path(node.location.file.name).resolve()
            try:
                rel = source.relative_to(repo)
            except ValueError:
                rel = None
            if rel is not None and rel.parts[0] in ("src", "include"):
                raw = _raw_number(source, node.location.offset, cache)
                if raw is not None:
                    spelling, value = raw
                    site_null_available = null_available
                    if source.suffix.lower() in (".h", ".hpp", ".inl"):
                        site_null_available = bool(
                            re.search(rb"\bNULL\b", cache[source]))
                    if stack and stack[-1].kind == cidx.CursorKind.UNARY_OPERATOR:
                        unary = "".join(tok.spelling
                                        for tok in stack[-1].get_tokens())
                        if unary.startswith("-") and value is not None:
                            value = -value
                            spelling = "-" + spelling
                    scope, function = _scope(cidx, stack)
                    cls, repl, context, reason = _classify(
                        cidx, node, stack, value, enum_values,
                        site_null_available)
                    review_group, review_context = _review_group(
                        cidx, node, stack, scope, value, cls)
                    sites.append(Site(
                        str(rel), node.location.line, node.location.column,
                        node.location.offset, function, scope, spelling, value,
                        cls, repl, context, review_group, review_context, reason))
        for child in node.get_children():
            walk(child, stack + (node,))

    walk(tu.cursor)
    return sites, None


def scan_entries(entries: list[dict], *, repo: Path = REPO, jobs: int = 1):
    payloads = [(entry, str(repo.resolve())) for entry in entries]
    rows: dict[tuple[str, int], Site] = {}
    errors: list[str] = []
    if jobs <= 1:
        results = map(_scan_entry, payloads)
    else:
        pool = ProcessPoolExecutor(max_workers=jobs)
        results = pool.map(_scan_entry, payloads)
    try:
        for sites, error in results:
            if error:
                errors.append(error)
                continue
            for site in sites:
                key = (site.file, site.offset)
                old = rows.get(key)
                unavailable = "NULL is not visible" in site.reason
                old_unavailable = old is not None and "NULL is not visible" in old.reason
                if old is None or unavailable or (site.proven and not old.proven
                                                   and not old_unavailable):
                    rows[key] = site
    finally:
        if jobs > 1:
            pool.shutdown()
    return sorted(rows.values(), key=lambda x: (x.file, x.offset)), errors


def scan(*, cdb: Path = CDB, repo: Path = REPO, jobs: int = 1):
    if not cdb.is_file():
        raise FileNotFoundError(f"{cdb}: no compile database; run homm1 configure")
    entries = json.loads(cdb.read_text())
    entries = [entry for entry in entries
               if Path(entry["file"]).suffix == ".cpp"
               and str(entry["file"]).replace("\\", "/").startswith("src/")]
    if not entries:
        raise RuntimeError(f"{cdb}: no project C++ translation units")
    return scan_entries(entries, repo=repo, jobs=jobs)


def write_report(path: Path, sites: list[Site]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fields = list(asdict(sites[0]).keys()) if sites else [
        "file", "line", "column", "offset", "function", "scope",
        "spelling", "value", "classification", "replacement",
        "context_type", "review_group", "review_context", "reason"]
    lines = ["\t".join(fields)]
    for site in sites:
        row = asdict(site)
        lines.append("\t".join("" if row[name] is None else str(row[name])
                               for name in fields))
    path.write_text("\n".join(lines) + "\n")


def findings(sites: list[Site]) -> list[str]:
    return [f"{site.file}:{site.line}:{site.column}: {site.spelling} -> "
            f"{site.replacement} ({site.reason})"
            for site in sites if site.proven]


def apply_proven(sites: list[Site], *, repo: Path = REPO) -> int:
    by_file: dict[str, list[Site]] = {}
    for site in sites:
        if site.proven:
            by_file.setdefault(site.file, []).append(site)
    applied = 0
    for rel, file_sites in sorted(by_file.items()):
        path = repo / rel
        raw = path.read_bytes()
        for site in sorted(file_sites, key=lambda item: item.offset, reverse=True):
            old = site.spelling.encode("ascii")
            replacement = site.replacement.encode("ascii")
            if raw[site.offset:site.offset + len(old)] != old:
                raise RuntimeError(
                    f"{rel}:{site.line}:{site.column}: source changed since scan")
            raw = raw[:site.offset] + replacement + raw[site.offset + len(old):]
            applied += 1
        path.write_bytes(raw)
    return applied


def legacy_boolean_spellings(*, repo: Path = REPO) -> list[str]:
    findings = []
    for root_name in ("include", "src"):
        root = repo / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if not path.is_file() or path.suffix not in _SOURCE_EXTENSIONS:
                continue
            code = blank_comments(path.read_text(errors="ignore"))
            code = _STRING.sub(lambda match: " " * len(match.group()), code)
            code = _CHAR.sub(lambda match: " " * len(match.group()), code)
            for match in _LEGACY_BOOLEAN.finditer(code):
                line = code.count("\n", 0, match.start()) + 1
                column = match.start() - code.rfind("\n", 0, match.start())
                rel = path.relative_to(repo)
                findings.append(
                    f"{rel}:{line}:{column}: {match.group()} -> "
                    f"{'true' if match.group() == 'TRUE' else 'false'}")
    return findings


def summary(sites: list[Site]) -> str:
    scopes = Counter(site.scope for site in sites)
    classes = Counter(site.classification for site in sites)
    values = Counter(site.value for site in sites
                     if site.scope == "function-body")
    nontrivial = [site for site in sites
                  if site.scope == "function-body" and site.value not in (-1, 0, 1)]
    nontrivial_groups = Counter(site.review_group for site in nontrivial)
    return (f"{len(sites)} bare numeric spelling(s): "
            f"{scopes['function-body']} function-body, "
            f"{scopes['data-initializer-or-extent']} data/extent, "
            f"{scopes['named-enum-definition']} named-enum, "
            f"{scopes['field-or-class-extent']} field/class; "
            f"function 0/1/-1={values[0]}/{values[1]}/{values[-1]}; "
            f"nontrivial review={len(nontrivial)} "
            f"(call {nontrivial_groups['call-argument']}, "
            f"bitwise {nontrivial_groups['bitwise-or-packing']}, "
            f"bound {nontrivial_groups['comparison-or-bound']}, "
            f"arithmetic {nontrivial_groups['arithmetic']}, "
            f"payload {nontrivial_groups['initializer-payload']}, "
            f"unresolved {nontrivial_groups['unresolved']}); "
            f"proven replacements={sum(site.proven for site in sites)} "
            f"(NULL {classes['null-pointer']}, bool {classes['boolean']}, "
            f"enum {classes['enum']})")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm1 verify constants",
                                     description=__doc__)
    parser.add_argument("--gate", action="store_true",
                        help="fail while any compiler-proven replacement remains")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="print every compiler-proven replacement")
    parser.add_argument("--fix", action="store_true",
                        help="apply every compiler-proven replacement")
    parser.add_argument("--no-report", action="store_true",
                        help="do not write build/gen/bare_constants.tsv")
    parser.add_argument("--jobs", type=int,
                        default=min(4, multiprocessing.cpu_count()),
                        help="parallel libclang workers (default: up to 4)")
    args = parser.parse_args(argv)
    try:
        sites, errors = scan(jobs=max(1, args.jobs))
    except (FileNotFoundError, RuntimeError) as exc:
        print(f"[constants] FATAL: {exc}")
        return 2
    if errors:
        for error in errors[:20]:
            print(f"   {error}")
        if len(errors) > 20:
            print(f"   ... and {len(errors) - 20} more")
        print(f"[constants] FATAL: {len(errors)} translation unit(s) did not parse")
        return 2
    if args.fix:
        try:
            applied = apply_proven(sites, repo=REPO)
        except (OSError, RuntimeError) as exc:
            print(f"[constants] FATAL: {exc}")
            return 2
        print(f"[constants] applied {applied} compiler-proven replacement(s)")
        return 0
    if not args.no_report:
        write_report(REPORT, sites)
    bad = findings(sites)
    legacy_booleans = legacy_boolean_spellings(repo=REPO)
    if args.verbose:
        for finding in bad + legacy_booleans:
            print(f"   {finding}")
    print(f"[constants] {summary(sites)}")
    print(f"[constants] legacy TRUE/FALSE spelling(s): {len(legacy_booleans)}")
    if not args.no_report:
        print(f"[constants] report: {REPORT.relative_to(REPO)}")
    if args.gate and (bad or legacy_booleans):
        print(f"[constants] FAIL: {len(bad)} compiler-proven replacement(s), "
              f"{len(legacy_booleans)} legacy boolean spelling(s) remain")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
