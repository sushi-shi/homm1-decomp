"""homm1.verify.constants - AST-backed bare numeric constant census.

This is deliberately a standalone audit rather than a default build tier: it
parses every project translation unit.  The report is derived under build/gen
and separates all genuinely numeric source spellings from the small set whose
context proves a semantic replacement:

  * integer zero implicitly converted to a pointer -> NULL
  * integer zero/one used as Win32 BOOL -> FALSE/TRUE (VC4 has no `bool`,
    `true` or `false`: C2065; Clang's C++ bool conditions are not a VC4 type)
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
    homm1 verify constants --gate      # nonzero while proven sites remain,
                                       # open constants exceed the floor or
                                       # config/constants.tsv has stale rows
    homm1 verify constants --list ADV  # open constants in files/owners with ADV
    homm1 verify constants --update-floor
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

from homm1.core.paths import BUILD, IMAGE_BUILD, REPO
from homm1.verify.srcscan import blank_comments


CDB = IMAGE_BUILD / "clangd/compile_commands.json"
REPORT = IMAGE_BUILD / "gen/bare_constants.tsv"
_NUMBER = re.compile(rb"(?:0[xX][0-9A-Fa-f]+|[0-9]+)(?:[uUlL]*)(?![A-Za-z0-9_.])")
_FLOAT = re.compile(rb"(?:[0-9]+\.[0-9]*|\.[0-9]+)(?:[eE][-+]?[0-9]+)?[fFlL]?"
                    rb"|[0-9]+[eE][-+]?[0-9]+[fFlL]?")
_SUFFIX = re.compile(r"[uUlL]+$")
_BOOLEAN_TYPE_SPELLINGS = {"BOOL"}
#: VC4 (MSVC 4.x) predates the bool keywords; spelling them is C2065.
_CXX_BOOLEAN = re.compile(r"\b(?:false|true)\b")
#: Clang's strict-domain view (Domains.h selects it by __cplusplus).
_STRICT_VIEW = ["/std:c++20", "/Zc:__cplusplus"]
#: Macro names a proven replacement may spell; each must be visible.
_NAMED_MACROS = frozenset(("NULL", "TRUE", "FALSE"))
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


def _raw_number(path: Path, offset: int, cache: dict[Path, bytes],
                floating: bool = False):
    raw = cache.setdefault(path, path.read_bytes())
    if offset < 0 or offset >= len(raw):
        return None
    if offset and (chr(raw[offset - 1]).isalnum() or raw[offset - 1] in b"_."):
        return None
    if floating:
        match = _FLOAT.match(raw, offset)
        return (match.group().decode("ascii"), None) if match else None
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
        parent = function.semantic_parent
        if parent is not None and parent.kind in (cidx.CursorKind.CLASS_DECL,
                                                  cidx.CursorKind.STRUCT_DECL):
            return "function-body", f"{parent.spelling}::{function.spelling}"
        return "function-body", function.spelling
    variable = next((node for node in reversed(stack)
                     if node.kind == cidx.CursorKind.VAR_DECL), None)
    if variable is not None:
        return "data-initializer-or-extent", variable.spelling
    record = next((node for node in reversed(stack)
                   if node.kind in (cidx.CursorKind.STRUCT_DECL,
                                    cidx.CursorKind.UNION_DECL,
                                    cidx.CursorKind.CLASS_DECL)), None)
    if any(node.kind == cidx.CursorKind.FIELD_DECL for node in stack):
        return "field-or-class-extent", record.spelling if record is not None else ""
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
        cidx.CursorKind.UNARY_OPERATOR,
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
    # Only Win32 BOOL is a boolean domain for VC4. Clang's `bool` (an if/while
    # condition, a logical operand) is int truthiness in the retail compiler.
    if _type_spelling(ty) in _BOOLEAN_TYPE_SPELLINGS:
        return "boolean"
    return None


def _boolean_classification(value, type_spelling, visible, reason):
    name = "TRUE" if value else "FALSE"
    if name not in visible:
        return ("numeric", "", type_spelling,
                f"BOOL context, but {name} is not visible in this TU")
    return "boolean", name, type_spelling, reason


def _typed_value_classification(cidx, value, ty, visible, reason,
                                enum_values=None):
    canonical = ty.get_canonical()
    if (enum_values is not None and value is not None
            and canonical.kind == cidx.TypeKind.ENUM):
        spelling = canonical.spelling.replace("enum ", "")
        names = enum_values.get(spelling, {}).get(value, set())
        if len(names) == 1:
            return ("enum", next(iter(names)), spelling,
                    f"{reason} is {spelling}; value has one enumerator")
    role = _semantic_type_role(cidx, ty)
    if value == 0 and role == "pointer":
        if "NULL" not in visible:
            return ("numeric", "", ty.spelling,
                    "pointer context, but NULL is not visible in this TU")
        return "null-pointer", "NULL", ty.spelling, reason
    if value in (0, 1) and role == "boolean":
        return _boolean_classification(value, ty.spelling, visible, reason)
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
        cidx.CursorKind.UNARY_OPERATOR,
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


def _case_subject_type(cidx, stack):
    """The type of the switch subject when the literal is a case label."""
    transparent = {cidx.CursorKind.PAREN_EXPR, cidx.CursorKind.UNEXPOSED_EXPR,
                   cidx.CursorKind.UNARY_OPERATOR}
    for pos in range(len(stack) - 1, -1, -1):
        node = stack[pos]
        if node.kind in transparent:
            continue
        if node.kind != cidx.CursorKind.CASE_STMT:
            return None
        switch = next((n for n in reversed(stack[:pos])
                       if n.kind == cidx.CursorKind.SWITCH_STMT), None)
        if switch is None:
            return None
        parts = list(switch.get_children())
        if not parts:
            return None
        subject = parts[0]
        while subject.kind in (cidx.CursorKind.UNEXPOSED_EXPR, cidx.CursorKind.PAREN_EXPR):
            inner = list(subject.get_children())
            if not inner:
                break
            subject = inner[0]
        return subject.type
    return None


def _classify(cidx, literal, stack, value, enum_values, visible):
    if _explicit_cast_ancestor(cidx, stack):
        return "numeric", "", "", "explicit conversion is an ingest boundary"

    parent = stack[-1] if stack else None
    if parent is not None and parent.kind == cidx.CursorKind.UNEXPOSED_EXPR:
        typed = _typed_value_classification(
            cidx, value, parent.type, visible, "implicit conversion")
        if typed is not None:
            return typed

    bool_type = _binary_bool_context(cidx, literal, stack)
    if value in (0, 1) and bool_type:
        return _boolean_classification(value, bool_type, visible,
                                       "equality with a BOOL operand")

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

    case_type = _case_subject_type(cidx, stack)
    if case_type is not None and value is not None:
        typed = _typed_value_classification(
            cidx, value, case_type, visible, "switch subject", enum_values)
        if typed is not None and typed[0] == "enum":
            return typed

    expected = _expected_semantic_context(cidx, literal, stack)
    if expected is not None:
        ty, reason = expected
        typed = _typed_value_classification(
            cidx, value, ty, visible, reason, enum_values)
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


def _expr_name(cidx, node) -> str:
    """A short name for an operand: a variable or member, a call's callee
    with (), an array's name with [], else ''."""
    while node is not None and node.kind in (cidx.CursorKind.UNEXPOSED_EXPR,
                                             cidx.CursorKind.PAREN_EXPR):
        inner = list(node.get_children())
        node = inner[0] if inner else None
    if node is None:
        return ""
    if node.kind in (cidx.CursorKind.MEMBER_REF_EXPR, cidx.CursorKind.DECL_REF_EXPR):
        return node.spelling
    if node.kind == cidx.CursorKind.CALL_EXPR:
        return f"{node.spelling}()" if node.spelling else ""
    if node.kind == cidx.CursorKind.ARRAY_SUBSCRIPT_EXPR:
        base = _subscript_base(cidx, node)
        return f"{base}[]" if base else ""
    if node.kind == cidx.CursorKind.UNARY_OPERATOR:
        inner = list(node.get_children())
        return _expr_name(cidx, inner[0]) if inner else ""
    return ""


