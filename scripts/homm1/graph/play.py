#!/usr/bin/env python3
"""Set up a Buka 2003 game copy under Wine and run a built HEROES.EXE or
EDITOR.EXE in it.

One runner for both trees: `homm1 play` (the matching build's candidate) and
the generated source tree's `nix run .#play` and `nix run .#editor` (its own
build.py), which carries this file as play.py. It imports nothing beyond the
standard library. The scenario editor (`--target editor`) shares the game's
installed folder, CD drive, registry key and prefix: it opens DATA\\HEROES.AGG,
probes the CD for its first track, reads the same registry key (its own
`HMM1 Editor...` window settings) and edits the maps in MAPS.

`--game` takes the game copy once: an installed game folder, the CD (a mount
or a copy of its files), or the CD image (.iso) or a .zip/.7z of either. The
CD's InstallShield cabinet is unpacked with unshield and its music tracks are
copied; images are read with 7z. Each copy is checked against the retail file
names and sizes below, the resource archive and runtime DLLs also by SHA-256.
Nothing is written to the copy.

The per-user state directory (`$XDG_DATA_HOME/homm1-buka`, or `--state`):

    game/      the installed game: DATA, ANIM, SOUND, MAPS and GAMES, the
               Smacker, Miles and Audiere DLLs, and HEROES.EXE or EDITOR.EXE,
               replaced on every launch. Files are copied once and never
               overwritten, so saved games, edited maps and high scores stay
               here.
    cd/        drive D:, a CD-ROM holding Tracks/, the CD music. At start-up
               the game looks for Tracks\\02-AudioTrack 02.ogg on a CD-ROM
               drive. Without the CD the tracks are links to the installed
               SOUND files: the same music at the installed quality.
    retail/    the copy's own HEROES.EXE and EDITOR.EXE, from which the builds
               take their icons.
    prefix/    the Wine prefix: D: as a CD-ROM and the game's registry key
               (AppPath, `HMM1 CDDrive`) in the 32-bit view. The game writes
               its own settings there on its first start.
    play.json  the --game sources and the last locale played.

WinG resolves to Wine's built-in wing32, DirectDraw to Wine's. Each language
runs under the system locale of its descriptor (locales/<LANG>.json; ru_RU
for Russian), so the window title and message boxes show the text in the
language's Windows code page.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

STATE_NAME = "homm1-buka"
#: The programs the runner starts: executable, and the retail file's size and
#: SHA-256 (only its icon is used).
PROGRAMS = {
    "game": {"executable": "HEROES.EXE", "retail": (
        692297, "34233110eff3c5689664ded89577486e3fe8d6961d917c381172248a08a654db")},
    "editor": {"executable": "EDITOR.EXE", "retail": (
        340031, "103380e9a8e4030ba25a76447bef1d6f3d05c47b0caf2f6748620dc3487f0485")},
}
EXECUTABLE = PROGRAMS["game"]["executable"]
#: The game's HKLM key as the 32-bit game sees it in a 64-bit prefix.
GAME_KEY = r"Software\Wow6432Node\Buka\3DO\Heroes of Might and Magic Platinum\1.000"
CD_DRIVE = "d:"
DESKTOP = "640x480"
#: Seconds within which a failed full-screen start suggests --window.
QUICK_EXIT = 30
WINE_ENV = {"WINEDLLOVERRIDES": "mscoree,mshtml="}

#: The resource archive every screen reads: path, size, SHA-256.
AGG = ("DATA/heroes.agg", 21661575,
       "fec418e23d3b777e850ee1feb289248c613e4c18641edf286a5c672d9dac8687")
#: Runtime DLLs the game loads from its folder: size, SHA-256.
RUNTIME_DLLS = {
    "SMACKW32.DLL": (67584, "0d9973230fd4eff0632c01954a49bd43a372828be5b932c3f84da08896431e68"),
    "MSS32.DLL": (144384, "a2912c7f00475f22e310351cfdf6fa2be3529f3ff1a9c245830d1b7c3486fd6c"),
    "audiere.dll": (475136, "10f1975637690fc1bb051371dfb7bec6c7a7f496e4e363e4dd5cbc8b5db3ebec"),
}

#: The installed game's data files and sizes. DATA/ is required; a missing or
#: different ANIM, SOUND or MAPS file is reported but does not stop the game.
GAME_FILES = {
    "DATA/CAMPAIGN.HS": 870, "DATA/heroes.agg": 21661575, "DATA/NETLODR.DAT": 3469,
    "DATA/ORIGDATA.BIN": 62111, "DATA/REMOTE.GAM": 77663, "DATA/STANDARD.HS": 870,
    "ANIM/buka.smk": 688092, "ANIM/intro.smk": 2138068, "ANIM/lose.smk": 297388,
    "ANIM/NWCLOGO.SMK": 1324960, "ANIM/win1.smk": 2320456, "ANIM/win2.smk": 419160,
    "MAPS/AES31000.MAP": 59984, "MAPS/BEM20234.MAP": 59540, "MAPS/CAMP1.CMP": 58176,
    "MAPS/CAMP2.CMP": 61062, "MAPS/CAMP3.CMP": 58086, "MAPS/CAMP4.CMP": 57510,
    "MAPS/CAMP5.CMP": 59400, "MAPS/CAMP6.CMP": 58882, "MAPS/CAMP7.CMP": 60525,
    "MAPS/CAMP8.CMP": 60224, "MAPS/CAMP9.CMP": 58028, "MAPS/CNM51234.MAP": 60280,
    "MAPS/DNL31234.MAP": 60206, "MAPS/ENS11234.MAP": 58652, "MAPS/FEL60234.MAP": 59540,
    "MAPS/GHM41000.MAP": 59540, "MAPS/HNM11234.MAP": 58948, "MAPS/INM61234.MAP": 60132,
    "MAPS/JEM70234.MAP": 59688, "MAPS/KNS21234.MAP": 59614, "MAPS/LNS41234.MAP": 59318,
    "MAPS/MIS71200.MAP": 59096, "MAPS/NHL51000.MAP": 60892, "MAPS/ONL71234.MAP": 59762,
    "MAPS/PNM31234.MAP": 59614, "MAPS/QNL11234.MAP": 60206, "MAPS/RNL41234.MAP": 59688,
    "MAPS/SEL21234.MAP": 61168, "MAPS/THS51000.MAP": 59614, "MAPS/UHS61200.MAP": 60280,
    "MAPS/VILV1234.MAP": 80056, "MAPS/W95A1234.MAP": 80426, "MAPS/W95B1234.MAP": 79908,
    "MAPS/W95C1234.MAP": 86124, "MAPS/W95D1234.MAP": 80722, "MAPS/W95E1000.MAP": 82257,
    "MAPS/W95F1234.MAP": 83608, "MAPS/W95G1234.MAP": 80836, "MAPS/W95H1234.MAP": 80574,
    "MAPS/W95I1234.MAP": 79982, "MAPS/W95J1234.MAP": 79908, "MAPS/W95K1234.MAP": 81832,
    "MAPS/W95L1234.MAP": 80426, "MAPS/W95M1234.MAP": 80944, "MAPS/W95N1234.MAP": 80056,
    "MAPS/W95O1234.MAP": 81462, "MAPS/W95P1234.MAP": 84718, "SOUND/HEROES00.ogg": 830072,
    "SOUND/HEROES01.ogg": 615246, "SOUND/HEROES02.ogg": 845890, "SOUND/HEROES03.ogg": 833960,
    "SOUND/HEROES04.ogg": 640353, "SOUND/HEROES05.ogg": 590831, "SOUND/HEROES06.ogg": 626264,
    "SOUND/HEROES07.ogg": 26861, "SOUND/HEROES08.ogg": 31517, "SOUND/HEROES09.ogg": 23673,
    "SOUND/HEROES10.ogg": 31827, "SOUND/HEROES11.ogg": 27962, "SOUND/HEROES12.ogg": 41897,
    "SOUND/HEROES13.ogg": 47926, "SOUND/HEROES14.ogg": 31656, "SOUND/HEROES15.ogg": 29997,
    "SOUND/HEROES16.ogg": 15554, "SOUND/HEROES17.ogg": 33968, "SOUND/HEROES18.ogg": 7295,
    "SOUND/HEROES19.ogg": 29128, "SOUND/HEROES20.ogg": 26478, "SOUND/HEROES21.ogg": 17729,
    "SOUND/HEROES22.ogg": 13991, "SOUND/HEROES23.ogg": 14254, "SOUND/HEROES24.ogg": 13086,
    "SOUND/HEROES25.ogg": 17367, "SOUND/HEROES26.ogg": 21675, "SOUND/HEROES27.ogg": 26932,
    "SOUND/HEROES28.ogg": 48950, "SOUND/HEROES29.ogg": 1034774, "SOUND/HEROES30.ogg": 1464386,
    "SOUND/HEROES31.ogg": 1347690, "SOUND/HEROES32.ogg": 1442582, "SOUND/HEROES40.ogg": 988379,
    "SOUND/HEROES41.ogg": 636165, "SOUND/HEROES42.ogg": 720173, "SOUND/HEROES43.ogg": 65353,
    "SOUND/HEROES44.ogg": 100422, "SOUND/HEROES45.ogg": 100127, "SOUND/HEROES46.ogg": 65539,
    "SOUND/HEROES47.ogg": 55516, "SOUND/HEROES48.ogg": 447957, "SOUND/HEROES49.ogg": 323713,
    "SOUND/HEROES50.ogg": 23862, "SOUND/HEROES51.ogg": 24860, "SOUND/HEROES52.ogg": 8293,
    "SOUND/HEROES53.ogg": 804056, "SOUND/HEROES54.ogg": 336912, "SOUND/HEROES99.ogg": 366911,
}

#: The CD's music (Tracks/ on the disc) and sizes.
CD_TRACKS = {
    "02-AudioTrack 02.ogg": 2624651, "03-AudioTrack 03.ogg": 2237745,
    "04-AudioTrack 04.ogg": 2884677, "05-AudioTrack 05.ogg": 3265921,
    "06-AudioTrack 06.ogg": 2143222, "07-AudioTrack 07.ogg": 2333200,
    "08-AudioTrack 08.ogg": 2409259, "09-AudioTrack 09.ogg": 89502,
    "10-AudioTrack 10.ogg": 114125, "11-AudioTrack 11.ogg": 68876,
    "12-AudioTrack 12.ogg": 110892, "13-AudioTrack 13.ogg": 76915,
    "14-AudioTrack 14.ogg": 141112, "15-AudioTrack 15.ogg": 178303,
    "16-AudioTrack 16.ogg": 100376, "17-AudioTrack 17.ogg": 109629,
    "18-AudioTrack 18.ogg": 52556, "19-AudioTrack 19.ogg": 115591,
    "20-AudioTrack 20.ogg": 21592, "21-AudioTrack 21.ogg": 101717,
    "22-AudioTrack 22.ogg": 108339, "23-AudioTrack 23.ogg": 74306,
    "24-AudioTrack 24.ogg": 47808, "25-AudioTrack 25.ogg": 50105,
    "26-AudioTrack 26.ogg": 37175, "27-AudioTrack 27.ogg": 56320,
    "28-AudioTrack 28.ogg": 77814, "29-AudioTrack 29.ogg": 96420,
    "30-AudioTrack 30.ogg": 150877, "31-AudioTrack 31.ogg": 3730849,
    "32-AudioTrack 32.ogg": 5478033, "33-AudioTrack 33.ogg": 5085116,
    "34-AudioTrack 34.ogg": 5148186, "35-AudioTrack 35.ogg": 3145591,
    "36-AudioTrack 36.ogg": 2176259, "37-AudioTrack 37.ogg": 2283424,
    "38-AudioTrack 38.ogg": 254514, "39-AudioTrack 39.ogg": 397363,
    "40-AudioTrack 40.ogg": 307668, "41-AudioTrack 41.ogg": 234929,
    "42-AudioTrack 42.ogg": 766709, "43-AudioTrack 43.ogg": 1523674,
    "44-AudioTrack 44.ogg": 1121887, "45-AudioTrack 45.ogg": 84023,
    "46-AudioTrack 46.ogg": 86327, "47-AudioTrack 47.ogg": 23108,
    "48-AudioTrack 48.ogg": 2759280, "49-AudioTrack 49.ogg": 1274259,
    "50-AudioTrack 50.ogg": 1424744,
}

#: SOUND\\HEROESnn.ogg -> CD track mm (Tracks\\mm-AudioTrack mm.ogg), from the
#: game's track table: both hold the same piece.
TRACK_OF_SOUND = {**{n: n + 2 for n in range(33)}, **{n: n - 5 for n in range(40, 55)}, 99: 50}

#: The CD's installer, and its file groups that form the installed game folder.
CABINET = "autorun/launch/Setup1/data1.cab"
INSTALL_GROUPS = ("Data", "Program_DLLs", "Program_Executable_Files")
#: Executables the game folder gets from the build, not from the copy.
BUILT = ("heroes.exe", "editor.exe")
IMAGES = (".iso", ".zip", ".7z")


class PlayError(Exception):
    pass


def say(message: str) -> None:
    print(f"[play] {message}", flush=True)


# --------------------------------------------------------------------------
# Files
# --------------------------------------------------------------------------

def find_ci(root: Path, relative: str) -> Path | None:
    """`relative` under `root`, matching every component case-insensitively."""
    path = root
    for part in relative.split("/"):
        if not path.is_dir():
            return None
        path = next((p for p in path.iterdir() if p.name.lower() == part.lower()), None)
        if path is None:
            return None
    return path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def windows_path(path: Path) -> str:
    return "Z:" + str(path.resolve()).replace("/", "\\")


def check_install(root: Path) -> tuple[list[str], list[str]]:
    """(errors, warnings) for an installed game folder."""
    errors, warnings = [], []
    for relative, (size, digest) in {AGG[0]: AGG[1:], **RUNTIME_DLLS}.items():
        path = find_ci(root, relative)
        if path is None or not path.is_file():
            errors.append(f"missing {relative}")
        elif path.stat().st_size != size or sha256(path) != digest:
            errors.append(f"{relative} is not the Buka 2003 file")
    for relative, size in GAME_FILES.items():
        path = find_ci(root, relative)
        found = errors if relative.startswith("DATA/") else warnings
        if path is None or not path.is_file():
            found.append(f"missing {relative}")
        elif path.stat().st_size != size and relative.startswith(("ANIM/", "SOUND/", "MAPS/")):
            # DATA's high scores and network save are rewritten by play.
            found.append(f"{relative}: {path.stat().st_size} bytes, expected {size}")
    return errors, warnings


def check_tracks(directory: Path) -> list[str]:
    problems = []
    for name, size in CD_TRACKS.items():
        path = find_ci(directory, name)
        if path is None:
            problems.append(f"missing Tracks/{name}")
        elif path.stat().st_size != size:
            problems.append(f"Tracks/{name}: {path.stat().st_size} bytes, expected {size}")
    return problems


def copy_missing(source: Path, destination: Path, skip: tuple[str, ...] = ()) -> int:
    """Copy the files `destination` lacks (names compared case-insensitively);
    returns how many. Top-level names in `skip` are left out."""
    copied = 0
    destination.mkdir(parents=True, exist_ok=True)
    for entry in sorted(source.iterdir()):
        if entry.name.lower() in skip:
            continue
        target = find_ci(destination, entry.name) or destination / entry.name
        if entry.is_dir():
            copied += copy_missing(entry, target)
        elif not target.exists():
            partial = target.with_name(target.name + ".partial")
            shutil.copyfile(entry, partial)
            partial.rename(target)
            copied += 1
    return copied


# --------------------------------------------------------------------------
# Game copies
# --------------------------------------------------------------------------

class Copy:
    """What one --game argument provides."""

    def __init__(self, given: Path):
        self.given = given
        self.install: Path | None = None     # an installed game folder
        self.tracks: Path | None = None      # the CD's Tracks/
        self.executables: dict[str, Path] = {}   # its HEROES.EXE and EDITOR.EXE


def tool(name: str) -> str:
    found = shutil.which(name)
    if found is None:
        raise PlayError(f"{name} is required to read the game copy; `nix run .#play` "
                        "supplies it")
    return found


def search(root: Path, relative: str, depth: int) -> Path | None:
    """The shallowest folder at most `depth` levels below `root` holding `relative`."""
    level = [root]
    for _ in range(depth + 1):
        for directory in level:
            if find_ci(directory, relative) is not None:
                return directory
        level = [p for d in level for p in sorted(d.iterdir())
                 if p.is_dir() and not p.is_symlink()]
    return None


def unpack(image: Path, work: Path, dry_run: bool) -> Path | None:
    """An image's files (from a CD image only the installer and the music)."""
    if dry_run:
        listing = subprocess.run([tool("7z"), "l", "-ba", str(image)], capture_output=True,
                                 text=True, errors="replace")
        if listing.returncode:
            raise PlayError(f"{image}: 7z cannot read it")
        say(f"would unpack {image} with 7z")
        return None
    target = work / "image"
    members = ["autorun/launch/Setup1", "Tracks"] if image.suffix.lower() == ".iso" else []
    say(f"unpacking {image}")
    result = subprocess.run([tool("7z"), "x", "-y", "-bd", f"-o{target}", str(image), *members],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True,
                            errors="replace")
    if result.returncode:
        raise PlayError(f"{image}: 7z failed: {result.stderr.strip()[-400:]}")
    return target


