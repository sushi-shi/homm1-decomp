#!/usr/bin/env python3
"""Drives the browser build in headless Chromium or Firefox (Playwright).

    web_smoke.py SITE --data GAME_FOLDER [--cd CD_FOLDER] [--help-file HLP]
                 [--browser chromium|firefox] [--out DIR]

SITE is the built page (`nix build .#wasm`: result/share/homm1-web). The
script serves it on a local port and, like a player would:

1. opens the page and gives it the game folder (and the CD music and help
   file when named) through the page's own file inputs;
2. starts the game with /I0, reaches the main menu, starts a standard game
   and waits for the adventure map;
3. reloads the page: the files must still be there (IndexedDB); starts the
   game with its intro movie and checks that audio is running after the
   click that started it;
4. loads the shipped saved game, saves it again and reloads: the saved file
   must have been written back to the browser's storage;
5. opens the scenario editor on the same files.

Screenshots of each step go to --out. Clicks are made on the page at the
places where the game's buttons are, converted from the game's 640x480
coordinates through the canvas's on-page size, so they also check the
pointer mapping. Exits 77 when Playwright or the data are missing. Game data
is read from the local folder given and never copied anywhere else.
"""
import argparse
import functools
import hashlib
import http.server
import os
import sys
import threading
import time
from pathlib import Path

NEW_GAME = (497, 104)
LOAD_GAME = (497, 170)
STANDARD_GAME = (497, 104)
SCENARIO_OK = (382, 437)
FIRST_SAVE = (468, 63)
LOAD_OK = (392, 305)
FILE_OPTIONS = (604, 378)
SAVE_GAME = (252, 140)
SAVE_OK = (243, 330)
SAVE = "________.GM1"


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "text/javascript",
    }

    def log_message(self, format, *args):
        pass


def serve(site: Path) -> tuple[http.server.ThreadingHTTPServer, str]:
    server = http.server.ThreadingHTTPServer(
        ("127.0.0.1", 0), functools.partial(Handler, directory=str(site)))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server, f"http://127.0.0.1:{server.server_address[1]}/"


class Session:
    def __init__(self, page, out: Path, prefix: str):
        self.page = page
        self.out = out
        self.prefix = prefix
        self.count = 0

    def shot(self, name: str) -> Path:
        self.count += 1
        path = self.out / f"{self.prefix}-{self.count:02d}-{name}.png"
        self.page.screenshot(path=str(path))
        print(f"  screenshot {path}")
        return path

    def canvas_point(self, x: int, y: int) -> tuple[float, float]:
        """Page coordinates of game display point (x, y)."""
        box = self.page.evaluate("""() => {
            const c = document.getElementById('canvas');
            const r = c.getBoundingClientRect();
            return {left: r.left, top: r.top, width: r.width, height: r.height,
                    pixelWidth: c.width, pixelHeight: c.height};
        }""")
        scale = box["pixelWidth"] / 640
        bar = round(box["pixelHeight"] / scale) - 480
        page_x = box["left"] + (x + 0.5) * box["width"] / 640
        page_y = box["top"] + (y + bar + 0.5) * box["height"] / (480 + bar)
        return page_x, page_y

    def click(self, point: tuple[int, int], wait: float):
        x, y = self.canvas_point(*point)
        self.page.mouse.move(x, y)
        self.page.wait_for_timeout(150)
        self.page.mouse.down()
        self.page.wait_for_timeout(80)
        self.page.mouse.up()
        self.page.wait_for_timeout(int(wait * 1000))

    def log_text(self) -> str:
        return self.page.evaluate("() => document.getElementById('log').textContent")

    def check_running(self):
        ended = self.page.evaluate("() => { const e = document.getElementById('ended'); return e.hidden ? '' : e.textContent; }")
        if ended:
            raise RuntimeError(f"the program stopped: {ended}\n{self.log_text()}")


def open_page(browser, url: str):
    context = browser.new_context(viewport={"width": 1100, "height": 900})
    return context


def wait_status(page, text: str, timeout: float = 120):
    page.wait_for_function(
        "(t) => document.getElementById('status').textContent.includes(t)", arg=text,
        timeout=timeout * 1000)


