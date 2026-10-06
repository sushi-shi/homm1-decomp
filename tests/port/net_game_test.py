#!/usr/bin/env python3
"""Two native instances play network, direct-connection and modem games on
localhost, driven by scripted input, and must agree on the game state.

    net_game_test.py HEROES_BINARY [--keep DIR]

Needs $HOMM1_DATA (the game folder) and xvfb-run; without them the test is
skipped (exit 77). Each instance runs headless in its own copy of the game
folder with HOMM1_NET_TRACE set: every saved game it sends, receives and
loads, and every battle it fights with the other side, appends a line with a
hash of what both peers must agree on (REMOTE.h).

1. A new network game: the host sets up the first scenario, the save goes to
   the guest, the host ends its turn, the AI players move, the guest ends its
   turn and the save comes back.
2. A battle: from the host's first save of that game a copy is made with the
   guest's hero (given a single peasant) next to the host's hero and the
   computer players retired. Both load it as a network game; the host
   attacks, both sides fight on auto combat, then each ends its turns until
   the time is up.
3. The same start as 1 over a direct connection and over the modem.

At every hand-off the hashes of the save sent, received and loaded must be
equal, and a battle's outcome (result, armies as fought, heroes) must hash
the same on both sides.
"""
from __future__ import annotations

import argparse
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

# Display coordinates (the 640x480 game image).
NEW_GAME = (497, 104)
LOAD_GAME = (497, 170)
CHOICE = {1: (497, 104), 2: (497, 170), 3: (497, 236), 4: (497, 302)}
SCENARIO_OK = (382, 437)
FILE_FIRST = (468, 63)
FILE_OK = (392, 305)
END_TURN = (505, 370)
CONFIRM_YES = (168, 163)
COMBAT_AUTO = (20, 470)
RESULT_OK = (304, 429)
# The guest's hero is placed one cell down and right of the host's, which the
# view centres on cell (7, 7) of 32-pixel cells.
ATTACK = (272, 272)

HERO = 182
TOWN = 55
CELL = 10
GRID = 72
HEROES = 36
TOWNS = 36
TRIGGER_HERO = 0x80 | 61
CREATURE_PEASANT = 0
# The saved game's dead player count and flags.
DEAD_COUNT = 227
DEAD_FLAGS = 228


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


def replay(*steps: tuple) -> str:
    """Steps are (delay_ms, action, *arguments); the first delay is absolute."""
    lines = []
    for index, (delay, action, *arguments) in enumerate(steps):
        when = str(delay) if index == 0 else f"+{delay}"
        lines.append(" ".join([when, action, *map(str, arguments)]))
    return "\n".join(lines) + "\n"


def click(delay: int, point: tuple[int, int]) -> tuple:
    return (delay, "click", point[0], point[1])


def find(directory: Path, name: str) -> Path | None:
    for entry in directory.iterdir():
        if entry.name.lower() == name.lower():
            return entry
    return None


class Instance:
    def __init__(self, root: Path, name: str, data: Path):
        self.root = root / name
        self.name = name
        self.game = self.root / "game"
        shutil.copytree(data, self.game, ignore=shutil.ignore_patterns(
            "*.exe", "*.EXE", "*.dll", "*.DLL", "*.GM*", "*.gm*", "*.CGM", "*.cgm"))
        self.trace = self.root / "net.trace"
        self.log = self.root / "log.txt"
        self.process: subprocess.Popen | None = None

    def start(self, binary: Path, script: str, arguments: list[str], timeout: int) -> None:
        (self.root / "input.replay").write_text(script)
        self.trace.write_text("")
        environment = dict(
            os.environ, HOMM1_DATA=str(self.game), HOMM1_INPUT_REPLAY=str(self.root / "input.replay"),
            HOMM1_NO_DIALOGS="1", SDL_AUDIO_DRIVER="dummy", HOMM1_NET_TRACE=str(self.trace),
            HOMM1_NET_TRACE_DUMP="1", XDG_CONFIG_HOME=str(self.root / "config"),
            ASAN_OPTIONS=os.environ.get("ASAN_OPTIONS", "detect_leaks=0"))
        self.process = subprocess.Popen(
            ["timeout", str(timeout), "xvfb-run", "-a", "-s", "-screen 0 1024x768x24",
             str(binary), "/I0", *arguments],
            env=environment, cwd=self.root, stdout=self.log.open("w"), stderr=subprocess.STDOUT)

    def wait(self) -> int:
        assert self.process is not None
        return self.process.wait()

    def events(self) -> list[tuple[str, str]]:
        result = []
        for line in self.trace.read_text().splitlines():
            fields = dict(field.split("=", 1) for field in line.split()[1:])
            result.append((line.split()[0], fields["hash"]))
        return result