def install_from_cabinet(cabinet: Path, work: Path) -> Path:
    """The installed game folder, laid out as the CD's installer lays it out."""
    unpacked = work / "cabinet"
    say(f"unpacking the installer {cabinet}")
    result = subprocess.run([tool("unshield"), "-d", str(unpacked), "x", str(cabinet)],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True,
                            errors="replace")
    if result.returncode:
        raise PlayError(f"{cabinet}: unshield failed: {result.stderr.strip()[-400:]}")
    install = work / "install"
    install.mkdir()
    for group in INSTALL_GROUPS:
        directory = unpacked / group
        if not directory.is_dir():
            raise PlayError(f"{cabinet}: no {group} file group; not the Buka 2003 installer")
        for entry in directory.iterdir():
            entry.rename(install / entry.name)
    return install


def locate(given: Path, work: Path, dry_run: bool) -> Copy:
    """Find the installed game, the CD music and the executable in `given`."""
    copy = Copy(given)
    if not given.exists():
        raise PlayError(f"{given}: no such file or folder")
    root = given
    if given.is_file():
        if given.suffix.lower() not in IMAGES:
            raise PlayError(f"{given}: pass a game folder, a CD folder or a "
                            f"{'/'.join(IMAGES)} image")
        root = unpack(given, work, dry_run)
        if root is None:
            return copy
    install = search(root, AGG[0], depth=3)
    cd = search(root, CABINET, depth=2)
    if install is None and cd is not None:
        if dry_run:
            say(f"would unpack the installer {find_ci(cd, CABINET)} with unshield")
        else:
            install = install_from_cabinet(find_ci(cd, CABINET), work)
    if install is not None:
        copy.install = install
        for program, facts in PROGRAMS.items():
            found = find_ci(install, facts["executable"])
            if found is not None:
                copy.executables[program] = found
    for home in (cd, install, install.parent if install else None):
        if home is not None and find_ci(home, "Tracks") is not None:
            copy.tracks = find_ci(home, "Tracks")
            break
    if copy.install is None and copy.tracks is None and cd is None:
        raise PlayError(f"{given}: no game here (looked for {AGG[0]} and {CABINET})")
    return copy