def _other_operand(cidx, binary, literal) -> str:
    children = list(binary.get_children())
    if len(children) != 2:
        return ""
    other = children[1] if _cursor_contains(children[0], literal) else children[0]
    return _expr_name(cidx, other)


def _nearest_expression_context(cidx, stack, literal=None):
    bitwise = {"&", "|", "^", "<<", ">>"}
    comparison = {"==", "!=", "<", "<=", ">", ">="}
    arithmetic = {"+", "-", "*", "/", "%"}
    for pos in range(len(stack) - 1, -1, -1):
        node = stack[pos]
        if node.kind == cidx.CursorKind.ARRAY_SUBSCRIPT_EXPR:
            return "array-index", f"array subscript {_subscript_base(cidx, node)}".rstrip()
        if node.kind == cidx.CursorKind.CASE_STMT:
            switch = next((n for n in reversed(stack[:pos])
                           if n.kind == cidx.CursorKind.SWITCH_STMT), None)
            subject = ""
            if switch is not None:
                parts = list(switch.get_children())
                subject = _expr_name(cidx, parts[0]) if parts else ""
            return "case-label", f"switch on {subject}".rstrip()
        if node.kind == cidx.CursorKind.RETURN_STMT:
            return "return", "return value"
        if node.kind == cidx.CursorKind.COMPOUND_ASSIGNMENT_OPERATOR:
            children = list(node.get_children())
            target = _expr_name(cidx, children[0]) if children else ""
            return "arithmetic", f"compound assignment to {target}".rstrip()
        if node.kind == cidx.CursorKind.VAR_DECL:
            return "store", f"initializer of {node.spelling}"
        if node.kind != cidx.CursorKind.BINARY_OPERATOR:
            continue
        other = _other_operand(cidx, node, literal) if literal is not None else ""
        if node.spelling == "=":
            return "store", f"store to {other}".rstrip()
        if node.spelling in bitwise:
            return "bitwise-or-packing", f"operator {node.spelling} {other}".rstrip()
        if node.spelling in comparison:
            return "comparison-or-bound", f"operator {node.spelling} {other}".rstrip()
        if node.spelling in arithmetic:
            return "arithmetic", f"operator {node.spelling} {other}".rstrip()
    return None

