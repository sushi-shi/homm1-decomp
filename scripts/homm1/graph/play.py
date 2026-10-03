"""homm1.graph.play - install a built HEROESW.EXE beside local game data and run it.

One definition shared by `homm1 play` (the matching build's candidate) and the
generated clean tree's `nix run path:. -- --data DIR` (its own build.py), after
Gruntz's `gruntz play` and clean-export runner. The clean tree carries a copy
of this file as play.py, so it imports nothing beyond the standard library.

<target>/ holds:

    game/      the game folder: HEROESW.EXE (replaced on every launch), the
               user's DATA, MAPS and GAMES copied once (saves and high scores
               stay here, the user's folder is never written), other folders
               linked, and the SMACKW32/MSS32 runtime DLLs from the user's
               installation. Wine's built-in wing32 serves WinG.
    cd/        drive D:, a CD-ROM: the user's --cd folder, or a stand-in that
               holds _autorun/autorun.exe (the CD check opens it) and links
               HEROES/SOUND and HEROES/ANIM to whichever copy the data has.
    prefix/    a Wine prefix for playing, separate from the build prefix, with
               a 640x480 virtual desktop and the game's registry key.
    play.sh    the runner: gamescope integer-scales the 640x480 desktop and
               stops this prefix's wineserver on exit.
"""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess

#: Retail runtime files the user's installation must provide.
REQUIRED = ("DATA/HEROES.AGG",)
RUNTIME_DLLS = ("SMACKW32.DLL", "MSS32.DLL")
#: Folders the game writes (saves, high scores, configuration): copied, never linked.
LOCAL_DIRS = ("DATA", "GAMES", "MAPS")
#: CD folders the game reads from D:\HEROES\.
CD_DIRS = ("SOUND", "ANIM")
REGISTRY_KEY = r"HKLM\SOFTWARE\New World Computing\Heroes of Might and Magic\1.0"
WINE_ENV = {"WINEDLLOVERRIDES": "mscoree,mshtml=", "WINEDEBUG": "fixme-all,err-kerberos"}


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


def runtime(data: Path, cd: Path | None) -> dict[str, Path]:
    """The user's runtime files: {name: path}. Raises ValueError naming the gaps."""
    roots = [data, data / "HEROES", *([cd, cd / "HEROES"] if cd else [])]
    found, missing = {}, []
    for name in REQUIRED + RUNTIME_DLLS:
        hit = next((p for p in (find_ci(r, name) for r in roots) if p and p.is_file()), None)
        if hit is None:
            missing.append(name)
        else:
            found[name] = hit
    if missing:
        raise ValueError(f"{data}: missing {', '.join(missing)} - point --data at an "
                         "installed Heroes of Might and Magic (Windows 95) folder")
    return found