def run_pair(binary: Path, host: Instance, guest: Instance, host_script: str, guest_script: str,
             host_arguments: list[str], guest_arguments: list[str], timeout: int) -> bool:
    host.start(binary, host_script, host_arguments, timeout)
    time.sleep(2)
    guest.start(binary, guest_script, guest_arguments, timeout)
    results = (host.wait(), guest.wait())
    for instance, result in zip((host, guest), results):
        if result != 0:
            print(f"{instance.name} ended with {result}; its log:")
            print(instance.log.read_text()[-3000:])
            return False
    return True


def check_handoffs(phase: str, sender: Instance, receiver: Instance, minimum: int) -> list[str]:
    """Every save the sender sent must arrive and load with the same hash."""
    sent = [value for event, value in sender.events() if event == "send"]
    received = [value for event, value in receiver.events() if event == "receive"]
    loaded = [value for event, value in receiver.events() if event == "load"]
    errors = []
    count = min(len(sent), len(received))
    if count < minimum:
        errors.append(f"{phase}: {sender.name} -> {receiver.name}: {count} hand-offs, "
                      f"expected at least {minimum} (sent {len(sent)}, received {len(received)})")
    for index in range(count):
        if sent[index] != received[index]:
            errors.append(f"{phase}: hand-off {index} {sender.name} -> {receiver.name}: sent "
                          f"{sent[index]}, received {received[index]}")
        if index < len(loaded) and loaded[index] != received[index]:
            errors.append(f"{phase}: hand-off {index} on {receiver.name}: received "
                          f"{received[index]}, state after loading {loaded[index]}")
    return errors


def check_battles(phase: str, host: Instance, guest: Instance, minimum: int) -> list[str]:
    ours = [value for event, value in host.events() if event == "combat"]
    theirs = [value for event, value in guest.events() if event == "combat"]
    errors = []
    if min(len(ours), len(theirs)) < minimum:
        errors.append(f"{phase}: {len(ours)} battles on the host, {len(theirs)} on the guest, "
                      f"expected at least {minimum}")
    if ours != theirs:
        errors.append(f"{phase}: battle outcomes differ: host {ours}, guest {theirs}")
    return errors


def craft_battle(save: bytes) -> bytes:
    """Moves player 1's hero next to player 0's, leaves it one peasant and
    retires the computer players, so that the game is the two humans'."""
    data = bytearray(save)
    heroes = next(base for base in range(len(data) - HEROES * HERO)
                  if all(data[base + i * HERO] == i for i in range(HEROES)))
    world = heroes - 1 - GRID * GRID * CELL
    towns = heroes + HEROES * HERO + HEROES

    def cell(x: int, y: int) -> int:
        return world + (x * GRID + y) * CELL

    def hero_of(player: int) -> int:
        for i in range(HEROES):
            offset = heroes + i * HERO
            if data[offset + 1] == player and data[offset + 30] and data[offset + 31]:
                return offset
        raise SystemExit(f"the save has no hero of player {player}")

    host, guest = hero_of(0), hero_of(1)
    hx, hy = data[host + 30], data[host + 31]
    target = (hx + 1, hy + 1)
    free = cell(*target)
    if data[free] < 20 or data[free + 2] != 0xff or data[free + 4] != 0xff or data[free + 8]:
        raise SystemExit("the cell down and right of the host's hero is not free land")
    gx, gy = data[guest + 30], data[guest + 31]
    old = cell(gx, gy)
    data[old + 8], data[old + 9] = data[guest + 35], data[guest + 36]
    for t in range(TOWNS):
        town = towns + t * TOWN
        if data[town + 4] == gx and data[town + 5] == gy and data[town + 21] == data[guest]:
            data[town + 21] = 0xff
    data[guest + 35], data[guest + 36] = data[free + 8], data[free + 9]
    data[free + 8], data[free + 9] = TRIGGER_HERO, data[guest]
    data[guest + 30], data[guest + 31] = target
    army = guest + 87
    data[army:army + 5] = bytes([CREATURE_PEASANT, 0xff, 0xff, 0xff, 0xff])
    data[army + 5:army + 15] = bytes([1, 0] + [0] * 8)
    data[DEAD_COUNT] = 2
    data[DEAD_FLAGS:DEAD_FLAGS + 4] = bytes([0, 0, 1, 1])
    return bytes(data)


def multiplayer(start: tuple[int, int], connection: int, role: int) -> list[tuple]:
    return [click(3000, start), click(1500, CHOICE[3]), click(1500, CHOICE[connection]),
            click(1500, CHOICE[role])]


def serial_setup(role: int) -> list[tuple]:
    # COM 1 and 9600 baud; the transport ignores both.
    return [click(1500, CHOICE[1]), click(1500, CHOICE[2])]