# --------------------------------------------------------------------------
# State
# --------------------------------------------------------------------------

def default_state() -> Path:
    base = os.environ.get("XDG_DATA_HOME") or Path.home() / ".local" / "share"
    return Path(base) / STATE_NAME


def installed_game(state: Path | None = None) -> Path | None:
    """The state's game folder, once a game copy has been imported."""
    game = (state or default_state()) / "game"
    return game if find_ci(game, AGG[0]) is not None else None


def load_config(state: Path) -> dict:
    try:
        return json.loads((state / "play.json").read_text())
    except (OSError, ValueError):
        return {}


def save_config(state: Path, **values) -> None:
    config = {**load_config(state), **values}
    state.mkdir(parents=True, exist_ok=True)
    (state / "play.json").write_text(json.dumps(config, indent=2, ensure_ascii=False) + "\n")


def keep_retail(state: Path, executables: dict[str, Path]) -> None:
    """Keep each retail program of the copy (checked by size and SHA-256)."""
    for program, executable in executables.items():
        size, digest = PROGRAMS[program]["retail"]
        if executable.stat().st_size != size or sha256(executable) != digest:
            continue
        retail = state / "retail" / PROGRAMS[program]["executable"]
        retail.parent.mkdir(parents=True, exist_ok=True)
        if not retail.exists():
            shutil.copyfile(executable, retail)


