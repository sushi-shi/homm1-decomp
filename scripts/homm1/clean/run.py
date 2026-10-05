"""homm1 clean - generate the clean source trees, verify them, publish them.

    homm1 clean --out build/clean
    homm1 clean --out build/clean --verify --publish             # source-buka-2003
    homm1 clean --variant classic --out build/classic --verify --publish

The matching tree carries scaffolding that exists only to prove the source
reproduces retail object code: address annotations (`VA`, `DATA`,
`VA_COMPGEN`, `RVA_DYNINIT`), the dual-build enum machinery (`H1_ENUM_*`),
the `#line` pins of retail assertion lines, and reconstruction comments. The
generator resolves each macro to the production expansion the pinned VC6
compiler already sees and removes the rest.

Two variants come from one snapshot. `source` keeps the catalog references
(`localization::Tr("id")`) and builds Russian or English with VC6; `classic`
is the same tree with the Russian text spelled out as readable UTF-8 for
reading. See docs/clean-source.md.
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile

from homm1.clean import classic, source
from homm1.clean.project import EXECUTABLE
from homm1.core.paths import REPO
from homm1.core.usage import logged

MARKER = ".homm1-clean-generated"
PROVENANCE = "Generated-By: homm1 clean"
#: The retail target the branches are named after.
TARGET = "buka-2003"
VARIANTS = ("source", "classic")


def default_branch(variant: str) -> str:
    return f"{variant}-{TARGET}"


def git(repo: Path, *arguments: str, **kwargs) -> str:
    return subprocess.check_output(["git", "-C", str(repo), *arguments], text=True,
                                   **kwargs).strip()


def snapshot(repo: Path, revision: str = "HEAD", *, working: bool = False
             ) -> tuple[str, dict[str, bytes]]:
    """The tracked files of `revision`, or of the working tree for a preview."""
    commit = git(repo, "rev-parse", "--verify", f"{revision}^{{commit}}")
    files: dict[str, bytes] = {}
    if working:
        for name in git(repo, "ls-files", "-z").split("\0"):
            path = repo / name
            if name and path.is_file() and not path.is_symlink():
                files[name] = path.read_bytes()
        return commit, files
    archive = subprocess.check_output(["git", "-C", str(repo), "archive", commit])
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        for member in stream:
            if member.isfile():
                files[member.name] = stream.extractfile(member).read()
    return commit, files


def _units(files: dict[str, bytes]) -> list[dict]:
    import tomllib
    return list(tomllib.loads(files["config/units.toml"].decode())["unit"])


def selected(files: dict[str, bytes]) -> dict[str, str]:
    """{path: transform} for every file the clean tree carries.

    The tree holds the unit sources, every header, and the resource script.
    Anything else under src/ fails generation rather than silently vanishing.
    """
    chosen = {unit["source"]: "" for unit in _units(files)}
    for name in files:
        top = name.split("/", 1)[0]
        if top in ("include", "vendor") and name.endswith(".h") \
                and name not in source.DROP_FILES:
            chosen[name] = ""
        elif top == "src" and name.endswith(".rc"):
            chosen[name] = ""
        elif top == "src" and name not in chosen and not name.endswith(".h"):
            raise ValueError(f"{name}: not a unit source; extend homm1.clean.run.selected")
    for name in chosen:
        if name not in files:
            raise ValueError(f"{name}: configured unit source is missing")
        suffix = Path(name).suffix.lower()
        chosen[name] = {".cpp": "cpp", ".c": "cpp", ".h": "cpp", ".asm": "asm",
                        ".rc": "rc"}.get(suffix) or ""
        if not chosen[name]:
            raise ValueError(f"{name}: no clean transform for {suffix} files")
    return chosen


LOCALE_FILES = ("locales/messages.def", "locales/ru.po", "locales/format-variants.json")


def catalog_of(files: dict[str, bytes]):
    """The snapshot's localization catalog (None without one)."""
    from homm1.graph.catalog import Catalog
    if "locales/messages.def" not in files:
        return None
    variants = files.get("locales/format-variants.json")
    return Catalog.parse(files["locales/messages.def"].decode("utf-8"),
                         files["locales/ru.po"].decode("utf-8"),
                         variants.decode("utf-8") if variants is not None else None)


