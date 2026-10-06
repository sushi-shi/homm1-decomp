"""homm1.tool.wine - the shared era-toolchain plumbing.

    homm1 tool wine --init [--force] | --verify | --shutdown

Everything cl/link/rc need to run under wine, in one place: tool lookup,
path translation, the persistent wineserver, prefix initialisation (registry
PATH/INCLUDE/LIB + MSDIS100.DLL for link.exe), and the one hang-proof runner.
Callers above tool/ never see wine.
"""

from __future__ import annotations

import os
import shutil
import signal
import subprocess
import tempfile
from pathlib import Path

from homm1.core.paths import msvc_dir
from homm1.tool import ToolError


#: The time zone of every wine process the tooling starts. A wine client that
#: finds no wineserver for its prefix starts one with its own environment, and
#: the server's zone is the local time of all its clients: the native C
#: runtime of the editor's linker converts that local time back with its own
#: TZ parse (native_crt_linker), so one client started in the host's zone
#: would move the linked image's timestamps by the host's UTC offset.
WINE_ZONE = "UTC"


def wine_env(env: dict[str, str] | None = None) -> dict[str, str]:
    """`env` (default: this process's environment) for a wine process: the
    one place that pins WINE_ZONE for wine, winepath, wineboot and wineserver
    alike, whichever of them ends up starting the prefix's server."""
    return {**(os.environ if env is None else env), "TZ": WINE_ZONE}


#: Display variables a headless wine process must not inherit.
DISPLAY_VARIABLES = ("DISPLAY", "WAYLAND_DISPLAY")
#: The wine debugger's crash dialog switch (0: report on stderr only).
CRASH_DIALOG_KEY = r"HKEY_CURRENT_USER\Software\Wine\WineDbg"


def headless_env(env: dict[str, str] | None = None, *,
                 quiet: bool = True) -> dict[str, str]:
    """wine_env for a program the tooling runs, never shows: no X11 or
    Wayland display, so a window, message box or crash dialog cannot reach
    the user's screen. `quiet` also silences every wine debug channel (a
    test program's output is then its own)."""
    out = {k: v for k, v in wine_env(env).items() if k not in DISPLAY_VARIABLES}
    if quiet:
        out["WINEDEBUG"] = "-all"
    return out


def disable_crash_dialog() -> None:
    """Set the prefix's WineDbg ShowCrashDialog to 0 (idempotent): an
    unhandled exception is then reported on stderr, never in a window."""
    got = _reg("query", CRASH_DIALOG_KEY, "/v", "ShowCrashDialog", capture=True)
    if not any("REG_DWORD" in line and line.split()[-1] in ("0x0", "0")
               for line in got.stdout.splitlines()):
        _reg("add", CRASH_DIALOG_KEY, "/v", "ShowCrashDialog", "/t", "REG_DWORD",
             "/d", "0", "/f")


def find_ci(d: Path, name: str) -> Path | None:
    """Case-insensitive lookup (the toolchain mixes CL.EXE / cl.exe case)."""
    if not d.is_dir():
        return None
    low = name.lower()
    return next((p for p in d.iterdir() if p.name.lower() == low), None)


def require(prog: str) -> str:
    """`prog`'s path on $PATH, or a ToolError naming the fix.

    Everything under tool/ used to spawn wine/winepath straight from a bare
    name, so a shell outside `nix develop` got a FileNotFoundError traceback
    out of subprocess - including from `homm1 tool wine --verify`, which
    exists precisely to answer "is wine set up?".
    """
    hit = shutil.which(prog)
    if hit is None:
        raise ToolError(f"{prog} not found on PATH - run inside `nix develop`")
    return hit


def toolchain_root() -> Path:
    """$MSVC_DIR as a Path, as a ToolError rather than a RuntimeError.

    homm1.core.paths raises RuntimeError, which no tool main() catches; the
    layer's own error type is what the drivers report on.
    """
    try:
        return msvc_dir()
    except RuntimeError as e:
        raise ToolError(str(e)) from e


def era_tool(name: str) -> Path:
    """$MSVC_DIR/bin/<name>, or a ToolError naming the fix."""
    root = toolchain_root()
    p = find_ci(root / "bin", name)
    if p is None:
        # rc.exe and the link.exe DLLs are the two things an older pinned
        # toolchain release actually lacks; naming the release is only honest
        # for those.
        hint = (" (rc.exe arrived in toolchain release r3)"
                if name.lower() == "rc.exe" else "")
        raise ToolError(f"{name} not found under {root}/bin - run "
                        f"`homm1 toolchain install`{hint}")
    require("wine")
    return p