def retail_executable(state: Path, program: str = "game") -> Path | None:
    retail = state / "retail" / PROGRAMS[program]["executable"]
    return retail if retail.is_file() else None


def import_game(state: Path, given: list[Path], dry_run: bool) -> None:
    """Check every --game copy and copy the game and its CD music into `state`."""
    game, tracks = state / "game", state / "cd" / "Tracks"
    given = [path.expanduser().resolve() for path in given]
    if not dry_run:
        state.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="import-", dir=None if dry_run else state) as work:
        copies = [locate(path, Path(work) / str(i), dry_run) for i, path in enumerate(given)]
        install = next((c.install for c in copies if c.install), None)
        if install is not None:
            errors, warnings = check_install(install)
            for warning in warnings[:10]:
                say(f"warning: {warning}")
            if errors:
                raise PlayError(f"{install}: not the Buka 2003 game: " + "; ".join(errors[:6]))
            say(f"checked the game files in {install}")
            if dry_run:
                say(f"would copy them into {game} (files already there are kept)")
            else:
                say(f"copied {copy_missing(install, game, BUILT)} file(s) into {game}")
        elif installed_game(state) is None and not dry_run:
            raise PlayError("no installed game in " + ", ".join(map(str, given)) +
                            ": pass the game folder, the CD or its image")
        if not dry_run:
            for copy in copies:
                keep_retail(state, copy.executables)
        music = next((c.tracks for c in copies if c.tracks), None)
        if music is not None:
            for problem in check_tracks(music)[:10]:
                say(f"warning: {problem}")
            if dry_run:
                say(f"would copy the CD music {music} into {tracks}")
            else:
                for link in tracks.iterdir() if tracks.is_dir() else ():
                    if link.is_symlink():
                        link.unlink()
                say(f"copied {copy_missing(music, tracks)} CD track(s) into {tracks}")
    if not dry_run:
        save_config(state, game=[str(path) for path in given])


