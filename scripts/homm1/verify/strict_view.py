"""homm1.verify.strict_view - every unit compiles in the strict-domain view.

`include/Domains.h` and `include/match.h` give each translation unit two
views. VC6 sees the integer expansion that the retail build compiles; clang-cl
with `/std:c++20 /Zc:__cplusplus` sees the strict view, where domains are
`enum class` types, typed storage converts only to and from its domain,
`H1_ENUM_ARRAY` subscripts accept only their domain and b8/b32 flags convert
only to and from bool. A strict-view error is a value crossing a domain the
source does not name.

The gate parses every unit of every image's compile database (the editor
compiles shared units with its own defines) with `clang-cl /Zs` in the strict
view and fails on any error. The committed floor in config/strict_view.floor
is the error count still allowed and only goes down (`--update-floor`).

    python3 -m homm1.verify.strict_view [--gate] [--list] [--update-floor]
                                        [--unit TEXT]
"""

from __future__ import annotations

import argparse
import collections
import json
import os
import re
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from homm1.core.paths import REPO

FLOOR = REPO / "config/strict_view.floor"
REPORT = "strict_view_errors.txt"
# `register` is the vendor codecs' C++98 spelling, not a domain question.
_STRICT = ["/std:c++20", "/Zc:__cplusplus", "/Zs", "/clang:-ferror-limit=0",
           "-Wno-delayed-template-parsing-in-cxx20", "-Wno-register"]
_ERROR = re.compile(r"^(?P<loc>.*?): error: (?P<message>.*)$")


def entries() -> list[tuple[str, dict]]:
    """(image, compile command) for every C++ unit of every image."""
    from homm1.core.paths import DEFAULT_IMAGE, image_build, images
    out = []
    for image in [DEFAULT_IMAGE, *[i for i in images() if i != DEFAULT_IMAGE]]:
        cdb = image_build(image) / "clangd/compile_commands.json"
        if not cdb.is_file():
            if image == DEFAULT_IMAGE:
                raise FileNotFoundError(f"{cdb}: no compile database; run homm1 configure")
            continue
        for entry in json.loads(cdb.read_text()):
            if Path(entry["file"]).suffix in (".c", ".cpp"):
                out.append((image, entry))
    return out


_LINE = re.compile(r'^\s*#\s*line\s+(\d+)(?:\s+(\S.*?))?\s*$')
_LOCATION = re.compile(r"^(?P<path>.*?)\((?P<line>\d+),(?P<column>\d+)\) ?: error: (?P<message>.*)$")
_SEGMENTS: dict[str, list[tuple[int, int, int, str]]] = {}


def _segments(source: Path) -> list[tuple[int, int, int, str, bool]]:
    """(first physical line, last physical line, logical first line, logical
    file basename, whether clang reports the physical path) for each `#line`
    region of a source file. Until a `#line` names a file, clang keeps the
    physical path; after one, it reports the retail path it names."""
    key = str(source)
    if key not in _SEGMENTS:
        out, name, physical = [], source.name.lower(), True
        start, logical = 1, 1
        try:
            lines = source.read_text(errors="replace").split("\n")
        except OSError:
            lines = []
        for index, text in enumerate(lines, 1):
            match = _LINE.match(text)
            if match:
                out.append((start, index - 1, logical, name, physical))
                start, logical = index + 1, int(match.group(1))
                spelled = match.group(2)
                if spelled and spelled.startswith('"'):
                    name = re.split(r"[\\/]+", spelled.strip('"'))[-1].lower()
                    physical = False
                elif spelled:
                    # A path macro (INPUTMGR_CPP_PATH) names the unit's own
                    # retail file.
                    name, physical = source.name.lower(), False
        out.append((start, len(lines), logical, name, physical))
        _SEGMENTS[key] = out
    return _SEGMENTS[key]


def _repo_location(line: str, unit: Path) -> str:
    """An error line with a `#line`-pinned retail path or line number mapped
    back to the repository file and physical line."""
    match = _LOCATION.match(line)
    if not match:
        return line
    path, number = match.group("path"), int(match.group("line"))
    candidate = Path(path)
    on_disk = candidate.is_file()
    if on_disk and not _segments(candidate.resolve())[1:]:
        try:
            path = str(candidate.resolve().relative_to(REPO))
        except ValueError:
            pass
        return f"{path}({number},{match.group('column')}): error: {match.group('message')}"
    base = re.split(r"[\\/]+", path)[-1].lower()
    sources = [candidate.resolve()] if on_disk else []
    sources += [unit] + sorted((REPO / "src").rglob(unit.name))
    for source in sources:
        for first, last, logical, name, physical in _segments(source):
            if (physical == on_disk and name == base
                    and logical <= number <= logical + (last - first)):
                shown_line = first + number - logical
                try:
                    shown = str(source.relative_to(REPO))
                except ValueError:
                    shown = str(source)
                return (f"{shown}({shown_line},{match.group('column')}): error: "
                        f"{match.group('message')}")
    return line