def ensure_link_deps() -> None:
    """Verify the era linker and its local PDB runtime are installed."""
    root = toolchain_root()
    era_tool("link.exe")
    if not any(find_ci(root / "bin", name) for name in ("mspdb40.dll", "mspdb41.dll", "mspdb60.dll")):
        raise ToolError(f"MSVC PDB runtime not found under {root}/bin - reinstall "
                        "the pinned MSVC toolchain")


def winepath(p: Path | str) -> str:
    """Unix path -> windows path. stderr is discarded on purpose: winepath can
    be the call that boots the persistent wine session, and a daemonised
    session inheriting our stderr holds the caller's pipe open forever."""
    exe = require("winepath")
    try:
        return subprocess.check_output([exe, "-w", str(p)], text=True,
                                       env=wine_env(),
                                       stderr=subprocess.DEVNULL).strip()
    except subprocess.CalledProcessError as e:
        raise ToolError(f"winepath -w {p} failed (rc={e.returncode}) - the "
                        "wine prefix may not be initialised; run "
                        "`homm1 init`") from e


def ensure_wineserver() -> None:
    """`wineserver -p60`: keep the server 60s past the last client, so parallel
    `wine cl` invocations under ninja skip the cold start, yet it exits on
    its own afterwards (bare `-p` persisted forever and leaked). Idempotent:
    with a server already running this is a no-op (the new one cannot take
    the prefix's lock and exits), so it never changes a running server's
    zone or persistence. A server this starts runs in WINE_ZONE."""
    ws = shutil.which("wineserver")
    if ws:
        subprocess.run([ws, "-p60"], check=False, env=wine_env(),
                       stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                       stderr=subprocess.DEVNULL)


def shutdown_wineserver() -> None:
    """`wineserver -k`: reap the persistent server (leaked servers slow builds
    and hold deleted files open - kill between long sessions)."""
    ws = shutil.which("wineserver")
    if ws:
        subprocess.run([ws, "-k"], check=False)


def run(argv: list[str], *, cwd: Path | None = None,
        env: dict[str, str] | None = None,
        timeout: float | None = None,
        success: Path | None = None) -> tuple[str, int]:
    """Run one wine tool hang-proof; return (combined output, returncode).

    Wine intermittently leaves a finished-but-unreaped grandchild
    (mspdbsrv/conhost/...) holding the inherited stdio, which wedges a capture
    PIPE forever even though the artifact is already written. So: output to a
    temp FILE, the tool in its own process group, a bounded wait; on a stall
    SIGKILL the group and let `success` (the artifact the caller expects)
    decide the verdict.
    """
    os.environ.setdefault("WINEDEBUG", "fixme-all,err-kerberos")
    env = headless_env(env, quiet=False)
    ensure_wineserver()
    if timeout is None:
        timeout = float(os.environ.get("HOMM1_WINE_TIMEOUT", "300"))
    with tempfile.TemporaryFile() as logf:
        try:
            proc = subprocess.Popen(argv, cwd=str(cwd) if cwd else None,
                                    env=env,
                                    stdin=subprocess.DEVNULL, stdout=logf,
                                    stderr=subprocess.STDOUT,
                                    start_new_session=True)
        except FileNotFoundError as e:
            raise ToolError(f"{argv[0]} not found on PATH - run inside "
                            "`nix develop`") from e
        try:
            proc.wait(timeout=timeout)
            rc = proc.returncode
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except (ProcessLookupError, PermissionError):
                pass
            proc.wait()
            rc = 0 if success is not None and success.exists() else 1
        logf.seek(0)
        return logf.read().decode("utf-8", "replace"), rc


# --------------------------------------------------------------------------- #
# prefix initialisation
# --------------------------------------------------------------------------- #

_ENV_KEY = (r"HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control"
            r"\Session Manager\Environment")


def _reg(*args: str, capture: bool = False) -> subprocess.CompletedProcess:
    quiet = {} if capture else {"stdout": subprocess.DEVNULL,
                                "stderr": subprocess.DEVNULL}
    return subprocess.run([require("wine"), "reg", *args], check=False,
                          text=True, capture_output=capture, env=headless_env(),
                          **quiet)


