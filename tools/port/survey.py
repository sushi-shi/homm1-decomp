#!/usr/bin/env python3
"""The long-running sanitizer survey of the native port (docs/port/README.md).

Runs a sanitizer build of the game headless over every shipped map, the
campaign openings, the shipped saved games and many random battles, and of
the game and the editor under random input, and reports every distinct
sanitizer finding, hang and failed check with a command that reproduces it.

    cmake -S . -B build/port-asan -G Ninja -DHOMM1_SANITIZERS=ON \\
          -DHOMM1_SANITIZERS_RECOVER=ON -DHOMM1_SURVEY=ON
    ninja -C build/port-asan
    tools/port/survey.py --build build/port-asan --data ~/.local/share/homm1-buka/game

Parts (--parts, default all but monkey-editor):
  ai        every map, --seeds seeds each, --days days, all players computer
  campaign  each campaign scenario's opening
  load      the shipped saved games, continued by the computer
  combat    --battles random battles per map seed on the combat screen
  editor    random maps from the editor's generator (--seeds runs of
            --random-maps maps), each saved, read and saved again, then
            played by the computer like the shipped maps
  monkey    the game under random clicks and keys (HOMM1_INPUT_REPLAY)
  monkey-editor  the editor under random clicks and keys

Findings are grouped by the reporting source line; each group lists how
often it was seen and the first run that showed it. The exit status is 1
when anything was found.
"""
import argparse
import concurrent.futures
import os
import random
import re
import shutil
import signal
import subprocess
import sys
import tempfile
from pathlib import Path

SANITIZER_LINE = re.compile(r"^(?P<where>\S+?:\d+(?::\d+)?): runtime error: (?P<what>.*)$")
ASAN_LINE = re.compile(r"ERROR: AddressSanitizer: (?P<what>[\w-]+)")
FRAME = re.compile(r"^\s+#(?P<n>\d+) 0x[0-9a-f]+ in (?P<function>.+?) (?P<where>\S+:\d+)")


def find(directory: Path, name: str) -> Path | None:
    for entry in directory.iterdir():
        if entry.name.lower() == name.lower():
            return entry
    return None


def prepare_game_folder(data: Path, scratch: Path, extra_maps: Path | None = None) -> Path:
    """A writable game folder: DATA, ANIM and SOUND linked, MAPS and GAMES copied."""
    root = scratch / "game"
    root.mkdir(parents=True, exist_ok=True)
    for name in ("DATA", "ANIM", "SOUND"):
        source = find(data, name)
        if source is not None and not (root / source.name).exists():
            (root / source.name).symlink_to(source)
    for name in ("MAPS", "GAMES"):
        source = find(data, name)
        target = root / (source.name if source else name.capitalize())
        if source is not None and not target.exists():
            shutil.copytree(source, target)
        target.mkdir(exist_ok=True)
    if extra_maps is not None and extra_maps.is_dir():
        maps = find(root, "MAPS")
        for map_file in extra_maps.iterdir():
            shutil.copy(map_file, maps / map_file.name)
    return root


def answer_replay(path: Path, interval: int = 250, count: int = 100000) -> None:
    """Answers message boxes (Return) and the battle result windows, whose
    only way out is their button near the bottom centre."""
    actions = ["key return", "click 320 430", "key return", "click 320 445",
               "key return", "click 320 415", "key return", "click 320 400"]
    path.write_text("\n".join(f"+{interval} {actions[i % len(actions)]}" for i in range(count)) + "\n")


# The game's screens are 640x480; clicks land anywhere, with more weight on
# the adventure map's right panel and the dialog buttons in the middle.
MONKEY_KEYS = ["return", "escape", "space", "e", "h", "t", "c", "s", "v", "m", "a", "d", "q",
               "up", "down", "left", "right", "1", "2", "3", "4", "5", "tab", "f1", "pageup",
               "pagedown", "home", "end", "n", "y", "l", "i", "o", "w", "f4", "b", "r", "u"]