def generate(files: dict[str, bytes], *, variant: str = "source", control: bool = False
             ) -> tuple[dict[str, bytes], list[str]]:
    """The clean tree as {path: bytes}, plus any self-check failures.

    `variant` selects the source tree or its classic (readable Russian) view.
    `control` gives verification's line-preserving source tree: the same macro
    expansions and comment removal, with every line, `#line` pin and
    scaffolding header and include kept, so the assertion lines and file
    names equal the matching build's."""
    if variant not in VARIANTS:
        raise ValueError(f"unknown variant {variant!r}; choose from {', '.join(VARIANTS)}")
    if control and variant != "source":
        raise ValueError("the control tree is a source tree")
    transforms = {"cpp": source.clean_cpp, "asm": source.clean_asm, "rc": source.clean_rc}
    kinds = {"cpp": {}, "asm": {"asm": True}, "rc": {"rc": True}}
    catalog = catalog_of(files)
    if variant == "classic" and catalog is None:
        raise ValueError("the classic view needs the snapshot's locales/ catalog")
    output: dict[str, bytes] = {}
    problems: list[str] = []
    chosen = sorted(selected(files).items())
    renames = source.aliases(files[name].decode("utf-8") for name, kind in chosen
                             if kind == "cpp")
    for name, kind in chosen:
        text = files[name].decode("utf-8")
        try:
            if kind == "asm":
                cleaned = source.clean_asm(text, keep_lines=control, renames=renames)
            else:
                cleaned = transforms[kind](text, keep_lines=control)
            if variant == "classic" and kind == "cpp":
                cleaned = classic.render_cpp(cleaned, catalog)
            elif variant == "classic" and kind == "rc":
                cleaned = classic.render_rc(cleaned, catalog)
        except ValueError as error:
            raise ValueError(f"{name}: {error}") from error
        if control:
            output[name] = cleaned.encode("utf-8")
            continue
        if kind == "cpp":
            problems += [f"{name}: scaffolding survived: {word}"
                         for word in sorted(set(source.residue(cleaned)))]
        elif any(token == "comment" for token, _ in source.tokens(cleaned, **kinds[kind])):
            problems.append(f"{name}: comment survived")
        problems += [f"{name}: stranded punctuation: {line}"
                     for line in source.stranded(text, cleaned, **kinds[kind])]
        if variant == "classic" and "localization::" in cleaned:
            problems.append(f"{name}: a catalog reference survived the classic rendering")
        output[name] = cleaned.encode("utf-8")
    if variant == "source" and catalog is not None:
        output.update({name: files[name] for name in LOCALE_FILES if name in files})
        output["catalog.py"] = files["scripts/homm1/graph/catalog.py"]
    if control:
        output["build.json"] = json.dumps({"locale": retail_locale(files)}).encode()
        # The scaffolding headers keep their declarations and definitions
        # (match.h now also carries the integer aliases), comments blanked.
        output.update({name: source.blank(source.strip_comments(files[name].decode())).encode()
                       for name in source.DROP_FILES})
        return output, problems
    from homm1.clean.project import project_files
    output.update(project_files(files, variant))
    return output, problems


def retail_locale(files: dict[str, bytes]) -> str:
    return json.loads(files["config/retail/targets.json"])["game"].get("locale", "ru")


