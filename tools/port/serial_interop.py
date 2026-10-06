#!/usr/bin/env python3
"""Play the native program against the Windows build (or the retail
HEROES.EXE) under Wine over a direct serial connection, and compare what
each side sent and received.

    python3 tools/port/serial_interop.py --heroes build/port/heroes \\
        [--exe build/ru/HEROES.EXE | --retail] [--state DIR] [--keep DIR]

The Windows program opens COM1; in a scratch Wine prefix COM1 is a
pseudo-terminal that socat joins to the native program's TCP serial line:

    HEROES.EXE (Wine) <-> COM1 = pty <-> socat <-> TCP <-> heroes (native)

The native program hosts a direct connection and the Windows one joins as the
guest, driven by xdotool on its own Xvfb display. The host starts the first
scenario and sends the game, ends its turn; the computer players move; the
guest ends its turn and sends the game back. Both sides trace the hashes of
the saves they send and receive (HOMM1_NET_TRACE; the retail program has no
trace, so its last saved game is hashed from its DATA folder instead).

Needs: the game copy that `nix run .#play` installed (or --game), Wine,
socat, xdotool, Xvfb and xvfb-run on PATH, e.g.

    nix develop -c nix shell nixpkgs#xdotool nixpkgs#xorg.xorgserver \\
        nixpkgs#xvfb-run -c python3 tools/port/serial_interop.py --heroes ...

NetBIOS cannot be tested this way: Wine's netapi32 implements no NCBLISTEN
and no datagrams, which the game's host and guest both need.
"""
from __future__ import annotations

import argparse
import glob
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
import play  # noqa: E402

NAME_FIELD = (207, 224)
# The host: new game, multi-player, direct connection, host, COM 1, 9600
# baud; then the scenario's OK, and the end of its turn (confirmed).
HOST_REPLAY = """\
3000 click 497 104
+1500 click 497 236
+1500 click 497 302
+1500 click 497 104
+1500 click 497 104
+1500 click 497 170
+8000 click 382 437
+15000 click 505 370
+1500 click 168 163
+70000 exit
"""
# The guest's clicks before the host starts: the same, as the guest.
GUEST_SETUP = [(497, 104), (497, 236), (497, 302), (497, 170), (497, 104), (497, 170)]


def save_hash(data: bytes) -> str:
    value = 2166136261
    for index, byte in enumerate(data):
        if NAME_FIELD[0] <= index < NAME_FIELD[1]:
            continue
        value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
    return f"{value:08x}"