def monkey_replay(path: Path, seed: int, actions: int, start: str = "") -> None:
    rng = random.Random(seed)
    lines = [start] if start else []
    for _ in range(actions):
        roll = rng.random()
        wait = rng.choice([30, 80, 150, 300, 600])
        if roll < 0.55:
            x = rng.randrange(640)
            y = rng.randrange(480)
            lines.append(f"+{wait} click {x} {y}")
        elif roll < 0.65:
            x = rng.randrange(640)
            y = rng.randrange(480)
            lines.append(f"+{wait} right-down {x} {y}")
            lines.append(f"+{rng.choice([50, 300])} right-up {x} {y}")
        elif roll < 0.72:
            lines.append(f"+{wait} move {rng.randrange(640)} {rng.randrange(480)}")
        elif roll < 0.74:
            # The menu bar above the picture (negative y), shown in a window.
            lines.append(f"+{wait} click {rng.randrange(300)} {-rng.randrange(1, 19)}")
            lines.append(f"+{rng.choice([200, 400])} click {rng.randrange(400)} {rng.randrange(200)}")
        elif roll < 0.78:
            # Toward a dialog's OK / Cancel buttons.
            lines.append(f"+{wait} click {rng.choice([250, 320, 390])} {rng.choice([300, 330, 360, 410])}")
        else:
            lines.append(f"+{wait} key {rng.choice(MONKEY_KEYS)}")
    lines.append("+1000 exit")
    path.write_text("\n".join(lines) + "\n")


def parse(output: str) -> list[tuple[str, str, str]]:
    """(kind, where, text) for each finding in a run's output."""
    findings = []
    lines = output.splitlines()
    for index, line in enumerate(lines):
        match = SANITIZER_LINE.match(line)
        if match:
            frames = []
            for follow in lines[index + 1:index + 12]:
                frame = FRAME.match(follow)
                if frame:
                    frames.append(f"{frame['function']} {frame['where']}")
                elif frames:
                    break
            where = re.sub(r"^.*/localized/", "", match["where"])
            findings.append(("ubsan", where, match["what"] + ("\n      " + "\n      ".join(frames[:6]) if frames else "")))
            continue
        match = ASAN_LINE.search(line)
        if match:
            frames = []
            for follow in lines[index + 1:index + 30]:
                frame = FRAME.match(follow)
                if frame:
                    frames.append(f"{frame['function']} {frame['where']}")
                elif frames:
                    break
            where = re.sub(r"^.*/localized/", "", frames[0].split()[-1]) if frames else "?"
            # Skip the sanitizer's own interceptor frames for the key.
            for frame in frames:
                location = frame.split()[-1]
                if "/localized/" in location or "/tests/" in location:
                    where = re.sub(r"^.*/localized/", "", location)
                    break
            findings.append(("asan " + match["what"], where, "\n      ".join(frames[:8])))
        if line.startswith("FINDING"):
            findings.append(("check", line.split(":")[0], line))
    return findings


def last_progress(output: str) -> str:
    for line in reversed(output.splitlines()):
        if line and not line.startswith("[homm1]") and "runtime error" not in line and not line.startswith(" "):
            return line[:200]
    return ""