def init_prefix(force: bool = False) -> None:
    """Boot the prefix and set PATH/INCLUDE/LIB in the wine registry so era
    tools find binaries/headers/libs. DX6 comes FIRST in INCLUDE/LIB: VC5
    ships DirectX 3-era DDRAW.H/DPLAY.H which would shadow the DX6 SDK's
    (IID_IDirectPlay4A would not resolve)."""
    prefix = Path(os.environ.get("WINEPREFIX") or Path.home() / ".wine")
    if force or not (prefix / "drive_c").is_dir():
        prefix.mkdir(parents=True, exist_ok=True)
        try:
            subprocess.run([require("wineboot"), "--init"], check=True,
                           env=wine_env())
        except subprocess.CalledProcessError as e:
            raise ToolError(f"wineboot --init failed (rc={e.returncode}) for "
                            f"prefix {prefix}") from e
        subprocess.run([require("wineserver"), "--wait"], check=False,
                       env=wine_env())

    msvc = toolchain_root()
    vc_bin = winepath(msvc / "bin")
    include = winepath(msvc / "include")
    lib = winepath(msvc / "lib")

    cur = _reg("query", _ENV_KEY, "/v", "PATH", capture=True)
    cur_path = next((line.split()[-1] for line in cur.stdout.splitlines()
                     if "REG_" in line), "")
    if not cur_path:
        _reg("add", _ENV_KEY, "/v", "PATH", "/t", "REG_EXPAND_SZ",
             "/d", f"{vc_bin};%SystemRoot%\\system32;%SystemRoot%", "/f")
    elif vc_bin not in cur_path:
        _reg("add", _ENV_KEY, "/v", "PATH", "/t", "REG_EXPAND_SZ",
             "/d", f"{vc_bin};{cur_path}", "/f")
    _reg("add", _ENV_KEY, "/v", "INCLUDE", "/t", "REG_SZ", "/d", include, "/f")
    _reg("add", _ENV_KEY, "/v", "LIB", "/t", "REG_SZ", "/d", lib, "/f")
    disable_crash_dialog()


def verify_prefix() -> None:
    """Fail unless the registry INCLUDE exists and lists dx before msvc."""
    got = _reg("query", _ENV_KEY, "/v", "INCLUDE", capture=True)
    val = "".join(line for line in got.stdout.splitlines() if "REG_" in line).lower()
    if "include" not in val:
        raise ToolError("wine registry INCLUDE unset - run init_prefix() "
                        "(a cold wineserver can fail the first winepath)")


#: The era linker's name when it runs against a native period MSVCRT.DLL.
#: Wine's DLL overrides are keyed by executable name, so only this copy loads
#: the native runtime; every other tool keeps the builtin one.
NATIVE_CRT_LINKER = "LINKNCRT.EXE"


def _server_zones(prefix: Path) -> list[str | None]:
    """The TZ of every running wineserver of `prefix` (None: unset). A
    `wineserver -w`/`-k` invocation shares the name but serves nothing."""
    zones = []
    for proc in Path("/proc").glob("[0-9]*"):
        try:
            if proc.joinpath("comm").read_text().strip() != "wineserver":
                continue
            argv = proc.joinpath("cmdline").read_bytes().decode("utf-8", "replace").split("\0")
            if any(arg in ("-w", "--wait") or arg.startswith(("-k", "--kill"))
                   for arg in argv[1:]):
                continue
            env = dict(item.split("=", 1) for item in
                       proc.joinpath("environ").read_bytes().decode("utf-8", "replace")
                       .split("\0") if "=" in item)
        except OSError:
            continue
        theirs = Path(env.get("WINEPREFIX") or Path.home() / ".wine")
        if theirs.resolve() == prefix.resolve():
            zones.append(env.get("TZ"))
    return zones


