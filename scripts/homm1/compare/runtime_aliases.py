"""OLDNAMES aliases resolved to the C runtime functions they forward to.

VC4's OLDNAMES.LIB holds one member per pre-ANSI name. Each member is a COFF
WEAK EXTERNAL whose auxiliary default is the underscored runtime name:
`chdir` (`_chdir`) defaults to `_chdir` (`__chdir`), `strrev` to `_strrev`.
When no object defines the old name, LINK binds every reference to it to the
runtime function. A call spelled `chdir(...)` and one spelled `_chdir(...)`
therefore reach the same body in the linked image, but they carry different
relocation symbols in their objects.

A few defaults are not the function's own symbol but an assembler label at
the same address in the runtime member. LIBCMT's stricmp.obj defines the
function `__stricmp` (COFF type 0x20) and the untyped label `__strcmpi` at
the same `.text` offset, so OLDNAMES' `strcmpi` also reaches `__stricmp`.

`aliases()` reads both pinned libraries and returns `{alias: function}` for
every OLDNAMES name, composed through such a label. Comparison applies it to
undefined externals only, as LINK does: `normalize` proves that no compared
object defines one of the alias names strongly.
"""

from __future__ import annotations

import functools
import struct
from pathlib import Path

from homm1.compare import canonicalize as canon

OLDNAMES = "oldnames.lib"
#: The runtime LINK binds the defaults in (homm1.graph.link.CRT_LIBRARY).
RUNTIME = "libcmt.lib"


def _members(path: Path):
    """Yield the COFF bodies of a `!<arch>` library, skipping linker members."""
    data = path.read_bytes()
    if data[:8] != b"!<arch>\n":
        raise ValueError(f"{path}: not an archive")
    offset = 8
    while offset + 60 <= len(data):
        name = data[offset:offset + 16].decode("latin-1").strip()
        size = int(data[offset + 48:offset + 58].decode("latin-1").strip())
        body = data[offset + 60:offset + 60 + size]
        offset += 60 + size + (size & 1)
        if name in ("/", "//"):
            continue
        # OLDNAMES members carry machine 0 (IMAGE_FILE_MACHINE_UNKNOWN).
        if len(body) >= 20 and struct.unpack_from("<H", body, 0)[0] in (0, 0x14C):
            yield body


def _symbols(body: bytes) -> dict[int, canon.Symbol]:
    """The symbol table of one member (any machine), by symbol index."""
    pointer, count = struct.unpack_from("<II", body, 8)
    strings = pointer + count * canon.SYMBOL_SIZE
    found: dict[int, canon.Symbol] = {}
    index = 0
    while index < count:
        offset = pointer + index * canon.SYMBOL_SIZE
        raw = body[offset:offset + 8]
        if raw[:4] == bytes(4):
            start = strings + struct.unpack_from("<I", raw, 4)[0]
            name = body[start:body.index(b"\0", start)].decode("latin-1")
        else:
            name = raw.split(b"\0", 1)[0].decode("latin-1")
        value, section, typ, storage, aux = struct.unpack_from(
            "<IhHBB", body, offset + 8)
        found[index] = canon.Symbol(index, offset, name, value, section, typ,
                                    storage, aux)
        index += 1 + aux
    return found


def oldnames_defaults(path: Path) -> dict[str, str]:
    """{old name: default} for every weak external in OLDNAMES.LIB."""
    found: dict[str, str] = {}
    for body in _members(path):
        symbols = _symbols(body)
        for symbol in symbols.values():
            if (symbol.storage_class != canon.WEAK_EXTERNAL_STORAGE
                    or symbol.aux_count < 1):
                continue
            tag = struct.unpack_from(
                "<I", body, symbol.offset + canon.SYMBOL_SIZE)[0]
            default = symbols[tag].name
            previous = found.setdefault(symbol.name, default)
            if previous != default:
                raise ValueError(f"{path}: {symbol.name} defaults to both "
                                 f"{previous} and {default}")
    return found


def label_functions(path: Path, names: set[str]) -> dict[str, str]:
    """{label: function} for runtime labels in `names` that share their
    address with exactly one function-typed external of the same member."""
    found: dict[str, str] = {}
    for body in _members(path):
        defined = [s for s in _symbols(body).values()
                   if s.storage_class == canon.EXTERNAL_STORAGE and s.section > 0]
        for label in defined:
            if label.name not in names or label.typ & canon.FUNCTION_TYPE:
                continue
            functions = [s.name for s in defined
                         if s.typ & canon.FUNCTION_TYPE
                         and (s.section, s.value) == (label.section, label.value)]
            if len(functions) == 1:
                found[label.name] = functions[0]
    return found


@functools.cache
def aliases(lib_dir: Path | None = None) -> dict[str, str]:
    """{OLDNAMES name: runtime function symbol} from the pinned libraries."""
    if lib_dir is None:
        from homm1.core.paths import msvc_dir
        lib_dir = msvc_dir() / "lib"
    oldnames, runtime = Path(lib_dir) / OLDNAMES, Path(lib_dir) / RUNTIME
    for path in (oldnames, runtime):
        if not path.is_file():
            raise FileNotFoundError(
                f"{path}: pinned VC4 runtime library missing "
                "(install the VC4 toolchain under build/toolchains)")
    defaults = oldnames_defaults(oldnames)
    labels = label_functions(runtime, set(defaults.values()))
    resolved = {old: labels.get(default, default)
                for old, default in defaults.items()}
    resolved.update(labels)
    return resolved
