#!/usr/bin/env python3
"""Build HEROES.EXE and the scenario editor EDITOR.EXE with the Visual C++ 6.0
SP5 toolchain under Wine.

    python3 build.py [--target game|editor|all] [--locale LANG] [--toolchain DIR]
                     [--icon-from EXE ...] [--jobs N] [--out DIR]

`--target` selects the program (build.json `targets`; default: the game).
The editor shares the BASE library and the kbwin, REQUEST and wingraph
sources with the game, compiled again with its own profiles and
HOMM1_EDITOR defined. DIR holds vc6/ (CL, ML, LINK and the VC6 headers and
libraries), wing10/ and dx1/ (the WinG and DirectX 1 SDK files), as in the
hash-pinned release the flake fetches. `--locale` selects the language
compiled into the program, one of locales/<LANG>.json (default: ru, the
retail program): its catalog resolves every `localization::Tr("id")` to
literals in the language's Windows code page under build/<LANG>/localized/,
and the program is build/<LANG>/HEROES.EXE or build/<LANG>/EDITOR.EXE
(`--out` replaces build/); the source files are never rewritten. Resources
compile with llvm-rc and llvm-cvtres in the language's resource language.
The icons are retail assets: `--icon-from` names your HEROES.EXE and/or
EDITOR.EXE (repeatable; each program takes the icon of the file with its
name); without it the program carries its menus and About box but no icon.
"""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "build"


def windows(path: Path) -> str:
    return "Z:" + str(Path(path).resolve()).replace("/", "\\")


class Wine:
    def __init__(self, toolchain: Path, compiler: str):
        self.toolchain = toolchain
        self.bin = toolchain / compiler / "bin"
        includes = [OUT / "localized/include", ROOT / "include", *sorted(p for p in (ROOT / "vendor").iterdir() if p.is_dir()),
                    toolchain / "wing10" / "include", toolchain / "dx1" / "include",
                    toolchain / compiler / "include"]
        libraries = [OUT / "imports", toolchain / "wing10" / "lib", toolchain / compiler / "lib"]
        self.includes = includes
        prefix = OUT.parent / "wineprefix"
        self.env = dict(os.environ, WINEPREFIX=str(prefix), WINEPATH=windows(self.bin),
                        INCLUDE=";".join(map(windows, includes)),
                        LIB=";".join(map(windows, libraries)),
                        WINEDEBUG=os.environ.get("WINEDEBUG", "-all"),
                        WINEDLLOVERRIDES=os.environ.get("WINEDLLOVERRIDES", "mscoree,mshtml="))
        if not (prefix / "drive_c").is_dir():
            prefix.mkdir(parents=True, exist_ok=True)
            print(f"wine prefix: {prefix}", flush=True)
            subprocess.run(["wineboot", "--init"], env=self.env, check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            subprocess.run(["wineserver", "--wait"], env=self.env, check=False)

    def run(self, tool: str, arguments: list[str], cwd: Path, expect: Path) -> None:
        expect.unlink(missing_ok=True)
        # The selected bundle and generated headers own this build.
        if tool == "CL.EXE":
            arguments = ["/X", *["/I" + windows(p) for p in self.includes], *arguments]
        result = subprocess.run(["wine", str(self.bin / tool), *arguments], cwd=cwd, env=self.env,
                                stdin=subprocess.DEVNULL, capture_output=True, text=True,
                                errors="replace")
        if not expect.exists():
            output = "\n".join((result.stdout + result.stderr).strip().splitlines()[-30:])
            raise SystemExit(f"{tool} failed for {expect.name}:\n{output}")


def compile_unit(wine: Wine, target: str, unit: dict) -> Path:
    source = OUT / "localized" / unit["source"]
    obj = OUT / target / "obj" / f"{unit['unit']}.obj"
    obj.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=OUT) as scratch:   # fresh vc60.pdb/.idb
        if source.suffix.lower() == ".asm":
            wine.run("ML.EXE", ["/nologo", "/c", f"/Fo{windows(obj)}", windows(source)],
                     Path(scratch), obj)
        else:
            wine.run("CL.EXE", [*unit["flags"], f"/Fo{windows(obj)}", windows(source)],
                     Path(scratch), obj)
    print(f"  {target}: {unit['unit']}", flush=True)
    return obj


