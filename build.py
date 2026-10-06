#!/usr/bin/env python3
"""Build HEROES.EXE with the Visual C++ 4.0 toolchain under Wine.

    python3 build.py [--toolchain DIR] [--icon-from HEROES.EXE] [--jobs N]

DIR holds vc40/ (CL, ML, LINK and the VC4 headers and libraries), wing10/ and
dx1/ (the WinG and DirectX 1 SDK files), as in the hash-pinned release the
flake fetches. Resources compile with llvm-rc and llvm-cvtres. The icon is a
retail asset: `--icon-from` extracts it from your HEROES.EXE; without it the
executable carries the menus and About box but no icon.
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
    def __init__(self, toolchain: Path):
        self.toolchain = toolchain
        self.bin = toolchain / "vc40" / "bin"
        includes = [ROOT / "include", *sorted(p for p in (ROOT / "vendor").iterdir() if p.is_dir()),
                    toolchain / "wing10" / "include", toolchain / "dx1" / "include",
                    toolchain / "vc40" / "include"]
        libraries = [OUT / "imports", toolchain / "wing10" / "lib", toolchain / "vc40" / "lib"]
        prefix = Path(os.environ.get("WINEPREFIX") or OUT / "wineprefix")
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
        result = subprocess.run(["wine", str(self.bin / tool), *arguments], cwd=cwd, env=self.env,
                                stdin=subprocess.DEVNULL, capture_output=True, text=True,
                                errors="replace")
        if not expect.exists():
            output = "\n".join((result.stdout + result.stderr).strip().splitlines()[-30:])
            raise SystemExit(f"{tool} failed for {expect.name}:\n{output}")


def compile_unit(wine: Wine, unit: dict) -> Path:
    source = ROOT / unit["source"]
    obj = OUT / "obj" / f"{unit['unit']}.obj"
    obj.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=OUT) as scratch:   # fresh vc40.pdb/.idb
        if source.suffix.lower() == ".asm":
            wine.run("ML.EXE", ["/nologo", "/c", f"/Fo{windows(obj)}", windows(source)],
                     Path(scratch), obj)
        else:
            wine.run("CL.EXE", [*unit["flags"], f"/Fo{windows(obj)}", windows(source)],
                     Path(scratch), obj)
    print(f"  {unit['unit']}", flush=True)
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


def resources(icon_from: Path | None) -> Path:
    script = (ROOT / "src/SOURCE/Heroes.rc").read_text()
    stage = OUT / "rsrc"
    stage.mkdir(parents=True, exist_ok=True)
    if icon_from:
        (stage / "heroes.ico").write_bytes(icon_group(icon_from))
    else:
        print("no --icon-from: building without the retail icon", flush=True)
        script = "\n".join(line for line in script.split("\n")
                           if "ICON" not in line.split('"')[0])
    (stage / "Heroes.rc").write_text(script)
    res, obj = stage / "heroes.res", stage / "heroes_res.obj"
    subprocess.run(["llvm-rc", "/fo", str(res), str(stage / "Heroes.rc")], check=True)
    subprocess.run(["llvm-cvtres", "/machine:x86", f"/out:{obj}", str(res)], check=True)
    return obj


def build() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--toolchain", type=Path, default=os.environ.get("HOMM1_TOOLCHAIN"))
    parser.add_argument("--icon-from", type=Path, help="your HEROES.EXE, for the icon")
    parser.add_argument("--jobs", type=int, default=min(8, os.cpu_count() or 1))
    args = parser.parse_args()
    if args.toolchain is None or not (args.toolchain / "vc40/bin/CL.EXE").is_file():
        parser.error("--toolchain (or HOMM1_TOOLCHAIN) must hold vc40/bin/CL.EXE; "
                     "`nix develop` supplies it")
    for tool in ("wine", "wineboot", "llvm-rc", "llvm-cvtres"):
        if shutil.which(tool) is None:
            parser.error(f"{tool} is required; `nix develop` supplies it")
    manifest = json.loads((ROOT / "build.json").read_text())
    OUT.mkdir(exist_ok=True)
    wine = Wine(args.toolchain.resolve())

    (OUT / "imports").mkdir(exist_ok=True)
    for stub in sorted((ROOT / "imports").glob("*.c")):
        import_library(wine, stub)
    print(f"compiling {len(manifest['units'])} units", flush=True)
    with ThreadPoolExecutor(args.jobs) as pool:
        objects = dict(zip((u["unit"] for u in manifest["units"]),
                           pool.map(lambda u: compile_unit(wine, u), manifest["units"])))
    rsrc = resources(args.icon_from)

    link = manifest["link"]
    library = OUT / link["library"]
    response = OUT / "library.rsp"
    response.write_text("\n".join(["/NOLOGO", f"/OUT:{windows(library)}",
                                   *[f'"{windows(objects[u])}"' for u in link["members"]]]) + "\n")
    wine.run("LINK.EXE", ["-lib", f"@{windows(response)}"], OUT, library)
    libraries = list(link["libraries"])
    libraries.insert(libraries.index(link["library_after"]) + 1, windows(library))
    executable = OUT / "HEROES.EXE"
    response = OUT / "link.rsp"
    response.write_text("\n".join([
        f"/OUT:{windows(executable)}", f"/MAP:{windows(OUT / 'HEROES.map')}", "/NOLOGO",
        *link["flags"], f"/DEF:{windows(ROOT / link['definition'])}", *libraries,
        *[f'"{windows(objects[u])}"' for u in link["objects"]], f'"{windows(rsrc)}"']) + "\n")
    wine.run("LINK.EXE", [f"@{windows(response)}"], OUT, executable)
    print(f"built {executable.relative_to(ROOT)}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(build())