def _subscript_base(cidx, node) -> str:
    """The name of the array an ARRAY_SUBSCRIPT_EXPR indexes (its member or
    variable), or '' when the base is not a plain name."""
    children = list(node.get_children())
    base = children[0] if children else None
    while base is not None and base.kind in (cidx.CursorKind.UNEXPOSED_EXPR,
                                             cidx.CursorKind.PAREN_EXPR,
                                             cidx.CursorKind.ARRAY_SUBSCRIPT_EXPR):
        inner = list(base.get_children())
        base = inner[0] if inner else None
    if base is not None and base.kind in (cidx.CursorKind.MEMBER_REF_EXPR,
                                          cidx.CursorKind.DECL_REF_EXPR):
        return base.spelling
    return ""


def _trivial_role(cidx, literal, stack) -> str:
    """The syntactic role of a 0/1/-1 in a function body, for review rows."""
    transparent = {cidx.CursorKind.PAREN_EXPR, cidx.CursorKind.UNEXPOSED_EXPR,
                   cidx.CursorKind.UNARY_OPERATOR}
    child = literal
    for node in reversed(stack):
        if node.kind in transparent:
            child = node
            continue
        kind = node.kind
        if kind == cidx.CursorKind.RETURN_STMT:
            return "return"
        if kind == cidx.CursorKind.CASE_STMT:
            return "case"
        if kind == cidx.CursorKind.VAR_DECL:
            return "initializer"
        if kind == cidx.CursorKind.FOR_STMT:
            parts = list(node.get_children())
            return "for-init" if parts and _cursor_contains(parts[0], literal) \
                else "for-bound"
        if kind == cidx.CursorKind.COMPOUND_ASSIGNMENT_OPERATOR:
            return "compound-assignment"
        if kind == cidx.CursorKind.BINARY_OPERATOR:
            op = node.spelling
            if op == "=":
                loop = next((n for n in reversed(stack)
                             if n.kind == cidx.CursorKind.FOR_STMT), None)
                if loop is not None:
                    parts = list(loop.get_children())
                    if parts and _cursor_contains(parts[0], literal):
                        return "for-init"
                return f"store {_other_operand(cidx, node, literal)}".rstrip()
            if op in ("==", "!="):
                return f"equality {_other_operand(cidx, node, literal)}".rstrip()
            if op in ("<", "<=", ">", ">="):
                return f"bound {_other_operand(cidx, node, literal)}".rstrip()
            if op in ("+", "-", "*", "/", "%"):
                return "arithmetic"
            if op in ("&", "|", "^", "<<", ">>"):
                return "bitwise"
            if op in ("&&", "||"):
                return "condition"
            return f"operator {op}"
        if kind == cidx.CursorKind.CONDITIONAL_OPERATOR:
            return "conditional"
        if kind == cidx.CursorKind.ARRAY_SUBSCRIPT_EXPR:
            return f"array-index {_subscript_base(cidx, node)}".rstrip()
        if kind in (cidx.CursorKind.IF_STMT, cidx.CursorKind.WHILE_STMT,
                    cidx.CursorKind.DO_STMT):
            return "condition"
        return kind.name.lower()
    return "unresolved"