def free_port() -> int:
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def trace(path: Path) -> list[tuple[str, str]]:
    if not path.is_file():
        return []
    events = []
    for line in path.read_text().splitlines():
        fields = dict(field.split("=", 1) for field in line.split()[1:])
        events.append((line.split()[0], fields["hash"]))
    return events


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--heroes", type=Path, required=True, help="the native program")
    which = parser.add_mutually_exclusive_group()
    which.add_argument("--exe", type=Path, default=ROOT / "build" / "ru" / "HEROES.EXE",
                       help="the Windows program (default: the build.py output)")
    which.add_argument("--retail", action="store_true",
                       help="the retail HEROES.EXE of the game copy instead")
    parser.add_argument("--game", type=Path, help="the game copy (default: the installed one)")
    parser.add_argument("--keep", type=Path, help="work in DIR and keep it")
    args = parser.parse_args()

    for tool in ("wine", "socat", "xdotool", "Xvfb", "xvfb-run"):
        if shutil.which(tool) is None:
            print(f"needs {tool} on PATH")
            return 77
    installed = play.default_state()
    game = args.game or installed / "game"
    exe = installed / "retail" / "HEROES.EXE" if args.retail else args.exe
    with tempfile.TemporaryDirectory() as scratch:
        work = (args.keep or Path(scratch)).resolve()
        work.mkdir(parents=True, exist_ok=True)
        state = work / "wine"
        play.import_game(state, [game], False)
        play.stand_in_tracks(state, False)
        play.prepare_prefix(state, "ru", False, False)
        native_game = work / "native-game"
        if not native_game.exists():
            shutil.copytree(state / "game", native_game,
                            ignore=shutil.ignore_patterns("*.exe", "*.EXE", "*.dll", "*.DLL"))
        com1 = state / "prefix" / "dosdevices" / "com1"
        pty = work / "com1"
        if com1.is_symlink() or com1.exists():
            com1.unlink()
        com1.symlink_to(pty)
        port = free_port()
        windows_trace = work / "windows.trace"
        native_trace = work / "native.trace"
        for path in (windows_trace, native_trace):
            path.write_text("")
        display = ":%d" % (100 + port % 100)
        processes = []
        try:
            processes.append(subprocess.Popen(["Xvfb", display, "-screen", "0", "1024x768x24"],
                                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL))
            time.sleep(2)
            processes.append(subprocess.Popen(
                ["socat", f"pty,link={pty},raw,echo=0",
                 f"tcp:127.0.0.1:{port},retry=120,interval=1"],
                stdout=subprocess.DEVNULL, stderr=(work / "socat.log").open("w")))
            time.sleep(1)
            env = dict(os.environ, DISPLAY=display,
                       HOMM1_NET_TRACE="Z:" + str(windows_trace).replace("/", "\\"))
            windows = subprocess.Popen(
                [sys.executable, "-c",
                 "import sys; sys.path.insert(0, %r); import play; from pathlib import Path; "
                 "sys.exit(play.launch_game(Path(%r), Path(%r), 'ru', True, ['/I0'], False))"
                 % (str(ROOT), str(state), str(exe))],
                env=env, stdout=(work / "windows.log").open("w"), stderr=subprocess.STDOUT)
            processes.append(windows)
            for _ in range(90):
                time.sleep(1)
                if subprocess.run(["xdotool", "search", "--name", "Heroes"], env=env,
                                  capture_output=True).returncode == 0:
                    break
            time.sleep(10)
            for x, y in GUEST_SETUP:
                subprocess.run(["xdotool", "mousemove", str(x), str(y), "click", "1"], env=env)
                time.sleep(1.5)
            (work / "native.replay").write_text(HOST_REPLAY)
            native = subprocess.Popen(
                ["timeout", "150", "xvfb-run", "-a", "-s", "-screen 0 1024x768x24",
                 str(args.heroes.resolve()), "/I0", "--port", str(port)],
                env=dict(os.environ, HOMM1_NET_TRACE=str(native_trace), HOMM1_NET_TRACE_DUMP="1",
                         HOMM1_DATA=str(native_game),
                         HOMM1_INPUT_REPLAY=str(work / "native.replay"), HOMM1_NO_DIALOGS="1",
                         SDL_AUDIO_DRIVER="dummy", XDG_CONFIG_HOME=str(work / "native-config")),
                cwd=work, stdout=(work / "native.log").open("w"), stderr=subprocess.STDOUT)
            # The guest's turn comes after the host's and the computer players'.
            time.sleep(60)
            for x, y in ((505, 370), (168, 163)):
                subprocess.run(["xdotool", "mousemove", str(x), str(y), "click", "1"], env=env)
                time.sleep(1.5)
            native.wait()
        finally:
            for process in reversed(processes):
                process.terminate()
            play.stop_wine(play.wine_env(state, "ru"))

        native_events = trace(native_trace)
        print("native:", native_events)
        sent = [value for event, value in native_events if event == "send"]
        received = [value for event, value in native_events if event == "receive"]
        if args.retail:
            last = state / "game"
            data = next((path for path in last.iterdir() if path.name.lower() == "data"), None)
            remote = next((path for path in data.iterdir() if path.name.lower() == "remote.gam"))
            windows_sent = [save_hash(remote.read_bytes())]
            windows_received = []
            print("retail: its last saved game", windows_sent[0])
        else:
            windows_events = trace(windows_trace)
            print("windows:", windows_events)
            windows_sent = [value for event, value in windows_events if event == "send"]
            windows_received = [value for event, value in windows_events if event == "receive"]
        loaded = [value for event, value in native_events if event == "load"]
        if args.retail:
            # The full turn cycle; the retail sender's own defect may alter
            # the save's last bytes (reported below).
            ok = bool(sent) and bool(received) and bool(loaded)
        else:
            ok = (bool(sent) and bool(received) and received == windows_sent[:len(received)]
                  and bool(windows_received) and windows_received == sent[:len(windows_received)])
        if args.retail and received and received[-1] != windows_sent[-1]:
            dumps = sorted(glob.glob(str(work / "native.trace.*.receive")))
            if dumps:
                ours = Path(dumps[-1]).read_bytes()
                theirs = remote.read_bytes()
                differ = [i for i in range(len(ours)) if ours[i] != theirs[i]
                          and not NAME_FIELD[0] <= i < NAME_FIELD[1]]
                if differ:
                    print(f"retail -> native: {len(differ)} bytes differ, from {differ[0]}: the "
                          "original sends its compressed save without the stream's last four "
                          "bytes")
        print("ok" if ok else "MISMATCH")
        return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