def native_crt_linker(runtime: Path) -> Path:
    """A copy of the era LINK.EXE that imports `runtime` (a native MSVCRT.DLL)
    in place of wine's builtin msvcrt; returns its path.

    MSVCRT is a KnownDLL, so wine loads it from the prefix's 32-bit system
    directory, never from the linker's own directory: the native file goes
    there and an AppDefaults override selects it for NATIVE_CRT_LINKER alone.
    The builtin load order of every other process is unchanged, and a prefix
    update that restores wine's placeholder is undone on the next call.
    The wineserver maps the KnownDLLs when it starts, and its time zone is
    the local time the native runtime's time() reads: when the file changed
    or a running server is not in WINE_ZONE (one started outside this
    tooling), that server is let go once its clients finish and one in
    WINE_ZONE starts. A starting wineserver cannot take over a running one
    (it fails to lock the prefix and exits) and `wineserver -k` would kill
    parallel tools, so the only lever is to wait for the running one to exit.
    The candidate link checks the stamps it produced (graph.link).
    """
    import filecmp
    from homm1.core.paths import BUILD
    root = toolchain_root()
    ensure_link_deps()
    prefix = Path(os.environ.get("WINEPREFIX") or Path.home() / ".wine")
    windows = prefix / "drive_c" / "windows"
    system = windows / "syswow64" if (windows / "syswow64").is_dir() else windows / "system32"
    if not system.is_dir():
        raise ToolError(f"wine prefix {prefix} has no system directory - run `homm1 init`")
    target = system / "msvcrt.dll"
    changed = not (target.is_file() and filecmp.cmp(target, runtime, shallow=False))
    if changed:
        shutil.copyfile(runtime, target)
    ws = require("wineserver")
    wait = float(os.environ.get("HOMM1_WINE_TIMEOUT", "300"))

    def retire(why: str) -> None:
        try:
            subprocess.run([ws, "-w"], check=False, env=wine_env(), timeout=wait,
                           stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL)
        except subprocess.TimeoutExpired:
            raise ToolError(f"the wineserver of {prefix} ({why}) still runs after "
                            f"{wait:.0f}s; stop its clients or run `homm1 tool wine "
                            "--shutdown`") from None

    if changed:
        retire("started before the native runtime was installed")
    for _ in range(3):
        ensure_wineserver()
        stray = [zone for zone in _server_zones(prefix) if zone != WINE_ZONE]
        if not stray:
            break
        retire(f"TZ={stray[0]}, not {WINE_ZONE}")
    else:
        raise ToolError(f"a wineserver of {prefix} outside TZ={WINE_ZONE} keeps "
                        "starting: something outside the tooling runs wine on "
                        "this prefix")
    folder = BUILD / "link" / "native-crt"
    folder.mkdir(parents=True, exist_ok=True)
    for name in ("link.exe", "mspdb60.dll", "msobj10.dll", "msdis110.dll"):
        source = find_ci(root / "bin", name)
        if source is None:
            continue
        copy = folder / (NATIVE_CRT_LINKER if name == "link.exe" else source.name)
        if not (copy.is_file() and filecmp.cmp(copy, source, shallow=False)):
            shutil.copyfile(source, copy)
    key = rf"HKEY_CURRENT_USER\Software\Wine\AppDefaults\{NATIVE_CRT_LINKER}\DllOverrides"
    got = _reg("query", key, "/v", "msvcrt", capture=True)
    if "native" not in got.stdout:
        _reg("add", key, "/v", "msvcrt", "/d", "native", "/f")
    return folder / NATIVE_CRT_LINKER


from homm1.core.usage import logged


@logged
def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(prog="homm1 tool wine", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--init", action="store_true", help="initialise the prefix")
    ap.add_argument("--force", action="store_true",
                    help="with --init: re-run wineboot even on a live prefix")
    ap.add_argument("--verify", action="store_true",
                    help="check the registry INCLUDE (dx before msvc)")
    ap.add_argument("--shutdown", action="store_true", help="kill the wineserver")
    a = ap.parse_args()
    if not (a.init or a.verify or a.shutdown):
        # Silently exiting 0 having done nothing read as "the prefix is fine".
        ap.print_help(sys.stderr)
        print("\n[wine] pick an action: --init, --verify or --shutdown",
              file=sys.stderr)
        return 2
    if a.force and not a.init:
        print("[wine] --force only applies to --init", file=sys.stderr)
        return 2
    try:
        if a.init:
            init_prefix(force=a.force)
        if a.verify:
            verify_prefix()
            print("prefix OK")
        if a.shutdown:
            shutdown_wineserver()
    except (ToolError, OSError) as e:
        print(f"[wine] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
