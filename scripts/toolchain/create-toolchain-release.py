#!/usr/bin/env python3
"""Build the reproducible HoMM1 compiler bundle from preserved media.

The archive combines the pinned VC4 and Watcom 10.0a files in
``config/toolchains.json`` with MASM 6.11's ML.EXE/ML.ERR. Microsoft shipped
MASM separately; its first diskette is preserved by PCjs as a lossless CHS
JSON image, and its two KWAJ-compressed members are expanded with libmspack.

Run through ``scripts/toolchain/create-toolchain-release.nix``. Nothing from
the input media is checked into the repository.
"""

from __future__ import annotations

import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tarfile
import tempfile


def find_repo() -> Path:
    override = os.environ.get("HOMM1_DIR")
    if override:
        return Path(override).resolve()
    for parent in Path(__file__).resolve().parents:
        if (parent / "flake.nix").is_file():
            return parent
    raise SystemExit("cannot find the HoMM1 repository")


REPO = find_repo()
CONFIG = REPO / "config/toolchains.json"
RELEASE_EPOCH = 1760000000
MASM_DISK_MD5 = "bb1f36e70d67720fa63356010b07c992"


def log(message: str) -> None:
    print(f"[toolchain-release] {message}", flush=True)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run(*command: str) -> None:
    subprocess.run(command, check=True, stdout=subprocess.DEVNULL)


def media(variable: str, expected: str) -> Path:
    value = os.environ.get(variable)
    if not value:
        raise SystemExit(
            f"{variable} is unset; use create-toolchain-release.nix")
    path = Path(value).resolve()
    actual = sha256(path)
    if actual != expected:
        raise SystemExit(f"{variable} hashes {actual}, expected {expected}")
    return path


def extract_component(name: str, source: Path, destination: Path,
                      config: dict) -> None:
    extraction = destination.parent / f".{name}-media"
    paths = [entry["media_path"] for entry in config["files"].values()]
    run("7z", "x", "-y", f"-o{extraction}", str(source), *paths)
    for relative, entry in config["files"].items():
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(extraction / entry["media_path"], target)


def reconstruct_pcjs_disk(source: Path, output: Path) -> None:
    document = json.loads(source.read_text())
    info = document.get("imageInfo", {})
    geometry = (int(info.get("cylinders", 0)), int(info.get("heads", 0)),
                int(info.get("trackDefault", 0)),
                int(info.get("sectorDefault", 0)),
                int(info.get("diskSize", 0)))
    if geometry != (80, 2, 18, 512, 1_474_560):
        raise SystemExit("MASM 6.11 media has unexpected disk geometry")
    cylinders, heads, sectors_per_track, sector_size, disk_size = geometry
    image = bytearray(disk_size)
    seen: set[tuple[int, int, int]] = set()
    for cylinder_data in document.get("diskData", []):
        for head_data in cylinder_data:
            for sector in head_data:
                coordinate = (int(sector["c"]), int(sector["h"]),
                              int(sector["s"]))
                cylinder, head, number = coordinate
                if coordinate in seen or not (0 <= cylinder < cylinders
                        and 0 <= head < heads
                        and 1 <= number <= sectors_per_track):
                    raise SystemExit(f"invalid MASM disk sector {coordinate}")
                if int(sector["l"]) != sector_size:
                    raise SystemExit(f"invalid MASM sector size at {coordinate}")
                words = [int(value) & 0xffffffff
                         for value in sector.get("d", [])]
                word_count = sector_size // 4
                if not words or len(words) > word_count:
                    raise SystemExit(f"invalid MASM sector payload at {coordinate}")
                words.extend([words[-1]] * (word_count - len(words)))
                offset = ((cylinder * heads + head) * sectors_per_track
                          + number - 1) * sector_size
                image[offset:offset + sector_size] = struct.pack(
                    f"<{word_count}I", *words)
                seen.add(coordinate)
    if len(seen) != cylinders * heads * sectors_per_track:
        raise SystemExit("MASM disk image is incomplete")
    actual = hashlib.md5(image).hexdigest()
    if actual != MASM_DISK_MD5 or info.get("hash") != MASM_DISK_MD5:
        raise SystemExit(f"MASM disk hashes {actual}, expected {MASM_DISK_MD5}")
    output.write_bytes(image)


