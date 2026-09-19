"""Shared Clang VarDecl inventory for source ``DATA()`` definitions."""

from __future__ import annotations

import glob
import hashlib
import json
import os
import re
import tempfile
from dataclasses import dataclass
from pathlib import Path

from homm1.symbols.profile import source_unit

import clang.cindex as ci

from homm1.clang_options import ClangMode
from homm1.audit.common import configure_libclang, _mask_lexical_noise
from homm1.symbols.profile import _clang_args
from homm1.symbols.fixed_asm import claims as fixed_asm_claims


IMAGE_BASE = 0x400000
DATA_TOKEN = re.compile(rb"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")
INCLUDE_TOKEN = re.compile(r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', re.M)
INVENTORY_CACHE_SCHEMA = 2


@dataclass(frozen=True, order=True)
class AnnotatedDataDefinition:
    unit: str
    name: str
    qualified_name: str
    rva: int
    size: int
    location: str
    is_static: bool
    # The decorated linker name. The image is stripped, so a claim is only
    # usable by the delinker and the relocation audits when it carries the
    # spelling the compiler actually emits; MSVC decorates an internal-linkage
    # object as ``_name`` and an external one with the full ``?name@@3...``.
    symbol: str = ""








def _declaration_end(masked: bytes, start: int) -> int:
    depth = {40: 0, 91: 0, 123: 0}
    closing = {41: 40, 93: 91, 125: 123}
    for index in range(start, len(masked)):
        byte = masked[index]
        if byte in depth:
            depth[byte] += 1
        elif byte in closing:
            depth[closing[byte]] -= 1
        elif byte == 59 and not any(depth.values()):
            return index + 1
    raise ValueError("unterminated DATA declaration")


def _qualified_name(cursor) -> str:
    owners = []
    parent = cursor.semantic_parent
    owner_kinds = {
        ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL,
        ci.CursorKind.CLASS_TEMPLATE, ci.CursorKind.NAMESPACE,
    }
    while parent is not None and parent.kind in owner_kinds:
        if parent.spelling:
            owners.append(parent.spelling)
        parent = parent.semantic_parent
    return "::".join([*reversed(owners), cursor.spelling])


def definitions_for_file(path: Path, source_root: Path, repo: Path,
                         translation=None) -> list[AnnotatedDataDefinition]:
    """Bind every ``DATA()`` marker in one file to the object it defines.

    ``translation`` lets a caller that has already parsed the file hand its
    translation unit over instead of paying for a second parse; the marker
    binding below is the same either way.
    """
    path = path.resolve()
    blob = path.read_bytes()
    masked = _mask_lexical_noise(blob)
    markers = [(match, _declaration_end(masked, match.end()))
               for match in DATA_TOKEN.finditer(masked)]
    if not markers:
        return []
    tu = translation
    if tu is None:
        configure_libclang()
        index = ci.Index.create()
        # libclang offsets are UTF-8 byte offsets. Decoding as latin-1 would
        # re-encode non-ASCII comments and shift every later cursor.
        tu = index.parse(
            str(path),
            args=_clang_args(repo, path, mode=ClangMode.RETAIL_ANALYSIS),
                     options=ci.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
    variables = []
    for cursor in tu.cursor.walk_preorder():
        if cursor.kind != ci.CursorKind.VAR_DECL or not cursor.is_definition():
            continue
        if cursor.location.file is None or Path(str(cursor.location.file)).resolve() != path:
            continue
        variables.append(cursor)
    rows = []
    unit = source_unit(path, source_root, repo)
    for marker, end in markers:
        matches = [cursor for cursor in variables
                   if marker.start() <= cursor.extent.start.offset < end
                   and cursor.extent.end.offset <= end]
        if len(matches) != 1:
            line = blob.count(b"\n", 0, marker.start()) + 1
            raise ValueError(f"{path}:{line}: DATA marker covers {len(matches)} VarDecls")
        cursor = matches[0]
        size = cursor.type.get_size()
        if size <= 0:
            raise ValueError(f"{path}:{cursor.location.line}: incomplete DATA type")
        try:
            display = path.relative_to(repo)
        except ValueError:
            display = path.relative_to(source_root.resolve())
        marker_line = blob.count(b"\n", 0, marker.start()) + 1
        rows.append(AnnotatedDataDefinition(
            unit, cursor.spelling, _qualified_name(cursor),
            int(marker.group(1), 16) - IMAGE_BASE, size,
            f"{display.as_posix()}:{marker_line}",
            cursor.storage_class == ci.StorageClass.STATIC,
            cursor.mangled_name,
        ))
    return rows


def _source_dependencies(path: Path, include_roots: list[Path]) -> list[Path]:
    seen = set()
    stack = [path.resolve()]
    while stack:
        current = stack.pop()
        if current in seen:
            continue
        seen.add(current)
        try:
            text = current.read_text(errors="replace")
        except OSError:
            continue
        for include in INCLUDE_TOKEN.findall(text):
            for candidate in (current.parent / include,
                              *(root / include for root in include_roots)):
                if candidate.is_file():
                    stack.append(candidate.resolve())
                    break
    seen.discard(path.resolve())
    return sorted(seen)


def _inventory_cache_key(path: Path, unit: str, object_root: Path,
                         compile_database: bytes, include_roots: list[Path]) -> str | None:
    object_path = object_root / f"{unit}.obj"
    if not object_path.is_file():
        return None
    digest = hashlib.sha256()
    digest.update(f"annotated-data-v{INVENTORY_CACHE_SCHEMA}\0".encode("ascii"))
    digest.update(Path(__file__).read_bytes())
    digest.update(compile_database)
    digest.update(path.read_bytes())
    digest.update(object_path.read_bytes())
    for dependency in _source_dependencies(path, include_roots):
        digest.update(str(dependency).encode("utf-8"))
        digest.update(dependency.read_bytes())
    return digest.hexdigest()


def _load_inventory_cache(path: Path) -> dict:
    try:
        data = json.loads(path.read_text())
    except (FileNotFoundError, json.JSONDecodeError, OSError):
        return {}
    if data.get("schema") != INVENTORY_CACHE_SCHEMA:
        return {}
    entries = data.get("entries")
    return entries if isinstance(entries, dict) else {}


def _write_inventory_cache(path: Path, entries: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", prefix=f".{path.name}.",
                                     dir=path.parent, delete=False) as stream:
        temporary = Path(stream.name)
        json.dump({"schema": INVENTORY_CACHE_SCHEMA, "entries": entries},
                  stream, separators=(",", ":"))
    os.replace(temporary, path)


def source_definitions(source_root: Path, repo: Path, object_root: Path | None = None,
                       cache_path: Path | None = None) -> list[AnnotatedDataDefinition]:
    source_root = Path(source_root)
    repo = Path(repo)
    if object_root is None and source_root.resolve() == (repo / "src").resolve():
        candidate = repo / "build/objdiff/base"
        if candidate.is_dir():
            object_root = candidate
    if cache_path is None and object_root is not None:
        cache_path = repo / "build/gen/annotated_data_cache.json"
    object_root = Path(object_root) if object_root is not None else None
    cache_path = Path(cache_path) if cache_path is not None else None
    compile_path = repo / "build/analysis/retail/compile_commands.json"
    compile_database = compile_path.read_bytes() if compile_path.is_file() else b""
    include_roots = [repo / "include"]
    vendor = repo / "vendor"
    if vendor.is_dir():
        include_roots.extend(sorted(path for path in vendor.iterdir() if path.is_dir()))
    cached = _load_inventory_cache(cache_path) if cache_path is not None else {}
    retained = {}
    rows = []
    for path in sorted(source_root.rglob("*.cpp")):
        unit = source_unit(path, source_root, repo)
        key = (_inventory_cache_key(path, unit, object_root, compile_database, include_roots)
               if object_root is not None else None)
        entry = cached.get(unit) if key is not None else None
        if isinstance(entry, dict) and entry.get("key") == key:
            values = [AnnotatedDataDefinition(**row) for row in entry.get("rows", [])]
        else:
            values = definitions_for_file(path, source_root, repo)
        rows.extend(values)
        if key is not None:
            retained[unit] = {
                "key": key,
                "rows": [{field: getattr(row, field)
                          for field in AnnotatedDataDefinition.__dataclass_fields__}
                         for row in values],
            }
    if source_root.resolve() == (repo / "src").resolve():
        for unit, source, claim in fixed_asm_claims("data"):
            if (repo / source).is_file():
                name = claim.name.removeprefix("_")
                rows.append(AnnotatedDataDefinition(
                    unit=unit, name=name, qualified_name=name,
                    rva=claim.rva, size=claim.size, location=source,
                    is_static=True, symbol=claim.name,
                ))
    if cache_path is not None:
        _write_inventory_cache(cache_path, retained)
    identities = {(row.unit, row.rva) for row in rows}
    if len(identities) != len(rows):
        raise ValueError("duplicate DATA RVA within a translation unit")
    return rows