def _parse(item: tuple[str, dict]) -> tuple[str, str, list[str]]:
    from homm1.verify.constants import _flags
    image, entry = item
    directory = Path(entry.get("directory") or REPO)
    source = Path(entry["file"])
    source = source if source.is_absolute() else directory / source
    command = ["clang", *_flags(entry), *_STRICT, str(source.resolve())]
    run = subprocess.run(command, capture_output=True, text=True, cwd=directory)
    unit = source.resolve()
    errors = [_repo_location(line, unit) for line in run.stderr.splitlines()
              if _ERROR.match(line)]
    if run.returncode and not errors:
        errors = [f"{entry['file']}: error: clang exited {run.returncode}: "
                  f"{run.stderr.strip()[:200]}"]
    return image, entry["file"], errors


def scan(unit: str | None = None, jobs: int | None = None):
    """{(image, unit file): [error line]} and the distinct error sites."""
    items = [item for item in entries() if not unit or unit in item[1]["file"]]
    results: dict[tuple[str, str], list[str]] = {}
    with ThreadPoolExecutor(jobs or os.cpu_count() or 1) as pool:
        for image, name, errors in pool.map(_parse, items):
            results[(image, name)] = errors
    sites: dict[str, str] = {}
    for errors in results.values():
        for line in errors:
            sites.setdefault(line, line)
    return results, sorted(sites)


def read_floor() -> int | None:
    try:
        return int(FLOOR.read_text().split()[0])
    except (OSError, ValueError, IndexError):
        return None


def classes(sites: list[str]) -> collections.Counter:
    counter: collections.Counter = collections.Counter()
    for line in sites:
        match = _ERROR.match(line)
        if match:
            counter[re.sub(r"'[^']*'", "'X'", match.group("message"))] += 1
    return counter


def gate_findings() -> list[str]:
    _results, sites = scan()
    floor = read_floor() or 0
    if len(sites) > floor:
        return [f"strict-view errors: {len(sites)} > floor {floor}", *sites[:20]]
    return []


from homm1.core.usage import logged  # noqa: E402


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="homm1 verify strict-view", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 when the error count exceeds the committed floor")
    ap.add_argument("--list", action="store_true", help="print every distinct error")
    ap.add_argument("--unit", help="only units whose source path contains TEXT")
    ap.add_argument("--update-floor", action="store_true",
                    help="lower config/strict_view.floor to the current count")
    ap.add_argument("--jobs", type=int)
    args = ap.parse_args(argv)
    results, sites = scan(args.unit, args.jobs)
    from homm1.core.paths import IMAGE_BUILD
    report = IMAGE_BUILD / "gen" / REPORT
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text("".join(line + "\n" for line in sites))
    clean = sum(1 for errors in results.values() if not errors)
    print(f"[strict-view] {clean}/{len(results)} unit compile(s) parse in the strict view; "
          f"{len(sites)} distinct error(s) (report: {report})")
    for (image, name), errors in sorted(results.items()):
        if errors:
            print(f"   {len(errors):5d} {image}:{name}")
    for message, count in classes(sites).most_common(15):
        print(f"   {count:5d} {message}")
    if args.list:
        for line in sites:
            print(line)
    floor = read_floor()
    if args.unit:
        return 0
    if args.update_floor:
        if floor is not None and len(sites) > floor:
            print(f"[strict-view] FAIL: {len(sites)} exceeds the floor {floor}; not raised")
            return 1
        FLOOR.write_text(f"{len(sites)}\n")
        print(f"[strict-view] floor {floor} -> {len(sites)}")
        return 0
    if floor is None:
        print(f"[strict-view] FAIL: no floor in {FLOOR.relative_to(REPO)}")
        return 1 if args.gate else 0
    if len(sites) > floor:
        print(f"[strict-view] FAIL: {len(sites)} error(s) exceed the floor {floor}")
        return 1 if args.gate else 0
    if len(sites) < floor:
        print(f"[strict-view] below the floor {floor}: run --update-floor")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
