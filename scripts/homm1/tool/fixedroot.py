"""homm1.tool.fixedroot - run the era compiler under a fixed, retail-shaped path view.

    python3 -m homm1.tool.fixedroot --src S --out O [--retail-name NAME] -- <cl flags>

VC4's incremental compilation (/Gi, which retail used; see
docs/patterns/vc4-gi-incremental-compilation.md and docs/patterns/vc4-gi-line-var.md)
makes the generated code depend on the path strings of the files the compiler
opens (the source and every header): the same ARMY.cpp compiled from two
directories whose names differ in length gives different operand orders.
Without a fixed view the score of a function depends on where the worktree or
sandbox lives.

So every compile runs in a private mount namespace (`unshare -r -m`, no root
needed) that shows the compiler the same tree, whatever the checkout path:

    F:\\h1w95src\\source\\   src/SOURCE/* and include/SOURCE/* (symlinks)
    F:\\H1w95src\\Base\\     src/BASE/* and include/BASE/*
    D:\\MSDEV\\              the pinned VC4.1 tree

The source roots come from `build.source_roots` in config/units.toml;
1.0/1.1 contracts without that table retain D:\\Heroes. The working directory
and object output live beside the source on a private tmpfs. Header roots
follow the selected source root; compiler and SDK inputs remain pinned.

The namespace also gets its own wineserver (a tmpfs over /tmp/.wine-0), its own
dosdevices (d: -> the fixed root) and private copies of the prefix's registry
files, so concurrent compiles from several worktrees never see each other's
view and never write the shared prefix's registry.
"""
from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import tomllib
from pathlib import Path

ROOT = Path("/tmp/homm1-fixedroot")          # empty mount point; a tmpfs inside the namespace
SERVER_DIR = Path("/tmp/.wine-0")           # wineserver socket dir of uid 0 (the namespace uid)
DIR_NAMES = {"SOURCE": "Source", "BASE": "Base"}
#: The object directory, relative to D:\\ (empty: beside the source). Under /Gi
#: C1's symbol handles grow with the length of the -Fo path, so this is a
#: fitted build fact (docs/patterns/vc4-gi-handles-follow-path-lengths.md).
OBJ_DIR = ""


def split_name(name: str) -> tuple[str, str]:
    """'Source\\TOWNMGR.CPP' -> ('Source', 'TOWNMGR.CPP')."""
    d, _, f = name.replace("/", "\\").rpartition("\\")
    return DIR_NAMES.get(d.upper(), d), f


def default_name(src: Path, unit: str | None = None) -> str:
    """Retail-shaped Dir\\NAME.CPP: the unit's directory and stem (SOURCE/ARMY ->
    Source\\ARMY.CPP), else the source file's."""
    if unit and "/" in unit:
        d, stem = unit.split("/", 1)
    else:
        d, stem = Path(src).parent.name, disposable_stem(Path(src).stem)
    return f"{DIR_NAMES.get(d.upper(), d)}\\{stem.upper()}.CPP"


def disposable_stem(stem: str) -> str:
    """The unit stem of a permuter's sibling probe (.ARMY.trial0003 -> ARMY)."""
    if stem.startswith(".") and "." in stem[1:]:
        return stem[1:].split(".", 1)[0]
    return stem


