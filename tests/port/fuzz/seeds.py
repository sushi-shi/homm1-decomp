#!/usr/bin/env python3
"""Writes seed corpora for the fuzz harnesses from the game data.

    tests/port/fuzz/seeds.py GAME_DATA OUT

creates OUT/<harness>/ for each harness in tests/port/fuzz: the shipped
maps, campaign maps, saved games and high score tables (with the byte of
set-up choices fuzz_map and fuzz_savegame expect after the file), the help
book if the game data or $HOMM1_HELP has it, and the
resource archive's entries in fuzz_resources' input format (a kind byte, then
the payload). Archive entries are classified by their content, since the
archive stores only name hashes. The seeds are game data: keep them out of the
repository.
"""

import os
import struct
import sys
from pathlib import Path

# fuzz_resources' kinds.
ICON, BITMAP, TILESET, PALETTE, FONT, SAMPLE, ARCHIVE, CURSOR = range(8)


def find(root, *parts):
    """A path under root, matching each component case-insensitively."""
    path = Path(root)
    for part in parts:
        matches = [entry for entry in path.iterdir() if entry.name.lower() == part.lower()]
        if not matches:
            return None
        path = matches[0]
    return path


def files(folder, suffixes):
    if folder is None:
        return []
    return sorted(entry for entry in folder.iterdir()
                  if entry.is_file() and any(entry.name.upper().endswith(s) for s in suffixes))


def make_file_id(name):
    """MAKEFILEID (src/BASE/MAKEFILEID.cpp)."""
    file_id = 0
    for char in name.upper().encode("ascii"):
        high = file_id >> 8
        file_id = ((file_id << 8) | high) & 0xFFFF
        if file_id & 0x8000:
            file_id = ((file_id << 1) | 1) & 0xFFFF
        else:
            file_id = (file_id << 1) & 0xFFFF
        file_id = (file_id - char) & 0xFFFF
    return file_id


def remote_crc(data):
    """calc_crc (src/SOURCE/REMOTEREC.cpp): CRC-16, polynomial 0x1021."""
    crc = 0
    for byte in data:
        mask = 0x80
        while mask:
            overflow = crc & 0x8000
            crc = ((crc << 1) & 0xFFFF) | (1 if byte & mask else 0)
            if overflow:
                crc ^= 0x1021
            mask >>= 1
    return crc


def archive(entries):
    """An archive of (id, payload) entries."""
    out = bytearray(struct.pack("<H", len(entries)))
    offset = 2 + 10 * len(entries)
    for file_id, payload in entries:
        out += struct.pack("<HiI", file_id, offset, len(payload))
        offset += len(payload)
    for _, payload in entries:
        out += payload
    return bytes(out)


def classify(payload):
    size = len(payload)
    if size == 768:
        return PALETTE
    if size == 17:
        return FONT
    if size >= 6:
        a, b, c = struct.unpack_from("<hhh", payload)
        count, length = struct.unpack_from("<hi", payload)
        if count > 0 and length == size - 6:
            return ICON
        if a == 0x21 and b * c + 6 == size:
            return BITMAP
        if a > 0 and a * b * c + 6 == size:
            return TILESET
    return None