def display_signature(page) -> str:
    """A fingerprint of the canvas as shown, to tell screens apart."""
    return hashlib.sha256(page.locator("#canvas").screenshot()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("site", type=Path)
    parser.add_argument("--data", type=Path, default=os.environ.get("HOMM1_DATA"))
    parser.add_argument("--cd", type=Path)
    parser.add_argument("--help-file", type=Path)
    parser.add_argument("--browser", default="chromium", choices=["chromium", "firefox"])
    parser.add_argument("--out", type=Path, default=Path("web-smoke"))
    parser.add_argument("--headed", action="store_true")
    args = parser.parse_args()
    try:
        from playwright.sync_api import sync_playwright
    except ImportError:
        print("skipped: needs the playwright Python package")
        return 77
    if args.data is None or not args.data.is_dir():
        print("skipped: needs --data or HOMM1_DATA")
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    server, url = serve(args.site.resolve())
    print(f"serving {args.site} at {url}")
    extras = []
    if args.cd is not None:
        tracks = next((p for p in args.cd.iterdir() if p.name.lower() == "tracks"), args.cd)
        extras += sorted(p for p in tracks.iterdir() if p.suffix.lower() == ".ogg")
    if args.help_file is not None:
        extras += [args.help_file]
        cnt = args.help_file.with_suffix(".CNT")
        if cnt.exists():
            extras.append(cnt)

    with sync_playwright() as playwright:
        launcher = getattr(playwright, args.browser)
        browser = launcher.launch(headless=not args.headed)
        context = browser.new_context(viewport={"width": 1100, "height": 900})
        page = context.new_page()
        page.on("console", lambda message: None)
        page.on("pageerror", lambda error: print(f"  page error: {error}"))

        # 1. Give the page the files.
        print("1. storing the game files")
        page.goto(url + "?quiet=1&args=/I0")
        wait_status(page, "No game files are stored")
        started = time.monotonic()
        page.set_input_files("#pick-folder", str(args.data))
        wait_status(page, "The game files are stored", timeout=300)
        if extras:
            page.evaluate("() => { document.getElementById('progress').textContent = ''; }")
            page.set_input_files("#pick-files", [str(p) for p in extras])
            page.wait_for_function(
                "() => document.getElementById('progress').textContent.startsWith('Stored')",
                timeout=300000)
            wait_status(page, "The game files are stored")
        status = page.evaluate("() => document.getElementById('status').textContent")
        print(f"  {status} ({time.monotonic() - started:.1f} s)")
        if args.cd is not None and "CD music" not in status:
            raise RuntimeError("the CD music was not stored")
        if args.help_file is not None and "help file" not in status:
            raise RuntimeError("the help file was not stored")

        # 2. Main menu, new game, adventure map.
        print("2. new game to the adventure map")
        session = Session(page, args.out, args.browser)
        page.click("#play")
        page.wait_for_timeout(6000)
        session.check_running()
        menu = display_signature(page)
        session.shot("main-menu")
        session.click(NEW_GAME, 1.5)
        session.click(STANDARD_GAME, 2.5)
        session.shot("scenario-list")
        session.click(SCENARIO_OK, 12)
        session.check_running()
        adventure = display_signature(page)
        session.shot("adventure-map")
        if adventure == menu:
            raise RuntimeError("the display did not change after starting a game")
        # Scroll the map with the keyboard and open the adventure options.
        page.keyboard.press("ArrowRight")
        page.wait_for_timeout(1500)
        session.shot("adventure-map-scrolled")

        # 3. Reload: the stored files are still there; the intro plays with
        # sound.
        print("3. reload, intro movie and audio")
        page.goto(url + "?quiet=1")
        wait_status(page, "The game files are stored")
        page.click("#play")
        page.wait_for_timeout(2500)
        session.check_running()
        session.shot("intro-movie")
        audio = page.evaluate(
            "() => Module.SDL3 && Module.SDL3.audioContext ? Module.SDL3.audioContext.state : 'none'")
        print(f"  audio context: {audio}")
        if audio != "running":
            # Without an audio device (headless Firefox has none here) no
            # context ever runs; tell that apart from a page that failed to
            # unlock audio.
            probe = context.new_page()
            probe.set_content("<button id=b onclick='window.c = new AudioContext(); c.resume()'>a</button>")
            probe.click("#b")
            probe.wait_for_timeout(1000)
            baseline = probe.evaluate("() => window.c.state")
            probe.close()
            if baseline == "running":
                raise RuntimeError(f"audio is {audio} after the click that started the game")
            print(f"  (no audio device: a context made in a click is {baseline} too)")

        # 4. Load the shipped save and save it again.
        print("4. load and save the shipped saved game")
        page.goto(url + "?quiet=1&args=/I0")
        wait_status(page, "The game files are stored")
        before = page.evaluate(f"""() => {{
            const dir = Module.FS.readdir('/homm1/game').find(n => n.toLowerCase() === 'games');
            const st = Module.FS.stat('/homm1/game/' + dir + '/{SAVE}');
            return st.mtime.getTime();
        }}""")
        page.click("#play")
        page.wait_for_timeout(6000)
        session.click(LOAD_GAME, 1.5)
        session.click(STANDARD_GAME, 2.0)
        session.click(FIRST_SAVE, 1.0)
        session.shot("load-list")
        session.click(LOAD_OK, 8)
        session.check_running()
        session.shot("loaded-save")
        session.click(FILE_OPTIONS, 2)
        session.click(SAVE_GAME, 2)
        session.shot("save-dialog")
        session.click(SAVE_OK, 3)
        session.shot("after-save")
        page.wait_for_timeout(2000)
        page.goto(url + "?quiet=1")
        wait_status(page, "The game files are stored")
        after = page.evaluate(f"""() => {{
            const dir = Module.FS.readdir('/homm1/game').find(n => n.toLowerCase() === 'games');
            const st = Module.FS.stat('/homm1/game/' + dir + '/{SAVE}');
            return st.mtime.getTime();
        }}""")
        print(f"  save written at {before} before, {after} after reloading")
        if after <= before:
            raise RuntimeError("the saved game did not reach the browser's storage")

        # 5. The editor on the same files.
        print("5. the scenario editor")
        page.goto(url + "?program=editor&quiet=1")
        wait_status(page, "The game files are stored")
        page.click("#play")
        page.wait_for_timeout(8000)
        session.check_running()
        session.shot("editor")

        log = session.log_text()
        (args.out / f"{args.browser}-log.txt").write_text(log)
        browser.close()
    server.shutdown()
    print("ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