# --------------------------------------------------------------------------- outer
def compile(src: Path | str, out: Path | str, flags: list[str], *, retail_name: str | None = None,
            unit: str | None = None, repo: Path | None = None, msvc: Path | None = None, timeout: float | None = None, locale: str | None = None) -> str:
    """Compile SRC to OUT through the fixed view; return the compiler output."""
    from homm1.core.paths import REPO, VENDOR, msvc_dir, vendor_include_dirs
    from homm1.tool import ToolError
    src, out = Path(src).resolve(), Path(out).resolve()
    out.parent.mkdir(parents=True, exist_ok=True)
    out.unlink(missing_ok=True)
    ROOT.mkdir(exist_ok=True)
    SERVER_DIR.mkdir(mode=0o700, exist_ok=True)
    source_repo = (repo or REPO).resolve()
    # A generated clean/control tree deliberately has no reconstruction config.
    # Its verification still uses this checkout's compiler/path contract.
    contract_path = source_repo / "config/units.toml"
    if not contract_path.is_file():
        contract_path = REPO / "config/units.toml"
    contract = tomllib.loads(contract_path.read_text())
    source_dir = (unit.split("/", 1)[0] if unit else src.parent.name).upper()
    from homm1.graph.localization import prepare
    original_name = retail_name or default_name(src, unit)
    src, header, _overlay, _deps = prepare(source_repo, src, locale=locale)

    source_root = contract.get("build", {}).get("source_roots", {}).get(source_dir)
    job = {"source_root": source_root, "src": str(src), "out": str(out), "flags": flags,
           "localization_header": str(header) if header else None,
           "name": original_name,
           "repo": str((repo or REPO).resolve()), "msvc": str((msvc or msvc_dir()).resolve()),
           "prefix": str(Path(os.environ.get("WINEPREFIX") or Path.home() / ".wine").resolve()),
           # research knob: put include/ under D:\\Heroes\\<dir> instead of beside the sources
           "include": os.environ.get("HOMM1_FIXEDROOT_INCLUDE", ""),
           # the object directory under D:\\ (empty: the source directory)
           "objdir": os.environ.get("HOMM1_FIXEDROOT_OBJDIR", OBJ_DIR),
           # vendor/<sdk> trees and the pinned SDKs' installed include dirs
           "vendor": [[name, str(d.resolve())] for name, d in
                      vendor_include_dirs(Path(repo) / "vendor" if repo else VENDOR)]}
    env = dict(os.environ, PYTHONPATH=str(Path(__file__).resolve().parents[2]) + os.pathsep
               + os.environ.get("PYTHONPATH", ""))
    if timeout is None:
        timeout = float(os.environ.get("HOMM1_WINE_TIMEOUT", "300"))
    with tempfile.TemporaryFile() as logf:
        p = subprocess.Popen(["unshare", "-r", "-m", sys.executable, "-m", "homm1.tool.fixedroot",
                              "--inner", json.dumps(job)], env=env, stdin=subprocess.DEVNULL,
                             stdout=logf, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            p.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(p.pid, 9)
            p.wait()
        logf.seek(0)
        output = logf.read().decode("utf-8", "replace")
    if not out.exists():
        tail = "\n".join(output.strip().splitlines()[-12:]) or "(cl said nothing)"
        raise ToolError(f"cl produced no object for {src.name} (rc={p.returncode}):\n{tail}")
    return output


# --------------------------------------------------------------------------- inner
def _mount(*args: str) -> None:
    # A wineserver outside may be replacing a registry file (write + rename)
    # at this moment; the bind target then briefly does not exist.
    for attempt in range(20):
        if subprocess.run(["mount", *args], stderr=subprocess.DEVNULL).returncode == 0:
            return
        time.sleep(0.05 * (attempt + 1))
    subprocess.run(["mount", *args], check=True)


def _link_tree(dst: Path, srcdir: Path) -> None:
    dst.mkdir(parents=True, exist_ok=True)
    for f in srcdir.iterdir():
        t = dst / f.name
        if not t.exists():
            t.symlink_to(f.resolve())


def _inner(job: dict) -> int:
    repo, msvc, prefix = Path(job["repo"]), Path(job["msvc"]), Path(job["prefix"])
    src, out = Path(job["src"]), Path(job["out"])
    _mount("-t", "tmpfs", "homm1-fixedroot", str(ROOT))
    _mount("-t", "tmpfs", "homm1-wineserver", str(SERVER_DIR))
    os.chmod(SERVER_DIR, 0o700)
    source_root = job.get("source_root")
    project_root = source_root.rsplit("\\", 1)[0] if source_root else "D:\\Heroes"
    drive, project = project_root.split(":\\", 1)
    heroes = ROOT.joinpath(*project.split("\\"))
    inc = job.get("include", "")
    for top, base in ((repo / "src", heroes), (repo / "include", heroes / inc if inc else heroes)):
        for d in sorted(top.iterdir()):
            if d.is_dir():
                _link_tree(base / DIR_NAMES.get(d.name.upper(), d.name), d)
            elif not (base / d.name).exists():
                base.mkdir(parents=True, exist_ok=True)
                (base / d.name).symlink_to(d.resolve())
    header = job.get("localization_header")
    if header:
        header = Path(header)
        # /Gi includes every opened path in its compiler state. The generated
        # catalog must have the same name in matching and clean-control builds.
        (heroes / "__homm1_messages.h").symlink_to(header)
        job["flags"] = [*job["flags"], "/FI" + project_root + "\\__homm1_messages.h"]
        if header.name == "messages.h":
            # prepare() mirrored every reachable header, including unchanged
            # intermediates needed by quoted sibling includes.
            for generated in (header.parent / "include").rglob("*"):
                if not generated.is_file():
                    continue
                parts = generated.relative_to(header.parent / "include").parts
                mapped = (DIR_NAMES.get(parts[0].upper(), parts[0]), *parts[1:]) if len(parts) > 1 else parts
                target = (heroes / inc if inc else heroes).joinpath(*mapped)
                target.parent.mkdir(parents=True, exist_ok=True)
                target.unlink(missing_ok=True)
                target.symlink_to(generated)
    vendor = [name for name, _d in job["vendor"]]
    (heroes / "Vendor").mkdir(parents=True, exist_ok=True)
    for name, d in job["vendor"]:
        (heroes / "Vendor" / name).symlink_to(d)
    (ROOT / "MSDEV").symlink_to(msvc)
    rdir, rname = split_name(job["name"])
    workdir = heroes / rdir
    workdir.mkdir(parents=True, exist_ok=True)
    named = workdir / rname
    if named.exists() or named.is_symlink():
        named.unlink()
    named.symlink_to(src)
    # A private WINEPREFIX on the namespace's tmpfs: copies of the registry files
    # (never written back), the update stamp (so wine does not re-run its
    # prefix update), and dosdevices c: -> the shared drive_c, d: -> the fixed
    # root, z: -> /. Nothing is mounted over, or written into, the shared prefix.
    # Belt and braces: the shared prefix is read-only inside the namespace.
    _mount("--bind", str(prefix), str(prefix))
    _mount("-o", "remount,ro,bind", str(prefix))
    private = ROOT / ".prefix"
    (private / "dosdevices").mkdir(parents=True)
    (private / "dosdevices" / "c:").symlink_to(prefix / "drive_c")
    for letter in {"d", drive.lower()}:
        (private / "dosdevices" / (letter + ":")).symlink_to(ROOT)
    (private / "dosdevices" / "z:").symlink_to("/")
    (private / "drive_c").symlink_to(prefix / "drive_c")
    for f in [*prefix.glob("*.reg"), prefix / ".update-timestamp"]:
        if not f.is_file():
            continue
        data = f.read_bytes()
        if f.name == "user.reg":
            # %TEMP% (the compiler's -il intermediates) on the private root, at a
            # path that does not depend on the host user name.
            data = re.sub(rb'^"(TEMP|TMP)"=".*"$', rb'"\1"="D:\\\\TMP"', data, flags=re.M)
        (private / f.name).write_bytes(data)
    (ROOT / "TMP").mkdir()
    d = source_root or f"D:\\Heroes\\{rdir}"
    objdir = job.get("objdir") or ""
    if objdir:
        odir = ROOT.joinpath(*objdir.split("\\"))
        odir.mkdir(parents=True, exist_ok=True)
        fo = "D:\\" + objdir
    else:
        odir, fo = workdir, d
    obj = odir / (Path(rname).stem + ".obj")
    incs = ["/X", "/I" + project_root + (f"\\{inc}" if inc else ""), *[f"/I{project_root}\\Vendor\\{v}" for v in vendor], "/ID:\\MSDEV\\INCLUDE"]
    argv = ["wine", "D:\\MSDEV\\BIN\\CL.EXE", *incs, *job["flags"], f"/Fo{fo}\\{obj.name}",
            f"{d}\\{rname}"]
    from homm1.tool.wine import wine_env
    env = wine_env(dict(os.environ, WINEPREFIX=str(private),
                        WINEDEBUG=os.environ.get("WINEDEBUG", "fixme-all,err-kerberos")))
    r = subprocess.run(argv, cwd=workdir, env=env, stdin=subprocess.DEVNULL)
    if obj.exists():
        shutil.copyfile(obj, out)
        for lst in workdir.glob(Path(rname).stem + ".[aA][sS][mM]"):
            shutil.copyfile(lst, out.with_suffix(".asm"))
    subprocess.run(["wineserver", "-k"], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return 0 if obj.exists() else (r.returncode or 1)


if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[1] == "--inner":
        raise SystemExit(_inner(json.loads(sys.argv[2])))
    from homm1.core.usage import logged

    @logged
    def main() -> int:
        import argparse
        ap = argparse.ArgumentParser(description=__doc__)
        ap.add_argument("--src", required=True)
        ap.add_argument("--out", required=True)
        ap.add_argument("--retail-name")
        ap.add_argument("flags", nargs=argparse.REMAINDER)
        a = ap.parse_args()
        flags = a.flags[1:] if a.flags[:1] == ["--"] else a.flags
        print(compile(a.src, a.out, flags, retail_name=a.retail_name))
        return 0

    raise SystemExit(main())