def write(folder, name, data):
    folder.mkdir(parents=True, exist_ok=True)
    (folder / name).write_bytes(data)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    root, out = Path(sys.argv[1]), Path(sys.argv[2])
    maps = files(find(root, "MAPS"), [".MAP", ".CMP"])
    games = files(find(root, "GAMES"), [".GM1", ".GM2", ".GM3", ".GM4", ".CGM"])
    data = find(root, "DATA")
    extra_games = [path for path in (find(root, "DATA", "REMOTE.GAM"), find(root, "DATA", "ORIGDATA.BIN"))
                   if path is not None]
    scores = files(data, [".HS"])

    # fuzz_map and fuzz_savegame inputs end with a byte of set-up choices.
    for path in maps:
        data = path.read_bytes()
        for harness in ("fuzz_editor_map", "fuzz_records"):
            write(out / harness, path.name, data)
        write(out / "fuzz_map", path.name, data + bytes([0x05]))
        stem = path.stem.upper()
        if stem.startswith("CAMP") and stem[4:].isdigit():
            # Its own campaign scenario: bit 7, scenario in bits 2-6.
            write(out / "fuzz_map", path.name + ".campaign", data + bytes([0x80 | (int(stem[4:]) - 1) << 2]))
    for path in games + extra_games:
        data = path.read_bytes()
        for harness in ("fuzz_records", "fuzz_lzhuf"):
            write(out / harness, path.name, data)
        write(out / "fuzz_savegame", path.name, data + bytes([0x00]))
    for path in scores:
        for harness in ("fuzz_records", "fuzz_lzhuf", "fuzz_highscore"):
            write(out / harness, path.name, path.read_bytes())

    aggregate = find(root, "DATA", "HEROES.AGG")
    blob = aggregate.read_bytes()
    (count,) = struct.unpack_from("<h", blob)
    entries = []
    for index in range(count):
        file_id, offset, size = struct.unpack_from("<HiI", blob, 2 + 10 * index)
        entries.append((file_id, blob[offset:offset + size]))
    by_id = dict(entries)
    resources = out / "fuzz_resources"
    small = []
    written = {}
    for file_id, payload in entries:
        kind = classify(payload)
        if kind is None:
            # Sounds and window layouts; a few samples are enough.
            if written.get(SAMPLE, 0) < 8 and len(payload) < 20000:
                write(resources, f"sample-{file_id:04x}", payload + bytes([SAMPLE]))
                written[SAMPLE] = written.get(SAMPLE, 0) + 1
            continue
        if kind == FONT:
            # The font record names its glyph icon; the harness stores the
            # payload after the record as FUZZ.ICN.
            name = payload[4:17].split(b"\0")[0].decode("ascii", "replace")
            glyphs = by_id.get(make_file_id(name))
            if glyphs is None:
                continue
            record = payload[:4] + b"FUZZ.ICN".ljust(13, b"\0")
            write(resources, f"font-{file_id:04x}", record + glyphs + bytes([FONT]))
        else:
            prefix = {ICON: "icon", BITMAP: "bitmap", TILESET: "tileset", PALETTE: "palette"}[kind]
            write(resources, f"{prefix}-{file_id:04x}", payload + bytes([kind]))
            if kind == BITMAP and len(payload) == 6 + 32 * 32 and written.get(CURSOR, 0) < 8:
                # Pointers are 32 x 32 bitmaps.
                write(resources, f"cursor-{file_id:04x}", payload + bytes([CURSOR]))
                written[CURSOR] = written.get(CURSOR, 0) + 1
        written[kind] = written.get(kind, 0) + 1
        if len(payload) < 4096 and len(small) < 6:
            small.append((file_id, payload))
    write(resources, "archive-small", archive(small) + bytes([ARCHIVE]))
    # The help book: HELP\HEROES.HLP in the game data, else $HOMM1_HELP (the
    # .HLP; its .CNT beside it). fuzz_help takes the help file, the contents
    # file and the contents file's length.
    help_file = find(root, "HELP", "HEROES.HLP")
    if help_file is None and os.environ.get("HOMM1_HELP"):
        help_file = Path(os.environ["HOMM1_HELP"])
    if help_file is not None and help_file.is_file():
        contents = help_file.with_suffix(".CNT")
        if not contents.is_file():
            contents = help_file.with_suffix(".cnt")
        hlp = help_file.read_bytes()
        cnt = contents.read_bytes() if contents.is_file() else b""
        write(out / "fuzz_help", "help", hlp + struct.pack("<H", 0xFFFF))
        if cnt:
            write(out / "fuzz_help", "help-contents", hlp + cnt + struct.pack("<H", len(cnt)))
    # fuzz_remote: packets of the sizes the protocol's messages have, with
    # their CRC, and the bare payloads (no game data).
    for size in (0, 2, 3, 4, 8, 16, 27, 110, 183, 200, 202, 246):
        payload = bytes((index * 37 + size) & 0xFF for index in range(size))
        message = struct.pack("<BiBBh", 1, size, 2, 3, size) + payload
        header = struct.pack("<BBBB", 0, 1, size & 0xFF, len(message))
        crc = remote_crc(header + b"\0\0" + message)
        write(out / "fuzz_remote", f"packet-{size}", header + struct.pack("<H", crc) + message)
        write(out / "fuzz_remote", f"payload-{size}", payload)
    write(out / "fuzz_remote", "modem-id", b"ID123456_3")
    write(out / "fuzz_remote", "netbios", b"Empire Too " + b"HOST".ljust(16, b" "))
    for harness in sorted(os.listdir(out)):
        print(f"{harness}: {len(os.listdir(out / harness))} seeds")


if __name__ == "__main__":
    main()