def _copy_missing(source: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    for entry in source.iterdir():
        target = find_ci(destination, entry.name) or destination / entry.name
        if entry.is_dir():
            _copy_missing(entry, target)
        elif not target.exists():
            shutil.copy2(entry, target)


def install(target: Path, data: Path, cd: Path | None, executable: Path) -> Path:
    """Fill <target>/game and <target>/cd; returns the CD root for drive D:."""
    found = runtime(data, cd)
    game = target / "game"
    game.mkdir(parents=True, exist_ok=True)
    agg_home = found["DATA/HEROES.AGG"].parent.parent
    for entry in agg_home.iterdir():
        if entry.name.lower() in {"heroes.exe", "heroesw.exe"}:
            continue
        existing = find_ci(game, entry.name)
        if entry.is_dir() and entry.name.upper() in LOCAL_DIRS:
            _copy_missing(entry, existing or game / entry.name)
        elif existing is None and entry.is_dir():
            (game / entry.name).symlink_to(entry.resolve(), target_is_directory=True)
        elif existing is None:
            shutil.copy2(entry, game / entry.name)
    for name in RUNTIME_DLLS:
        if find_ci(game, name) is None:
            shutil.copy2(found[name], game / found[name].name)
    for name in LOCAL_DIRS:
        if find_ci(game, name) is None:
            (game / name).mkdir()
    installed = find_ci(game, "HEROESW.EXE") or game / "HEROESW.EXE"
    installed.unlink(missing_ok=True)
    shutil.copy2(executable, installed)

    if cd is not None:
        if find_ci(cd, "_autorun/autorun.exe") is None:
            raise ValueError(f"{cd}: not a Heroes CD (no _autorun/autorun.exe)")
        return cd.resolve()
    stand_in = target / "cd"
    (stand_in / "_autorun").mkdir(parents=True, exist_ok=True)
    shutil.copy2(executable, stand_in / "_autorun" / "autorun.exe")
    heroes = stand_in / "HEROES"
    heroes.mkdir(exist_ok=True)
    for name in CD_DIRS:
        link = heroes / name
        source = next((p for p in (find_ci(r, name) for r in (data, data / "HEROES", agg_home))
                       if p and p.is_dir()), None)
        if link.is_symlink():
            link.unlink()
        if source is not None:
            link.symlink_to(source.resolve(), target_is_directory=True)
        else:
            print(f"[play] no {name} folder in {data}: pass --cd with the game CD for "
                  f"{name.lower()} files")
    return stand_in


def windows_path(path: Path) -> str:
    return "Z:" + str(path.resolve()).replace("/", "\\")


def prepare_prefix(target: Path, cd_root: Path) -> Path:
    """Create or refresh the game prefix; returns its path."""
    prefix = target / "prefix"
    env = dict(os.environ, WINEPREFIX=str(prefix), **WINE_ENV)
    if not (prefix / "drive_c").is_dir():
        print(f"[play] creating the game Wine prefix {prefix}", flush=True)
        prefix.mkdir(parents=True, exist_ok=True)
        subprocess.run(["wineboot", "--init"], env=env, check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def reg(key: str, name: str, value: str) -> None:
        subprocess.run(["wine", "reg", "add", key, "/v", name, "/d", value, "/f"],
                       env=env, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    try:
        drive = prefix / "dosdevices" / "d:"
        if drive.is_symlink() or drive.exists():
            drive.unlink()
        drive.symlink_to(cd_root, target_is_directory=True)
        reg(r"HKLM\Software\Wine\Drives", "D:", "cdrom")
        reg(r"HKCU\Software\Wine\Explorer", "Desktop", "Default")
        reg(r"HKCU\Software\Wine\Explorer\Desktops", "Default", "640x480")
        reg(REGISTRY_KEY, "AppPath", windows_path(target / "game"))
        reg(REGISTRY_KEY, "CDDrive", "D:")
    finally:
        subprocess.run(["wineserver", "-k"], env=env, check=False,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.run(["wineserver", "-w"], env=env, check=False)
    return prefix


#: @PLAY_SHELL@ names the flake shell that supplies gamescope and Wine.
_PLAY_SH = """\
#!/usr/bin/env bash
# Generated by homm1 play; do not edit.
#   ./play.sh                 run game/HEROESW.EXE
#   PLAY_W/PLAY_H             gamescope output size (default 2560x1440)
#   PLAY_GAMESCOPE=0          plain Wine with its 640x480 virtual desktop
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "[play] HEROESW.EXE md5=$(md5sum "$here/game/HEROESW.EXE" | cut -c1-12)"
game_cmd=(bash -c '
cleanup() { wineserver -k >/dev/null 2>&1 || true; }
trap cleanup EXIT
trap "exit 129" HUP
trap "exit 130" INT
trap "exit 143" TERM
wine HEROESW.EXE
' homm1-play)
export WINEPREFIX="$here/prefix" WINEDLLOVERRIDES="mscoree,mshtml="
export WINEDEBUG="${WINEDEBUG:-fixme-all,err-kerberos}"
cd "$here/game"
if [ "${PLAY_GAMESCOPE:-1}" = 0 ]; then
    exec "${game_cmd[@]}"
fi
cmd=(gamescope -W "${PLAY_W:-2560}" -H "${PLAY_H:-1440}" -w 640 -h 480
     -S integer -F nearest --force-windows-fullscreen -f -- "${game_cmd[@]}")
if command -v gamescope >/dev/null 2>&1; then
    exec "${cmd[@]}"
fi
exec nix develop "@PLAY_SHELL@" -c "${cmd[@]}"
"""


def write_play_sh(target: Path, play_shell: str) -> Path:
    path = target / "play.sh"
    text = _PLAY_SH.replace("@PLAY_SHELL@", play_shell)
    if not path.exists() or path.read_text() != text:
        path.write_text(text)
    path.chmod(0o755)
    return path


def remembered(target: Path, name: str, given: Path | None) -> Path | None:
    """`given`, else the folder remembered from an earlier launch."""
    record = target / f"{name}-path.txt"
    if given is not None:
        return given.expanduser().resolve()
    if record.is_file():
        return Path(record.read_text().strip())
    return None


def remember(target: Path, name: str, path: Path | None) -> None:
    if path is not None:
        target.mkdir(parents=True, exist_ok=True)
        (target / f"{name}-path.txt").write_text(f"{path}\n")


def play(target: Path, data: Path, cd: Path | None, executable: Path, play_shell: str,
         dry_run: bool = False) -> int:
    """Install, prepare the prefix, write play.sh and run it."""
    cd_root = install(target, data, cd, executable)
    prepare_prefix(target, cd_root)
    remember(target, "data", data)
    remember(target, "cd", cd)
    runner = write_play_sh(target, play_shell)
    if dry_run:
        print(f"[play] ready: {runner} (dry run; not launched)")
        return 0
    return subprocess.run([str(runner)]).returncode


def launch(argv: list[str] | None = None) -> int:
    """The clean tree's `nix run path:. -- --data DIR`: build, install, run."""
    import argparse
    import sys
    root = Path(__file__).resolve().parent
    target = root / "build" / "game"
    parser = argparse.ArgumentParser(prog="nix run path:. --", description=__doc__.split("\n")[0])
    parser.add_argument("--data", type=Path,
                        help="installed game folder (remembered after the first launch)")
    parser.add_argument("--cd", type=Path, help="the game CD's contents (remembered)")
    parser.add_argument("--jobs", type=int, default=min(8, os.cpu_count() or 1))
    parser.add_argument("--dry-run", action="store_true",
                        help="build and install, but do not start the game")
    args = parser.parse_args(argv)
    data = remembered(target, "data", args.data)
    cd = remembered(target, "cd", args.cd)
    if data is None:
        parser.error('first launch: nix run path:. -- --data "/path/to/HEROES"')
    try:
        found = runtime(data, cd)
        build = [sys.executable, str(root / "build.py"), "--jobs", str(args.jobs)]
        retail = find_ci(found["DATA/HEROES.AGG"].parent.parent, "HEROESW.EXE")
        if retail is not None:
            build += ["--icon-from", str(retail)]
        subprocess.run(build, check=True)
        return play(target, data, cd, root / "build" / "HEROESW.EXE", f"path:{root}#play",
                    args.dry_run)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Cannot start Heroes of Might and Magic: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(launch())