def import_library(wine: Wine, stub: Path) -> None:
    directory = OUT / "imports"
    obj, dll = directory / f"{stub.stem}.obj", directory / f"{stub.stem}.dll"
    wine.run("CL.EXE", ["/nologo", "/c", f"/Fo{windows(obj)}", windows(stub)], directory, obj)
    definition = stub.with_suffix(".def")
    wine.run("LINK.EXE", ["/NOLOGO", "/DLL", "/NOENTRY", "/NODEFAULTLIB",
                          *([f"/DEF:{windows(definition)}"] if definition.exists() else []),
                          f"/OUT:{windows(dll)}", f"/IMPLIB:{windows(directory / stub.stem)}.lib",
                          windows(obj)], directory, directory / f"{stub.stem}.lib")


def icon_group(executable: Path) -> bytes:
    """The retail icon group as an .ico file."""
    data = executable.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    first = pe + 24 + struct.unpack_from("<H", data, pe + 20)[0]
    rsrc_rva = struct.unpack_from("<I", data, pe + 24 + 96 + 2 * 8)[0]
    sections = [struct.unpack_from("<IIII", data, first + i * 40 + 8) for i in range(count)]

    def offset(rva: int) -> int:
        for size, address, raw_size, raw in sections:
            if address <= rva < address + max(size, raw_size):
                return raw + rva - address
        raise SystemExit(f"{executable}: RVA 0x{rva:x} is outside every section")

    base = offset(rsrc_rva)

    def entries(directory: int):
        named, ids = struct.unpack_from("<HH", data, base + directory + 12)
        for i in range(named + ids):
            key, target = struct.unpack_from("<II", data, base + directory + 16 + 8 * i)
            yield key, target

    def leaves(type_id: int) -> dict[int, bytes]:
        found = {}
        for key, target in entries(0):
            if key == type_id and target & 0x80000000:
                for name, sub in entries(target & 0x7FFFFFFF):
                    for _language, leaf in entries(sub & 0x7FFFFFFF):
                        rva, size = struct.unpack_from("<II", data, base + leaf)
                        found[name] = data[offset(rva):offset(rva) + size]
        return found

    icons, groups = leaves(3), leaves(14)
    if not groups:
        raise SystemExit(f"{executable}: no icon group")
    group = next(iter(groups.values()))
    reserved, kind, n = struct.unpack_from("<HHH", group, 0)
    header = struct.pack("<HHH", reserved, kind, n)
    images, cursor = b"", 6 + 16 * n
    for i in range(n):
        entry = group[6 + 14 * i:6 + 14 * i + 14]
        image = icons[struct.unpack_from("<H", entry, 12)[0]]
        header += entry[:8] + struct.pack("<II", len(image), cursor)
        images += image
        cursor += len(image)
    return header + images


def load_catalog():
    """The validated catalog; every problem is reported at once."""
    from catalog import Catalog
    try:
        return Catalog.load(ROOT)
    except ValueError as error:
        raise SystemExit(f"locales/ is invalid (python3 catalog.py check):\n{error}")


def resources(target: str, script_path: str, icon_from: Path | None, locale: str) -> Path:
    script = load_catalog().render_resource((ROOT / script_path).read_text(), locale=locale)
    stage = OUT / target / "rsrc"
    stage.mkdir(parents=True, exist_ok=True)
    icon = next((line.split('"')[1] for line in script.split("\n")
                 if line.split('"')[0].split()[1:2] == ["ICON"] and '"' in line), None)
    if icon_from and icon:
        (stage / icon).write_bytes(icon_group(icon_from))
    else:
        print(f"{target}: no --icon-from: building without the retail icon", flush=True)
        script = "\n".join(line for line in script.split("\n")
                           if "ICON" not in line.split('"')[0])
    rc = stage / Path(script_path).name
    rc.write_text(script)
    res, obj = stage / f"{target}.res", stage / f"{target}_res.obj"
    subprocess.run(["llvm-rc", "/fo", str(res), str(rc)], check=True)
    subprocess.run(["llvm-cvtres", "/machine:x86", f"/out:{obj}", str(res)], check=True)
    return obj


def prepare_sources(locale: str):
    catalog = load_catalog()
    for directory in ("src", "include", "vendor"):
        for source in (ROOT / directory).rglob("*"):
            if not source.is_file():
                continue
            target = OUT / "localized" / source.relative_to(ROOT)
            target.parent.mkdir(parents=True, exist_ok=True)
            if source.suffix in (".cpp", ".h", ".c", ".hpp", ".inc"):
                text = catalog.render(source.read_text(), locale=locale, expanded=True)
                target.write_text(text)
            else:
                shutil.copyfile(source, target)