def run(job: dict) -> dict:
    scratch = Path(tempfile.mkdtemp(prefix="homm1-survey-", dir=job["scratch"]))
    try:
        root = prepare_game_folder(Path(job["data"]), scratch,
                                   Path(job["extra_maps"]) if job.get("extra_maps") else None)
        replay = scratch / "input.replay"
        if job.get("monkey") is not None:
            monkey_replay(replay, job["monkey"], job["actions"], job.get("start", ""))
        else:
            answer_replay(replay)
        environment = dict(os.environ)
        environment.update({
            "HOMM1_DATA": str(root),
            "HOMM1_INPUT_REPLAY": str(replay),
            "HOMM1_NO_DIALOGS": "1",
            "HOMM1_TIME_SCALE": str(job["time_scale"]),
            "HOMM1_SURVEY_WATCHDOG": str(job.get("watchdog", 300)),
            "SDL_AUDIO_DRIVER": "dummy",
            "XDG_CONFIG_HOME": str(scratch / "config"),
            "ASAN_OPTIONS": "detect_leaks=0:abort_on_error=0:print_summary=1:handle_abort=1",
            "UBSAN_OPTIONS": "print_stacktrace=1",
        })
        command = list(job["command"])
        if job.get("xvfb") and shutil.which("xvfb-run"):
            # A real (virtual) display, so that full screen and window sizes
            # take effect.
            command = ["xvfb-run", "-a", "-s", "-screen 0 1280x1024x24"] + command
        else:
            environment["SDL_VIDEODRIVER"] = "dummy"
        # Output goes to a file (a run can print a lot); the whole process
        # group is killed on timeout, xvfb-run and its server included.
        log_path = scratch / "output.log"
        with open(log_path, "wb") as log:
            process = subprocess.Popen(command, env=environment, cwd=scratch, stdout=log,
                                       stderr=subprocess.STDOUT, start_new_session=True)
            try:
                status = process.wait(timeout=job["timeout"])
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
                status = None
        output = log_path.read_text(errors="replace")
        findings = parse(output)
        if status is None and not job.get("expect_timeout"):
            findings.append(("hang", job["name"], "no progress after: " + last_progress(output)))
        elif status not in (None, 0) and not any(kind.startswith("asan") for kind, _, _ in findings):
            findings.append(("exit", job["name"], f"exit status {status} after: {last_progress(output)}"))
        if job.get("keep_log"):
            (Path(job["scratch"]) / (job["name"].replace("/", "_") + ".log")).write_text(output)
        return {"job": job, "findings": findings, "status": status}
    finally:
        shutil.rmtree(scratch, ignore_errors=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build", required=True, type=Path, help="a sanitizer build with -DHOMM1_SURVEY=ON")
    parser.add_argument("--data", type=Path, default=os.environ.get("HOMM1_DATA"))
    parser.add_argument("--parts", default="ai,campaign,load,combat,monkey")
    parser.add_argument("--maps", default="", help="comma-separated map names (default: every shipped map)")
    parser.add_argument("--seeds", type=int, default=2)
    parser.add_argument("--days", type=int, default=60)
    parser.add_argument("--battles", type=int, default=25)
    parser.add_argument("--random-maps", type=int, default=10, help="maps per editor generator run")
    parser.add_argument("--monkeys", type=int, default=8, help="random-input runs per program")
    parser.add_argument("--actions", type=int, default=3000, help="random actions per monkey run")
    parser.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) // 2))
    parser.add_argument("--time-scale", type=int, default=20)
    parser.add_argument("--timeout", type=int, default=1800)
    parser.add_argument("--scratch", type=Path, default=None)
    parser.add_argument("--keep-logs", action="store_true")
    arguments = parser.parse_args()
    if arguments.data is None:
        parser.error("--data or $HOMM1_DATA is required")
    build = arguments.build.resolve()
    survey = build / "tests" / "port" / "homm1_survey"
    editor_survey = build / "tests" / "port" / "homm1_editor_survey"
    game = build / "heroes"
    editor = build / "heroes-editor"
    scratch = Path(arguments.scratch or tempfile.mkdtemp(prefix="homm1-survey-")).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    data = arguments.data.resolve()
    maps_folder = find(data, "MAPS")
    maps = sorted(p.name for p in maps_folder.iterdir() if p.suffix.upper() == ".MAP")
    if arguments.maps:
        maps = arguments.maps.split(",")
    games_folder = find(data, "GAMES")
    saves = sorted(p.name for p in games_folder.iterdir() if p.suffix.upper().startswith(".GM")) if games_folder else []
    parts = set(arguments.parts.split(","))

    base = {"data": str(data), "scratch": str(scratch), "time_scale": arguments.time_scale,
            "timeout": arguments.timeout, "keep_log": arguments.keep_logs}
    jobs = []
    if "ai" in parts:
        for name in maps:
            for seed in range(1, arguments.seeds + 1):
                players = 2 + (seed % 3)
                jobs.append(dict(base, name=f"ai {name} seed {seed}", command=[
                    str(survey), "ai", name, str(arguments.days), str(seed), str(players)]))
    if "campaign" in parts:
        for side in range(1, 5):
            for scenario in range(0, 9):
                jobs.append(dict(base, name=f"campaign {side} {scenario}", command=[
                    str(survey), "campaign", str(side), str(scenario), "10", str(side * 10 + scenario)]))
    if "load" in parts:
        for save in saves:
            jobs.append(dict(base, name=f"load {save}", command=[str(survey), "load", save, "21", "1"]))
    if "combat" in parts:
        for index, name in enumerate(maps):
            for seed in range(1, arguments.seeds + 1):
                jobs.append(dict(base, name=f"combat {name} seed {seed}", command=[
                    str(survey), "combat", name, str(arguments.battles), str(seed * 1000 + index)]))
    # The monkeys start a new game (or the editor) and then click at random.
    new_game = "3000 click 497 104\n+1500 click 497 104\n+2000 click 382 437\n+6000 move 320 240"
    if "monkey" in parts:
        for seed in range(arguments.monkeys):
            jobs.append(dict(base, name=f"monkey game {seed}", monkey=seed, actions=arguments.actions,
                             start=new_game if seed % 4 else "", time_scale=4, xvfb=True,
                             timeout=arguments.timeout,
                             command=[str(game), "/I0"]))
    if "monkey-editor" in parts:
        for seed in range(arguments.monkeys):
            jobs.append(dict(base, name=f"monkey editor {seed}", monkey=10000 + seed,
                             actions=arguments.actions, time_scale=4, xvfb=True,
                             timeout=arguments.timeout,
                             command=[str(editor)]))

    generated = scratch / "generated"
    phases = [jobs]
    if "editor" in parts:
        generated.mkdir(exist_ok=True)
        editor_jobs = [dict(base, name=f"editor random seed {seed}", command=[
            str(editor_survey), "random", str(seed), str(arguments.random_maps), str(generated)])
            for seed in range(1, arguments.seeds + 1)]
        phases.insert(0, editor_jobs)
    groups: dict[tuple[str, str], dict] = {}

    def run_phase(phase: list[dict]) -> None:
        print(f"{len(phase)} runs, {arguments.jobs} at a time; scratch {scratch}", flush=True)
        done = 0
        with concurrent.futures.ThreadPoolExecutor(max_workers=arguments.jobs) as pool:
            for outcome in pool.map(run, phase):
                done += 1
                job = outcome["job"]
                new = 0
                for kind, where, text in outcome["findings"]:
                    key = (kind, where)
                    if key not in groups:
                        groups[key] = {"count": 0, "first": job, "text": text}
                        new += 1
                    groups[key]["count"] += 1
                print(f"[{done}/{len(phase)}] {job['name']}: status {outcome['status']}, "
                      f"{len(outcome['findings'])} findings ({new} new)", flush=True)

    for index, phase in enumerate(phases):
        if index == 1 and "editor" in parts:
            # The generated maps, played by the computer.
            for map_file in sorted(generated.iterdir()):
                phase.append(dict(base, name=f"ai {map_file.name} (generated)", extra_maps=str(generated),
                                  command=[str(survey), "ai", map_file.name, str(arguments.days), "1", "4"]))
        run_phase(phase)

    print(f"\n{len(groups)} distinct findings")
    for (kind, where), group in sorted(groups.items(), key=lambda item: (item[0][0], item[0][1])):
        print(f"\n[{kind}] {where}  (seen {group['count']}x; first: {group['first']['name']})")
        print("      " + group["text"])
        print("      reproduce: " + " ".join(group["first"]["command"]))
    return 1 if groups else 0


if __name__ == "__main__":
    sys.exit(main())
