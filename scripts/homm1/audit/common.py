"""Shared Buka audit helpers extracted without their data-matching callers.

Donor: 299514f88900c0cf30ba03422c72830a38fc1cb7.
annotated_data: configure_libclang, _mask_lexical_noise.
bool_fields: project paths, database selection, portable driver helpers.
The retail argument adapter consumes HoMM1's measured compiler profile.
"""
from __future__ import annotations

import functools
import glob
import json
from pathlib import Path
import shlex
import subprocess
from typing import Iterable

import clang.cindex as ci

from homm1.clang_options import ClangMode

PROJECT_ROOTS = ("include", "src")
RETAIL_DATABASE = Path("build/analysis/strict/compile_commands.json")
PORTABLE_DATABASE = Path("build/compile_commands.json")


def configure_libclang() -> None:
    # Nix's Python package specifies its matching library. Buka's glob is only
    # a fallback; choosing another installed version can mismatch the binding.
    if ci.Config.library_path or ci.Config.library_file or ci.Config.loaded:
        return
    libraries = glob.glob("/nix/store/*clang*-lib/lib/libclang.so")
    if libraries:
        try:
            ci.Config.set_library_file(libraries[0])
        except Exception:
            pass



def _mask_lexical_noise(blob: bytes) -> bytes:
    out = bytearray(blob)
    index = 0
    state = "code"
    quote = 0
    while index < len(blob):
        byte = blob[index]
        following = blob[index + 1] if index + 1 < len(blob) else 0
        if state == "code":
            if byte == 47 and following == 47:
                out[index:index + 2] = b"  "; index += 2; state = "line"; continue
            if byte == 47 and following == 42:
                out[index:index + 2] = b"  "; index += 2; state = "block"; continue
            if byte in (34, 39):
                quote = byte; out[index] = 32; index += 1; state = "literal"; continue
        elif state == "line":
            if byte == 10:
                state = "code"
            else:
                out[index] = 32
            index += 1; continue
        elif state == "block":
            if byte == 42 and following == 47:
                out[index:index + 2] = b"  "; index += 2; state = "code"; continue
            if byte != 10:
                out[index] = 32
            index += 1; continue
        else:
            if byte == 92 and index + 1 < len(blob):
                out[index:index + 2] = b"  "; index += 2; continue
            if byte == quote:
                state = "code"
            if byte != 10:
                out[index] = 32
            index += 1; continue
        index += 1
    if state in ("block", "literal"):
        raise ValueError("unterminated source comment or literal")
    return bytes(out)



def _project_relative(path: str | Path | None, repo: Path) -> str | None:
    if path is None:
        return None
    try:
        relative = Path(path).resolve().relative_to(repo.resolve()).as_posix()
    except (OSError, ValueError):
        return None
    if relative.split("/", 1)[0] not in PROJECT_ROOTS:
        return None
    return relative



def _command_arguments(entry: dict) -> list[str]:
    arguments = entry.get("arguments")
    if arguments:
        return list(arguments)
    command = entry.get("command")
    if command:
        return shlex.split(command)
    raise RuntimeError(f"compilation database entry has no command: {entry.get('file', '<unknown>')}")



def _parse_driver_includes(stderr: str) -> tuple[str, ...]:
    collecting = False
    paths = []
    for raw in stderr.splitlines():
        line = raw.strip()
        if line == "#include <...> search starts here:":
            collecting = True
            continue
        if collecting and line == "End of search list.":
            break
        if collecting and line:
            suffix = " (framework directory)"
            if line.endswith(suffix):
                line = line[:-len(suffix)]
            paths.append(line)
    return tuple(paths)



@functools.lru_cache(maxsize=None)
def _compiler_system_includes(compiler: str,
                              target_options: tuple[str, ...]) -> tuple[str, ...]:
    result = subprocess.run(
        [compiler, *target_options, "-E", "-x", "c++", "-", "-v"],
        input="",
        text=True,
        capture_output=True,
        check=False,
    )
    includes = _parse_driver_includes(result.stderr)
    if result.returncode or not includes:
        detail = result.stderr.strip().splitlines()
        tail = detail[-1] if detail else f"exit status {result.returncode}"
        raise RuntimeError(f"could not query native compiler include paths: {tail}")
    return includes