def stand_in_tracks(state: Path, dry_run: bool) -> None:
    """Link each missing CD track to the installed SOUND file holding it."""
    tracks = state / "cd" / "Tracks"
    missing = {name for name in CD_TRACKS if find_ci(tracks, name) is None}
    if not missing:
        return
    if dry_run:
        say(f"would link {len(missing)} missing CD track(s) in {tracks} to the installed music")
        return
    sound = find_ci(state / "game", "SOUND")
    if sound is None:
        raise PlayError("no CD music and no installed SOUND folder: pass the CD with --game")
    tracks.mkdir(parents=True, exist_ok=True)
    linked = 0
    for number, track in TRACK_OF_SOUND.items():
        name = f"{track:02d}-AudioTrack {track:02d}.ogg"
        installed = find_ci(sound, f"HEROES{number:02d}.ogg")
        if name in missing and installed is not None:
            (tracks / name).symlink_to(installed.resolve())
            linked += 1
    say(f"no CD music: linked {linked} track(s) in {tracks} to the installed SOUND files")


# --------------------------------------------------------------------------
# Wine
# --------------------------------------------------------------------------

def descriptors() -> dict[str, dict]:
    """The languages of the nearest locales/ (the source tree's, or the
    checkout's): {code: descriptor}."""
    here = Path(__file__).resolve().parent
    for root in (here, *here.parents):
        found = sorted((root / "locales").glob("*.json"))
        if found:
            return {path.stem: json.loads(path.read_text(encoding="utf-8")) for path in found}
    return {}