def link(wine: Wine, target: str, contract: dict, objects: dict[str, Path], rsrc: Path) -> Path:
    """Archive the BASE members, then link the program in retail object order."""
    line = contract["link"]
    library = OUT / target / line["library"]
    response = OUT / target / "library.rsp"
    response.write_text("\n".join(["/NOLOGO", f"/OUT:{windows(library)}",
                                   *[f'"{windows(objects[u])}"' for u in line["members"]]]) + "\n")
    wine.run("LINK.EXE", ["-lib", f"@{windows(response)}"], OUT, library)
    libraries = list(line["libraries"])
    libraries.insert(libraries.index(line["library_after"]) + 1, windows(library))
    executable = OUT / contract["executable"]
    response = OUT / target / "link.rsp"
    response.write_text("\n".join([
        f"/OUT:{windows(executable)}",
        f"/MAP:{windows(executable.with_suffix('.map'))}", "/NOLOGO",
        *line["flags"], *libraries,
        *[f'"{windows(objects[u])}"' for u in line["objects"]], f'"{windows(rsrc)}"']) + "\n")
    wine.run("LINK.EXE", [f"@{windows(response)}"], OUT, executable)
    return executable


def icons(given: list[Path], targets: dict[str, dict], parser) -> dict[str, Path]:
    """{target: retail executable}: each file serves the program of its name."""
    chosen = {}
    for path in given:
        target = next((key for key, contract in targets.items()
                       if contract["executable"].lower() == path.name.lower()), None)
        if target is None:
            parser.error(f"--icon-from {path}: name it as one of "
                         + ", ".join(c["executable"] for c in targets.values()))
        if not path.is_file():
            parser.error(f"--icon-from {path}: no such file")
        chosen[target] = path
    return chosen


def build() -> int:
    global OUT
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    manifest = json.loads((ROOT / "build.json").read_text())
    compiler, targets = manifest["compiler"], manifest["targets"]
    parser.add_argument("--target", default=manifest["default_target"],
                        choices=[*targets, "all"],
                        help="the program to build (default: %(default)s)")
    parser.add_argument("--toolchain", type=Path, default=os.environ.get("HOMM1_TOOLCHAIN"))
    parser.add_argument("--icon-from", type=Path, action="append", default=[],
                        metavar="EXE", help="your HEROES.EXE or EDITOR.EXE, for the "
                                            "program's icon (repeatable)")
    parser.add_argument("--jobs", type=int, default=min(8, os.cpu_count() or 1))
    parser.add_argument("--out", type=Path, default=ROOT / "build",
                        help="output directory (default: build/)")
    parser.add_argument("--locale", default=manifest.get("locale", "ru"),
                        choices=sorted(p.stem for p in (ROOT / "locales").glob("*.json")),
                        help="the language (locales/LANG.json; default: %(default)s)")
    args = parser.parse_args()
    OUT = args.out.resolve() / args.locale
    if args.toolchain is None or not (args.toolchain / compiler / "bin/CL.EXE").is_file():
        parser.error(f"--toolchain (or HOMM1_TOOLCHAIN) must hold {compiler}/bin/CL.EXE; "
                     "`nix develop` supplies it")
    for tool in ("wine", "wineboot", "llvm-rc", "llvm-cvtres"):
        if shutil.which(tool) is None:
            parser.error(f"{tool} is required; `nix develop` supplies it")
    chosen = list(targets) if args.target == "all" else [args.target]
    icon_of = icons(args.icon_from, {key: targets[key] for key in chosen}, parser)
    OUT.mkdir(parents=True, exist_ok=True)
    prepare_sources(args.locale)
    wine = Wine(args.toolchain.resolve(), compiler)

    (OUT / "imports").mkdir(exist_ok=True)
    for stub in sorted((ROOT / "imports").glob("*.c")):
        import_library(wine, stub)
    for target in chosen:
        contract = targets[target]
        units = contract["units"]
        print(f"{target}: compiling {len(units)} units", flush=True)
        with ThreadPoolExecutor(args.jobs) as pool:
            objects = dict(zip((u["unit"] for u in units),
                               pool.map(lambda u: compile_unit(wine, target, u), units)))
        rsrc = resources(target, contract["resources"], icon_of.get(target), args.locale)
        print(f"built {link(wine, target, contract, objects, rsrc)}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(build())
