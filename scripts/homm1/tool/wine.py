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
        # a winepath that boots the session starts the server in UTC, as
        # ensure_wineserver does
        return subprocess.check_output([exe, "-w", str(p)],
                                       text=True, env={**os.environ, "TZ": "UTC"},
                                       stderr=subprocess.DEVNULL).strip()
    except subprocess.CalledProcessError as e:
        raise ToolError(f"winepath -w {p} failed (rc={e.returncode}) - the "
                        "wine prefix may not be initialised; run "
                        "`homm1 init`") from e


def ensure_wineserver() -> None:
    """`wineserver -p60`: keep the server 60s past the last client, so parallel
    `wine cl` invocations under ninja skip the cold start, yet it exits on
    its own afterwards (bare `-p` persisted forever and leaked). Idempotent.
    A server this starts keeps UTC: its zone is every wine process's local
    time, which a native C runtime's time() reads (native_crt_linker)."""
    ws = shutil.which("wineserver")
    if ws:
        subprocess.run([ws, "-p60"], check=False, env={**os.environ, "TZ": "UTC"},
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
                          text=True, capture_output=capture, **quiet)


def init_prefix(force: bool = False) -> None:
    """Boot the prefix and set PATH/INCLUDE/LIB in the wine registry so era
    tools find binaries/headers/libs. DX6 comes FIRST in INCLUDE/LIB: VC5
    ships DirectX 3-era DDRAW.H/DPLAY.H which would shadow the DX6 SDK's
    (IID_IDirectPlay4A would not resolve)."""
    prefix = Path(os.environ.get("WINEPREFIX") or Path.home() / ".wine")
    if force or not (prefix / "drive_c").is_dir():
        prefix.mkdir(parents=True, exist_ok=True)
        try:
            subprocess.run([require("wineboot"), "--init"], check=True)
        except subprocess.CalledProcessError as e:
            raise ToolError(f"wineboot --init failed (rc={e.returncode}) for "
                            f"prefix {prefix}") from e
        subprocess.run([require("wineserver"), "--wait"], check=False)

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


def native_crt_linker(runtime: Path) -> Path:
    """A copy of the era LINK.EXE that imports `runtime` (a native MSVCRT.DLL)
    in place of wine's builtin msvcrt; returns its path.

    MSVCRT is a KnownDLL, so wine loads it from the prefix's 32-bit system
    directory, never from the linker's own directory: the native file goes
    there and an AppDefaults override selects it for NATIVE_CRT_LINKER alone
    (the wineserver, which maps the KnownDLLs at start and keeps the time
    zone the native runtime's time() reads, is restarted in UTC).
    The builtin load order of every other process is unchanged, and a prefix
    update that restores wine's placeholder is undone on the next call.
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
    if not (target.is_file() and filecmp.cmp(target, runtime, shallow=False)):
        shutil.copyfile(runtime, target)
    # The server maps the KnownDLLs when it starts, and its time zone is the
    # local time a native MSVCRT's time() reads: restart it in UTC, the zone
    # faked_clock gives the linker.
    shutdown_wineserver()
    subprocess.run([require("wineserver"), "-w"], check=False)
    ensure_wineserver()
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