def _portable_clang_args(repo: Path, entry: dict) -> list[str]:
    command = _command_arguments(entry)
    compiler = command[0]
    directory = Path(entry.get("directory", repo))
    args = ["-x", "c++", "-std=c++20", "-ferror-limit=0"]
    target_options = tuple(
        option for option in command[1:]
        if option in ("-m32", "-m64") or option.startswith("--target=")
    )

    path_options = {"-I", "-isystem", "-iquote", "-idirafter", "-include", "-imacros"}
    index = 1
    while index < len(command):
        option = command[index]
        if option in path_options:
            if index + 1 >= len(command):
                raise RuntimeError(f"missing argument after {option} in compilation database")
            value = Path(command[index + 1])
            args.extend((option, str(value if value.is_absolute() else directory / value)))
            index += 2
            continue
        if option.startswith("-I") and option != "-I":
            value = Path(option[2:])
            args.append("-I" + str(value if value.is_absolute() else directory / value))
        elif option.startswith(("-D", "-U", "--sysroot=", "--target=")):
            args.append(option)
        elif option in ("-m32", "-m64", "-pthread", "-fms-extensions"):
            args.append(option)
        index += 1

    for path in _compiler_system_includes(compiler, target_options):
        args.extend(("-isystem", path))
    args.extend(("-I", str(repo / "include"), "-I", str(repo)))
    return args



def _entries(
    repo: Path, filters: Iterable[str] = (), *, portable: bool = False
) -> list[dict]:
    relative_database = PORTABLE_DATABASE if portable else RETAIL_DATABASE
    database = repo / relative_database
    if not database.is_file():
        instruction = (
            "configure CMake with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`"
            if portable
            else "run `homm1 configure` inside `nix develop .#build`"
        )
        raise RuntimeError(f"{relative_database} not found; {instruction}")
    raw = json.loads(database.read_text())
    filters = tuple(value.lower() for value in filters)
    out = []
    for entry in raw:
        directory = Path(entry.get("directory", repo))
        source = (directory / entry.get("file", "")).resolve()
        try:
            relative = source.relative_to(repo.resolve()).as_posix()
        except ValueError as error:
            raise RuntimeError(f"compilation database entry is outside this worktree: {source}") from error
        if not source.is_file():
            raise RuntimeError(f"compilation database source is missing: {relative}")
        try:
            source.relative_to(repo.resolve() / "src")
        except ValueError:
            # Portable branches may compile bundled libraries and test tools
            # through the same CMake database. They are not reconstructed game
            # storage and do not belong to this source-tree audit.
            continue
        if filters and not any(value in relative.lower() for value in filters):
            continue
        out.append({**entry, "directory": str(directory)})
    return sorted(out, key=lambda row: row["file"])



def _clang_args(repo: Path, source: Path, *, mode: ClangMode) -> list[str]:
    """Use the existing VC4/profile adapter, not Buka's VC6/localization flags."""
    directory = "strict" if mode == ClangMode.STRICT else "retail"
    database = repo / "build/analysis" / directory / "compile_commands.json"
    if not database.is_file():
        raise RuntimeError("missing analysis database; run homm1 configure")
    for entry in json.loads(database.read_text()):
        path = (Path(entry["directory"]) / entry["file"]).resolve()
        if path == source.resolve():
            command = _command_arguments(entry)
            # HoMM1 emits driver, profile flags, absolute source, syntax-only.
            if command[-2:] != [str(path), "-fsyntax-only"]:
                raise RuntimeError(f"unexpected analysis command for {path}")
            return ["-x", "c++", *command[1:-2]]
    raise RuntimeError(f"source absent from analysis database: {source}")