def wine_env(state: Path, locale: str) -> dict[str, str]:
    env = dict(os.environ, WINEPREFIX=str(state / "prefix"), **WINE_ENV)
    env.setdefault("WINEDEBUG", "-all")
    system_locale = descriptors().get(locale, {}).get("system_locale")
    if system_locale:
        env["LC_ALL"] = system_locale
        archive = os.environ.get("HOMM1_LOCALE_ARCHIVE")
        if archive:
            env["LOCALE_ARCHIVE"] = env["LOCALE_ARCHIVE_2_27"] = archive
    return env


def registry(state: Path) -> str:
    """The prefix settings as a .reg file."""
    def string(text: str) -> str:
        return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'
    return "\r\n".join([
        "Windows Registry Editor Version 5.00", "",
        r"[HKEY_LOCAL_MACHINE\Software\Wine\Drives]",
        f'"{CD_DRIVE}"="cdrom"', "",
        f"[HKEY_LOCAL_MACHINE\\{GAME_KEY}]",
        f'"AppPath"={string(windows_path(state / "game"))}',
        f'"HMM1 CDDrive"={string(CD_DRIVE.upper())}', "", ""])


def stop_wine(env: dict[str, str]) -> None:
    subprocess.run(["wineserver", "-k"], env=env, check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    subprocess.run(["wineserver", "-w"], env=env, check=False)


def prepare_prefix(state: Path, locale: str, reset: bool, dry_run: bool) -> None:
    """Create the prefix if needed, map D: and write the game's registry key."""
    prefix = state / "prefix"
    env = wine_env(state, locale)
    if reset and prefix.exists():
        if dry_run:
            say(f"would delete the Wine prefix {prefix}")
        else:
            stop_wine(env)
            shutil.rmtree(prefix)
            say(f"deleted the Wine prefix {prefix}")
    settings = registry(state)
    if dry_run:
        if reset or not (prefix / "drive_c").is_dir():
            say(f"would create the Wine prefix {prefix} (wineboot -i)")
        say(f"would map {CD_DRIVE.upper()} as a CD-ROM to {state / 'cd'}")
        say(f"would set HKLM\\{GAME_KEY}: AppPath, HMM1 CDDrive")
        return
    if not (prefix / "drive_c").is_dir():
        say(f"creating the Wine prefix {prefix}")
        prefix.mkdir(parents=True, exist_ok=True)
        log = state / "wineboot.log"
        with log.open("w") as stream:
            result = subprocess.run(["wineboot", "-i"], env=env, stdout=stream, stderr=stream)
        stop_wine(env)
        if result.returncode or not (prefix / "drive_c").is_dir():
            raise PlayError(f"wineboot failed; see {log}")
    cd = state / "cd"
    cd.mkdir(parents=True, exist_ok=True)
    drive = prefix / "dosdevices" / CD_DRIVE
    if not drive.is_symlink() or Path(os.readlink(drive)) != cd:
        if drive.is_symlink() or drive.exists():
            drive.unlink()
        drive.symlink_to(cd, target_is_directory=True)
    stamp = prefix / ".homm1-play.reg"
    if stamp.is_file() and stamp.read_bytes() == settings.encode():
        return
    with tempfile.TemporaryDirectory() as work:
        reg = Path(work) / "homm1.reg"
        reg.write_text(settings, encoding="utf-16")
        try:
            result = subprocess.run(["wine", "reg", "import", windows_path(reg)], env=env,
                                    capture_output=True, text=True, errors="replace")
        finally:
            stop_wine(env)
    if result.returncode:
        raise PlayError(f"wine reg import failed: {(result.stdout + result.stderr).strip()}")
    stamp.write_bytes(settings.encode())
    say(f"prefix {prefix}: {CD_DRIVE.upper()} is a CD-ROM, game key written")


def launch_game(state: Path, executable: Path, locale: str, window: bool,
                extra: list[str], dry_run: bool, program: str = "game") -> int:
    """Install `executable` in the game folder as the program's retail name
    and run it; returns its status."""
    game = state / "game"
    name = PROGRAMS[program]["executable"]
    # explorer starts the program only by its full path.
    command = ["wine", windows_path(game / name), *extra]
    if window:
        command[1:1] = ["explorer", f"/desktop=Heroes,{DESKTOP}"]
    env = wine_env(state, locale)
    if dry_run:
        say(f"would install {executable} as {game / name}")
        say(f"would run in {game}: {' '.join(command)}"
            + (f" (LC_ALL={env['LC_ALL']})" if "LC_ALL" in env else ""))
        return 0
    for entry in game.iterdir():
        if entry.name.lower() == name.lower():
            entry.unlink()
    shutil.copyfile(executable, game / name)
    say(f"running {executable} (md5 {hashlib.md5(executable.read_bytes()).hexdigest()[:12]})"
        + (f" in a {DESKTOP} window" if window else ""))
    started = time.monotonic()
    try:
        status = subprocess.run(command, cwd=game, env=env).returncode
    except KeyboardInterrupt:
        return 130
    finally:
        stop_wine(env)
    if status and not window and time.monotonic() - started < QUICK_EXIT:
        # The program stops at start-up when the display cannot switch to 640x480.
        say(f"{name} stopped at start-up (status {status}); if the screen could not "
            f"switch to {DESKTOP}, run with --window")
    return status


# --------------------------------------------------------------------------
# Command line
# --------------------------------------------------------------------------

def add_arguments(parser: argparse.ArgumentParser, *, standalone: bool) -> None:
    parser.add_argument("--game", type=Path, action="append", metavar="PATH",
                        help="the game copy: installed folder, CD folder, or .iso/.zip/.7z "
                             "image; repeatable; needed on the first run only")
    if standalone:
        parser.add_argument("--locale", choices=sorted(descriptors()) or None,
                            help="program language (default: the last one played, else ru)")
        parser.add_argument("--rebuild", action="store_true",
                            help="rebuild the program even if the sources did not change")
        parser.add_argument("--jobs", type=int, default=min(8, os.cpu_count() or 1),
                            help="parallel compiler jobs")
    parser.add_argument("--window", action="store_true",
                        help=f"run inside a {DESKTOP} Wine desktop window instead of "
                             "full screen")
    parser.add_argument("--prefix-reset", action="store_true",
                        help="delete and recreate the Wine prefix (resets game settings)")
    parser.add_argument("--state", type=Path,
                        help=f"state directory (default: $XDG_DATA_HOME/{STATE_NAME})")
    parser.add_argument("--dry-run", action="store_true",
                        help="print what would be done and change nothing")


def split_argv(argv: list[str] | None) -> tuple[list[str], list[str]]:
    """(runner arguments, HEROES.EXE arguments after `--`)."""
    argv = list(sys.argv[1:] if argv is None else argv)
    if "--" in argv:
        cut = argv.index("--")
        return argv[:cut], argv[cut + 1:]
    return argv, []


def state_of(args: argparse.Namespace) -> Path:
    return (args.state or default_state()).expanduser().resolve()


def session(args: argparse.Namespace, extra: list[str], build, locale: str = "ru",
            program: str = "game") -> int:
    """Import the game copy when given, build with `build(retail_exe) -> Path`
    (the program's retail executable, for its icon), prepare the prefix and
    run the program. Errors print and return 1."""
    state = state_of(args)
    try:
        if args.dry_run:
            say(f"dry run; state directory {state}")
        if args.game:
            import_game(state, args.game, args.dry_run)
        elif installed_game(state) is None:
            if not args.dry_run:
                raise PlayError("first run: pass the game copy with --game PATH "
                                "(installed folder, CD folder or CD image)")
            say(f"no game imported in {state}; a real run needs --game PATH")
        if installed_game(state) is not None:
            stand_in_tracks(state, args.dry_run)
        executable = build(retail_executable(state, program))
        if executable is None:
            return 1
        prepare_prefix(state, locale, args.prefix_reset, args.dry_run)
        if not args.dry_run:
            save_config(state, locale=locale)
        return launch_game(state, executable, locale, args.window, extra, args.dry_run,
                           program)
    except (PlayError, OSError, subprocess.CalledProcessError) as error:
        print(f"[play] {error}", file=sys.stderr)
        return 1


# --------------------------------------------------------------------------
# The source tree's runner
# --------------------------------------------------------------------------

#: What the program is built from.
SOURCE_DIRS = ("src", "include", "vendor", "locales", "imports")
SOURCE_FILES = ("build.py", "build.json", "catalog.py", "heroes.def")


def source_fingerprint(root: Path, locale: str, icon: Path | None,
                       target: str = "game") -> str:
    digest = hashlib.sha256(f"{target}\0{locale}\0{icon is not None}\0".encode())
    paths = [root / name for name in SOURCE_FILES]
    for directory in SOURCE_DIRS:
        paths += sorted(p for p in (root / directory).rglob("*") if p.is_file())
    for path in paths:
        if path.is_file():
            digest.update(str(path.relative_to(root)).encode() + b"\0" + path.read_bytes())
    return digest.hexdigest()


def launch(argv: list[str] | None = None) -> int:
    """The source tree's `nix run .#play` (and `.#editor`, `--target editor`):
    build the program when the sources changed, then set up the game and run
    it."""
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(
        prog="nix run .#play --", description=__doc__.split("\n")[0],
        epilog="Arguments after `--` go to the program.")
    parser.add_argument("--target", choices=sorted(PROGRAMS), default="game",
                        help="the program to run (default: %(default)s; "
                             "`nix run .#editor` runs the editor)")
    add_arguments(parser, standalone=True)
    mine, extra = split_argv(argv)
    args = parser.parse_args(mine)
    state = state_of(args)
    manifest = json.loads((root / "build.json").read_text())
    locale = args.locale or load_config(state).get("locale") or manifest.get("locale", "ru")
    # A read-only tree (the flake's store copy) builds into the state directory.
    out = root / "build" if os.access(root, os.W_OK) else state / "build"
    target = args.target

    def build(icon: Path | None) -> Path | None:
        executable = out / locale / manifest["targets"][target]["executable"]
        stamp = out / locale / f".play-{target}-fingerprint"
        fingerprint = source_fingerprint(root, locale, icon, target)
        command = [sys.executable, str(root / "build.py"), "--target", target,
                   "--locale", locale, "--out", str(out), "--jobs", str(args.jobs)]
        if icon is not None:
            command += ["--icon-from", str(icon)]
        if not args.rebuild and executable.is_file() and stamp.is_file() \
                and stamp.read_text() == fingerprint:
            say(f"{executable} is up to date")
            return executable
        if args.dry_run:
            say(f"would build: {' '.join(command)}")
            return executable
        say(f"building {executable}")
        if subprocess.run(command, cwd=root).returncode or not executable.is_file():
            print("[play] the build failed", file=sys.stderr)
            return None
        stamp.write_text(fingerprint)
        return executable

    return session(args, extra, build, locale, target)


if __name__ == "__main__":
    raise SystemExit(launch())