def phase_new_game(binary: Path, root: Path, data: Path, connection: str) -> tuple[list[str], bytes]:
    host, guest = Instance(root, f"{connection}-host", data), Instance(root, f"{connection}-guest", data)
    port = free_port()
    host_arguments = ["--port", str(port)]
    guest_arguments = ["--port", str(port), "--join", f"127.0.0.1:{port}"]
    if connection == "network":
        host_steps = multiplayer(NEW_GAME, 2, 1)
        guest_steps = multiplayer(NEW_GAME, 2, 2)
    elif connection == "direct":
        host_steps = multiplayer(NEW_GAME, 4, 1) + serial_setup(1)
        guest_steps = multiplayer(NEW_GAME, 4, 2) + serial_setup(2)
    else:
        # The modem: the init string as offered, then a number of digits
        # only, which dials --join. The guest's modem answers.
        host_steps = multiplayer(NEW_GAME, 3, 1) + serial_setup(1) + [
            (300, "key", "enter"), (1500, "key", "1"), (300, "key", "enter")]
        guest_steps = multiplayer(NEW_GAME, 3, 2) + serial_setup(2) + [(300, "key", "enter")]
        host_arguments = ["--port", str(port), "--join", f"127.0.0.1:{port}"]
        guest_arguments = ["--port", str(port)]
    if connection == "network":
        # The full turn cycle: the host ends its turn (confirming, its hero
        # can still move), the AI players move, the guest ends its turn.
        host_script = replay(*host_steps, click(8000, SCENARIO_OK), click(12000, END_TURN),
                             click(1500, CONFIRM_YES), (45000, "exit"))
        guest_script = replay(*guest_steps, click(40000, END_TURN), click(1500, CONFIRM_YES),
                              (20000, "exit"))
        minimum_back = 1
    else:
        host_script = replay(*host_steps, click(8000, SCENARIO_OK), (15000, "exit"))
        guest_script = replay(*guest_steps, (28000, "exit"))
        minimum_back = 0
    if not run_pair(binary, host, guest, host_script, guest_script, host_arguments,
                    guest_arguments, 240):
        return [f"{connection}: an instance failed"], b""
    errors = check_handoffs(connection, host, guest, 1)
    errors += check_handoffs(connection, guest, host, minimum_back)
    dumps = sorted(host.root.glob("net.trace.*.send"))
    return errors, dumps[0].read_bytes() if dumps else b""


def phase_battle(binary: Path, root: Path, data: Path, save: bytes) -> list[str]:
    host, guest = Instance(root, "battle-host", data), Instance(root, "battle-guest", data)
    games = find(host.game, "GAMES")
    if games is None:
        games = host.game / "GAMES"
        games.mkdir()
    (games / "NETTEST.GM2").write_bytes(craft_battle(save))
    port = free_port()
    host_steps = multiplayer(LOAD_GAME, 2, 1) + [
        click(6000, FILE_FIRST), click(1000, FILE_OK),
        # Show the route to the guest's hero, then go: the attack.
        click(8000, ATTACK), click(2000, ATTACK),
        click(8000, COMBAT_AUTO), click(20000, RESULT_OK)]
    # Then end the turn whenever it is ours; the hero can always move.
    for _ in range(8):
        host_steps += [click(6500, END_TURN), click(1500, CONFIRM_YES)]
    host_steps.append((5000, "exit"))
    guest_steps = multiplayer(LOAD_GAME, 2, 2) + [click(25000, COMBAT_AUTO)]
    # Auto combat takes effect only on this side's turn: keep asking.
    for _ in range(10):
        guest_steps.append(click(1500, COMBAT_AUTO))
    guest_steps.append(click(3000, RESULT_OK))
    # The guest's hero is gone: ending its turn asks nothing.
    for _ in range(10):
        guest_steps.append(click(6000, END_TURN))
    guest_steps.append((9000, "exit"))
    if not run_pair(binary, host, guest, replay(*host_steps), replay(*guest_steps),
                    ["--port", str(port)], ["--port", str(port), "--join", f"127.0.0.1:{port}"],
                    300):
        return ["battle: an instance failed"]
    errors = check_battles("battle", host, guest, 1)
    errors += check_handoffs("battle", host, guest, 2)
    errors += check_handoffs("battle", guest, host, 1)
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--keep", type=Path, help="run in DIR and keep it")
    args = parser.parse_args()
    data = os.environ.get("HOMM1_DATA")
    if not data or shutil.which("xvfb-run") is None:
        print("skipped: needs HOMM1_DATA and xvfb-run")
        return 77
    binary = args.binary.resolve()
    with tempfile.TemporaryDirectory() as scratch:
        root = args.keep.resolve() if args.keep else Path(scratch)
        root.mkdir(parents=True, exist_ok=True)
        errors, save = phase_new_game(binary, root, Path(data), "network")
        if save:
            errors += phase_battle(binary, root, Path(data), save)
        else:
            errors.append("network: the host sent no save")
        for connection in ("direct", "modem"):
            phase_errors, _ = phase_new_game(binary, root, Path(data), connection)
            errors += phase_errors
        for instance in sorted(root.glob("*/net.trace")):
            print(f"{instance.parent.name}:")
            for line in instance.read_text().splitlines():
                print(f"  {line}")
    if errors:
        for error in errors:
            print(f"FAIL: {error}")
        return 1
    print("ok: both sides agreed at every hand-off and on every battle")
    return 0


if __name__ == "__main__":
    sys.exit(main())