def _review_group(cidx, literal, stack, scope, value, classification):
    if classification in {"null-pointer", "boolean", "enum"}:
        return "existing-symbol", classification
    if scope == "named-enum-definition":
        return "named-definition", "enumerator value"
    if scope in {"data-initializer-or-extent", "field-or-class-extent"}:
        lists = [node for node in stack if node.kind == cidx.CursorKind.INIT_LIST_EXPR]
        if not lists:
            return "data-or-extent", "array extent"
        outer = lists[0]
        items = list(outer.get_children())
        if len(items) == 1 and value == 0 and len(lists) == 1:
            return "data-or-extent", "zero fill"
        return "data-or-extent", "table entry"
    if any(node.kind == cidx.CursorKind.INIT_LIST_EXPR for node in stack):
        return "initializer-payload", "initializer list"

    call = _call_argument_context(cidx, literal, stack)
    if call is not None:
        return "call-argument", call
    if scope == "function-body" and value in (-1, 0, 1):
        return "trivial-function-literal", f"{value} {_trivial_role(cidx, literal, stack)}"
    expression = _nearest_expression_context(cidx, stack, literal)
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
    tu = None
    strict = False
    # The strict-enum view types domain-annotated storage, parameters and
    # returns with their enums, so literals meeting them can be named; a unit
    # that does not parse in it falls back to the retail view.
    for extra in (_STRICT_VIEW, []):
        try:
            candidate = cidx.Index.create().parse(
                str(path), args=args + extra,
                options=cidx.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
        except cidx.TranslationUnitLoadError as exc:
            return [], f"{path}: libclang could not load TU: {exc}"
        errors = [d for d in candidate.diagnostics if d.severity >= cidx.Diagnostic.Error]
        if not errors:
            tu = candidate
            strict = bool(extra)
            break
    if tu is None:
        return [], f"{path}: parse error: {errors[0]}"

    enum_values = _enum_values(cidx, tu.cursor)
    visible = frozenset(
        node.spelling for node in tu.cursor.get_children()
        if node.kind == cidx.CursorKind.MACRO_DEFINITION
        and node.spelling in _NAMED_MACROS)
    cache: dict[Path, bytes] = {}
    sites: list[Site] = []

    def walk(node, stack=()):
        if (node.kind in (cidx.CursorKind.INTEGER_LITERAL,
                          cidx.CursorKind.FLOATING_LITERAL)
                and node.location.file):
            source = Path(node.location.file.name).resolve()
            try:
                rel = source.relative_to(repo)
            except ValueError:
                rel = None
            if rel is not None and rel.parts[0] in ("src", "include"):
                raw = _raw_number(
                    source, node.location.offset, cache,
                    floating=node.kind == cidx.CursorKind.FLOATING_LITERAL)
                if raw is not None:
                    spelling, value = raw
                    site_visible = visible
                    if source.suffix.lower() in (".h", ".hpp", ".inl"):
                        # A header may be included before the macro exists.
                        site_visible = frozenset(
                            name for name in visible
                            if re.search(rb"\b" + name.encode() + rb"\b",
                                         cache[source]))
                    if stack and stack[-1].kind == cidx.CursorKind.UNARY_OPERATOR:
                        unary = "".join(tok.spelling
                                        for tok in stack[-1].get_tokens())
                        if unary.startswith("-"):
                            value = -value if value is not None else None
                            spelling = "-" + spelling
                    scope, function = _scope(cidx, stack)
                    cls, repl, context, reason = _classify(
                        cidx, node, stack, value, enum_values,
                        site_visible)
                    review_group, review_context = _review_group(
                        cidx, node, stack, scope, value, cls)
                    sites.append(Site(
                        str(rel), node.location.line, node.location.column,
                        node.location.offset, function, scope, spelling, value,
                        cls, repl, context, review_group, review_context, reason))
        for child in node.get_children():
            walk(child, stack + (node,))

    walk(tu.cursor)
    return sites, None if strict else f"RETAIL-VIEW {path.relative_to(repo)}"


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
            if error and error.startswith("RETAIL-VIEW "):
                RETAIL_VIEW_UNITS.append(error[len("RETAIL-VIEW "):])
                error = None
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
               if Path(entry["file"]).suffix in (".c", ".cpp")
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
            replacement = site.replacement.encode("ascii")
            start, digits = site.offset, site.spelling
            if digits.startswith("-"):
                # The literal's offset is its digits; the sign sits before it.
                digits = digits[1:]
                start = site.offset - 1
                while start > 0 and raw[start:start + 1] in (b" ", b"\t"):
                    start -= 1
                if raw[start:start + 1] != b"-":
                    raise RuntimeError(
                        f"{rel}:{site.line}:{site.column}: source changed since scan")
            end = site.offset + len(digits)
            if raw[site.offset:end] != digits.encode("ascii"):
                raise RuntimeError(
                    f"{rel}:{site.line}:{site.column}: source changed since scan")
            raw = raw[:start] + replacement + raw[end:]
            applied += 1
        path.write_bytes(raw)
    return applied


def cxx_boolean_spellings(*, repo: Path = REPO) -> list[str]:
    """`true`/`false` spellings: VC4 has no bool keywords (C2065), so the
    retail-era spelling of a BOOL value is the Win32 TRUE/FALSE macro."""
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
            for match in _CXX_BOOLEAN.finditer(code):
                line = code.count("\n", 0, match.start()) + 1
                column = match.start() - code.rfind("\n", 0, match.start())
                rel = path.relative_to(repo)
                findings.append(
                    f"{rel}:{line}:{column}: {match.group()} -> "
                    f"{'TRUE' if match.group() == 'true' else 'FALSE'} "
                    f"(VC4 has no bool keywords)")
    return findings


#: The work list: every numeric constant is open until it is spelled as a
#: name (an enumerator, a named macro, NULL, TRUE/FALSE) or a row here keeps
#: it numeric with the reason.
WORKLIST = REPO / "config/constants.tsv"

#: Units the last scan read in the retail view because the strict-domain view
#: does not parse (a literal meets an enum class, or a declaration disagrees
#: with its definition).
RETAIL_VIEW_UNITS: list[str] = []
OPEN_REPORT = IMAGE_BUILD / "gen/constants_open.tsv"
_WORKLIST_FIELDS = ("file", "owner", "spelling", "group", "detail", "reason")


@dataclass(frozen=True)
class Keep:
    line: int
    file: str
    owner: str
    spelling: str
    group: str
    detail: str
    reason: str

    def matches(self, site: Site) -> bool:
        from fnmatch import fnmatchcase
        return (fnmatchcase(site.file, self.file)
                and fnmatchcase(site.function, self.owner)
                and fnmatchcase(site.spelling, self.spelling)
                and fnmatchcase(site.review_group, self.group)
                and fnmatchcase(site.review_context, self.detail))


def load_worklist(path: Path = WORKLIST) -> tuple[list[Keep], int | None, list[str]]:
    """Kept rows, the committed floor of open constants, and format errors."""
    keeps: list[Keep] = []
    floor = None
    errors: list[str] = []
    if not path.is_file():
        return keeps, floor, errors
    for number, text in enumerate(path.read_text().splitlines(), 1):
        if not text.strip():
            continue
        if text.startswith("#"):
            parts = text[1:].split("\t")
            if parts[0].strip() == "floor" and len(parts) == 2:
                floor = int(parts[1])
            continue
        parts = text.split("\t")
        if len(parts) != len(_WORKLIST_FIELDS):
            errors.append(f"{path.name}:{number}: expected "
                          f"{len(_WORKLIST_FIELDS)} tab-separated fields")
            continue
        keep = Keep(number, *parts)
        if not keep.reason.strip() or keep.reason.strip() == "*":
            errors.append(f"{path.name}:{number}: a kept constant needs a reason")
            continue
        keeps.append(keep)
    return keeps, floor, errors


def is_counted(site: Site) -> bool:
    """Every numeric spelling counts except an enumerator's own value."""
    return site.scope != "named-enum-definition"


def open_sites(sites: list[Site], keeps: list[Keep]) -> tuple[list[Site], list[Keep]]:
    """Counted sites no row keeps, and the rows that keep nothing (stale)."""
    used: set[int] = set()
    out: list[Site] = []
    for site in sites:
        if not is_counted(site):
            continue
        keep = next((k for k in keeps if k.matches(site)), None)
        if keep is None:
            out.append(site)
        else:
            used.add(keep.line)
    return out, [k for k in keeps if k.line not in used]


def write_open_report(path: Path, sites: list[Site]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = ["\t".join(("file", "line", "owner", "spelling", "group", "detail",
                        "replacement"))]
    for site in sites:
        lines.append("\t".join((site.file, str(site.line), site.function,
                                site.spelling, site.review_group,
                                site.review_context, site.replacement)))
    path.write_text("\n".join(lines) + "\n")


def write_floor(path: Path, floor: int) -> None:
    text = path.read_text() if path.is_file() else ""
    lines = [line for line in text.splitlines() if not line.startswith("#floor")]
    head = [line for line in lines if line.startswith("#")]
    body = [line for line in lines if not line.startswith("#")]
    path.write_text("\n".join(head + [f"#floor\t{floor}"] + body) + "\n")


def open_summary(sites: list[Site]) -> str:
    groups = Counter(site.review_group for site in sites)
    detail = ", ".join(f"{name} {count}" for name, count in groups.most_common())
    return f"{len(sites)} open constant(s): {detail}"


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


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm1 verify constants",
                                     description=__doc__)
    parser.add_argument("--gate", action="store_true",
                        help="fail on proven replacements, true/false spellings, open "
                             "constants above the floor, or stale review rows")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="print every compiler-proven replacement")
    parser.add_argument("--fix", action="store_true",
                        help="apply every compiler-proven replacement")
    parser.add_argument("--no-report", action="store_true",
                        help="do not write build/gen/bare_constants.tsv")
    parser.add_argument("--list", metavar="FILTER", nargs="?", const="",
                        help="print the open constants whose file or owner "
                             "contains FILTER (all when empty)")
    parser.add_argument("--update-floor", action="store_true",
                        help="lower the committed floor to the current open count")
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
        keeps, _floor, _errors = load_worklist()
        remaining, _stale = open_sites(sites, keeps)
        try:
            applied = apply_proven(remaining, repo=REPO)
        except (OSError, RuntimeError) as exc:
            print(f"[constants] FATAL: {exc}")
            return 2
        print(f"[constants] applied {applied} compiler-proven replacement(s)")
        return 0
    keeps, floor, worklist_errors = load_worklist()
    remaining, stale = open_sites(sites, keeps)
    if not args.no_report:
        write_report(REPORT, sites)
        write_open_report(OPEN_REPORT, remaining)
    if args.list is not None:
        for site in remaining:
            if args.list in site.file or args.list in site.function:
                note = f" -> {site.replacement}" if site.replacement else ""
                print(f"{site.file}:{site.line}:{site.column}\t{site.function}\t"
                      f"{site.spelling}\t{site.review_group}\t"
                      f"{site.review_context}{note}")
    bad = findings(remaining)
    cxx_booleans = cxx_boolean_spellings(repo=REPO)
    if args.verbose:
        for finding in bad + cxx_booleans:
            print(f"   {finding}")
    print(f"[constants] {summary(sites)}")
    print(f"[constants] true/false spelling(s) (C2065 under VC4): {len(cxx_booleans)}")
    if not args.no_report:
        print(f"[constants] report: {REPORT.relative_to(REPO)}")
    if RETAIL_VIEW_UNITS:
        print(f"[constants] {len(RETAIL_VIEW_UNITS)} unit(s) read in the retail "
              f"view; the strict-domain view does not parse (-v lists them)")
    if args.verbose:
        for unit in sorted(RETAIL_VIEW_UNITS):
            print(f"   {unit}")
    print(f"[constants] {open_summary(remaining)}; floor "
          f"{floor if floor is not None else 'unset'} "
          f"({WORKLIST.relative_to(REPO)}: {len(keeps)} kept row(s))")
    if not args.no_report:
        print(f"[constants] open list: {OPEN_REPORT.relative_to(REPO)}")
    for error in worklist_errors:
        print(f"   {error}")
    for keep in stale:
        print(f"   {WORKLIST.name}:{keep.line}: keeps no constant (stale row)")
    if args.update_floor:
        if floor is None or len(remaining) < floor:
            write_floor(WORKLIST, len(remaining))
            print(f"[constants] floor -> {len(remaining)}")
        return 0
    failed = []
    if bad or cxx_booleans:
        failed.append(f"{len(bad)} compiler-proven replacement(s), "
                      f"{len(cxx_booleans)} true/false spelling(s)")
    if floor is not None and len(remaining) > floor:
        failed.append(f"open constants rose {floor} -> {len(remaining)}")
    if stale or worklist_errors:
        failed.append(f"{len(stale)} stale and {len(worklist_errors)} malformed "
                      f"work-list row(s)")
    if args.gate and failed:
        print(f"[constants] FAIL: {'; '.join(failed)}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