def expand_kwaj(source: Path, output: Path) -> None:
    library_path = os.environ.get("LIBMSPACK")
    if not library_path:
        raise SystemExit("LIBMSPACK is unset; use create-toolchain-release.nix")

    class KwajDecompressor(ctypes.Structure):
        pass

    pointer = ctypes.POINTER(KwajDecompressor)
    KwajDecompressor._fields_ = [
        ("open", ctypes.CFUNCTYPE(ctypes.c_void_p, pointer, ctypes.c_char_p)),
        ("close", ctypes.CFUNCTYPE(None, pointer, ctypes.c_void_p)),
        ("extract", ctypes.CFUNCTYPE(ctypes.c_int, pointer, ctypes.c_void_p,
                                     ctypes.c_char_p)),
        ("decompress", ctypes.CFUNCTYPE(ctypes.c_int, pointer, ctypes.c_char_p,
                                        ctypes.c_char_p)),
        ("last_error", ctypes.CFUNCTYPE(ctypes.c_int, pointer)),
    ]
    library = ctypes.CDLL(library_path)
    library.mspack_create_kwaj_decompressor.argtypes = [ctypes.c_void_p]
    library.mspack_create_kwaj_decompressor.restype = pointer
    library.mspack_destroy_kwaj_decompressor.argtypes = [pointer]
    decompressor = library.mspack_create_kwaj_decompressor(None)
    if not decompressor:
        raise SystemExit("libmspack could not create a KWAJ decompressor")
    try:
        result = decompressor.contents.decompress(
            decompressor, os.fsencode(source), os.fsencode(output))
    finally:
        library.mspack_destroy_kwaj_decompressor(decompressor)
    if result:
        raise SystemExit(f"libmspack failed to expand {source.name}: {result}")


def install_masm(source: Path, vc40: Path, work: Path) -> None:
    image = work / "MASM611-DISK1.img"
    tree = work / "masm611"
    reconstruct_pcjs_disk(source, image)
    run("7z", "x", "-y", f"-o{tree}", str(image),
        "BIN/ML.EX$", "BIN/ML.ER$")
    expand_kwaj(tree / "BIN/ML.EX$", vc40 / "bin/ML.EXE")
    expand_kwaj(tree / "BIN/ML.ER$", vc40 / "bin/ML.ERR")


def entries(config: dict) -> dict:
    result = dict(config["files"])
    result.update(config.get("release_files", {}))
    return result


def verify(root: Path, configs: dict) -> None:
    count = 0
    for name in ("vc40", "watcom10"):
        for relative, expected in entries(configs[name]).items():
            path = root / name / relative
            if not path.is_file():
                raise SystemExit(f"missing {name}/{relative}")
            actual = sha256(path)
            if actual != expected["sha256"]:
                raise SystemExit(
                    f"{name}/{relative} hashes {actual}, expected "
                    f"{expected['sha256']}")
            count += 1
    log(f"verified {count} pinned files")


def package(root: Path, output: Path) -> None:
    def normalize(info: tarfile.TarInfo) -> tarfile.TarInfo:
        info.uid = info.gid = 0
        info.uname = info.gname = ""
        info.mtime = RELEASE_EPOCH
        info.mode = 0o755 if info.isdir() else 0o644
        return info

    output.parent.mkdir(parents=True, exist_ok=True)
    with tarfile.open(output, "w:xz") as archive:
        for path in sorted(root.rglob("*")):
            archive.add(path, arcname=str(Path("toolchains") /
                        path.relative_to(root)), filter=normalize,
                        recursive=False)
    log(f"wrote {output}")
    log(f"archive SHA-256: {sha256(output)}")


def main() -> None:
    configs = json.loads(CONFIG.read_text())
    if len(sys.argv) > 1 and sys.argv[1] == "--check":
        if len(sys.argv) != 3:
            raise SystemExit("usage: create-toolchain-release.py --check ROOT")
        verify(Path(sys.argv[2]).resolve(), configs)
        return

    vc_media = media("MSVC40_MEDIA", configs["vc40"]["media"]["sha256"])
    watcom_media = media(
        "WATCOM10_MEDIA", configs["watcom10"]["media"]["sha256"])
    masm_media = Path(os.environ["MASM611_DISK1"]).resolve()
    output = Path(os.environ.get(
        "OUTPUT", REPO / "build/homm1-toolchain.tar.xz")).resolve()

    (REPO / "build").mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".toolchain-release-",
                                     dir=REPO / "build") as scratch_name:
        work = Path(scratch_name)
        root = work / "toolchains"
        log("extracting pinned Visual C++ 4.0 files")
        extract_component("vc40", vc_media, root / "vc40", configs["vc40"])
        log("extracting pinned Watcom 10.0a files")
        extract_component("watcom10", watcom_media, root / "watcom10",
                          configs["watcom10"])
        log("reconstructing MASM 6.11 from its preserved diskette")
        install_masm(masm_media, root / "vc40", work)
        verify(root, configs)
        package(root, output)


if __name__ == "__main__":
    main()