def validate_output(repo: Path, requested: Path) -> Path:
    """Fail closed before any recursive deletion."""
    path = requested if requested.is_absolute() else repo / requested
    if path.is_symlink() or any(parent.is_symlink() for parent in path.parents):
        raise ValueError("output must not traverse symlinks")
    path, repo = path.resolve(), repo.resolve()
    if path == repo or path in repo.parents:
        raise ValueError("output must not contain the repository")
    if path.is_relative_to(repo) and not path.is_relative_to(repo / "build"):
        raise ValueError("output inside the repository must be under build/")
    if path == repo / "build":
        raise ValueError("output must be a child of build/")
    if path.exists() and (not (path / MARKER).is_file() or (path / ".git").exists()):
        raise ValueError("existing output must be a marked generated directory, not a worktree")
    return path


def write_output(repo: Path, requested: Path, files: dict[str, bytes], commit: str,
                 working: bool) -> Path:
    output = validate_output(repo, requested)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".homm1-clean-", dir=output.parent) as directory:
        staging = Path(directory) / "tree"
        staging.mkdir()
        for name, data in sorted(files.items()):
            path = staging / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
            if name in EXECUTABLE:
                path.chmod(0o755)
        (staging / MARKER).write_text(json.dumps({"commit": commit, "working": working}) + "\n")
        if output.exists():
            shutil.rmtree(output)
        staging.rename(output)
    return output


def fingerprint(files: dict[str, bytes]) -> str:
    return hashlib.sha256(b"".join(name.encode() + b"\0" + data
                                   for name, data in sorted(files.items()))).hexdigest()


@logged
def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="homm1 clean", description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--variant", choices=VARIANTS, default="source",
                        help="source: the buildable tree with its locale catalog (default); "
                             "classic: the same tree with readable Russian text")
    parser.add_argument("--out", type=Path, help="output directory (default: build/<variant>)")
    parser.add_argument("--ref", default="HEAD", help="committed revision to export")
    parser.add_argument("--working-tree", action="store_true",
                        help="preview tracked working files (cannot publish)")
    parser.add_argument("--verify", action="store_true",
                        help="build the trees with the pinned VC6 toolchain and compare "
                             "them with the matching build")
    parser.add_argument("--publish", metavar="BRANCH", nargs="?", const="",
                        help="commit the tree as a single-commit snapshot branch "
                             "(default: <variant>-" + TARGET + "); local only, never pushed")
    args = parser.parse_args(argv)
    args.out = args.out or Path("build") / args.variant
    if args.publish == "":
        args.publish = default_branch(args.variant)
    try:
        if args.publish and args.working_tree:
            raise ValueError("publication requires a committed revision, not --working-tree")
        if args.verify and args.ref != "HEAD":
            raise ValueError("--verify compares with the current build; check out the "
                             "requested revision first")
        commit, inputs = snapshot(REPO, args.ref, working=args.working_tree)
        files, problems = generate(inputs, variant=args.variant)
        if problems:
            for problem in problems[:20]:
                print(f"[clean] {problem}", file=sys.stderr)
            if len(problems) > 20:
                print(f"[clean] ... and {len(problems) - 20} more", file=sys.stderr)
            return 1
        output = write_output(REPO, args.out, files, commit, args.working_tree)
        print(f"[clean] wrote {len(files)} files to {output} from {commit[:12]}"
              f"{' (working tree)' if args.working_tree else ''}; "
              f"SHA-256 {fingerprint(files)}")
        print(f"[clean] {args.variant}: no comments, scaffolding macros, #line pins or "
              "stranded punctuation remain")
        if args.verify:
            from homm1.clean.verify import verify
            if not args.working_tree and git(REPO, "status", "--porcelain", "--", "src",
                                             "include", "vendor", "config"):
                print("[clean] note: the matching build reads the working tree, which "
                      "differs from HEAD; commit first for a like-for-like comparison",
                      file=sys.stderr)
            status = verify(output, inputs, args.variant)
            if status:
                return status
        if args.publish:
            from homm1.clean.publish import publish
            tip = publish(REPO, files, commit, args.publish)
            print(f"[clean] {args.publish}: {tip[:12]} (local branch; nothing pushed)")
        return 0
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(f"[clean] {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
