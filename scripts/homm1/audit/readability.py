"""Source-reading inventory, separate from matching scores and retail boundaries.

Run with Universal Ctags 6 on PATH:
  python3 -m homm1.audit.readability --write
  python3 -m homm1.audit.readability --check

Generated reports live under build/readability/inventory/ and are not committed.
Human review records remain input at config/cleanliness/file_reviews.json; generation
never edits them or grants new reading credit.

Ctags indexes physical definitions, including header bodies and inactive branches.
An independent preprocessor pass covers macros in branches the C++ parser skips.
Project enum macros are expanded only for indexing; game files are never rewritten.
Every RVA marker must resolve to exactly one definition. Unmarked/header bodies are
also indexed, but the file-by-file human pass remains the completeness backstop.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess

from homm1.core.inputs import REPO
REPORT = Path("build/readability/inventory")
REVIEWS = Path("config/cleanliness/file_reviews.json")
RVA = re.compile(r'^\s*(?:extern\s+"C"\s+)?RVA\(\s*(0x[0-9a-fA-F]+)\s*,[^\n]*\)\s*$', re.M)
PROC = re.compile(r"^(\w+)\s+PROC\b", re.M | re.I)
IGNORES = (
    "RVA+,DATA+,VTBL+,VTBL2+,SIZE+,RVA_COMPGEN+,DATA_COMPGEN_GUARD+,"
    "NEW_RVA+,NEW_SIZE+,H1_RETAIL_INLINE,OVERRIDE,H1_CONST,H1_FINAL,"
    "requires+,__is_enum+,__is_integral+"
)
DEFINES = [
    "H1_UNUSED(n)=n",
    "H1_ENUM_BEGIN(n)=enum n {", "H1_ENUM_END(n)=};",
    "H1_ENUM_CLASS_BEGIN(n)=enum n {", "H1_ENUM_CLASS_END(n)=};",
    "H1_ENUM_CLASS_BEGIN_T(n,s)=enum n {", "H1_ENUM_CLASS_END_T(n,s)=};",
    "H1_ENUM_CLASS_BEGIN_SPLIT(n,s)=enum n {", "H1_ENUM_CLASS_END_SPLIT(n,s)=};",
    "H1_ENUM_CLASS_FORWARD(n)=enum n", "H1_ENUM_CLASS_FORWARD_SPLIT(n,s)=enum n",
    "H1_ENUM_FLAGS(n)=", "H1_ENUM_STEPPED(n)=", "H1_ENUM_INDEX_OFFSET(n)=",
]
DEFINES += [f"{name}(n,s)=n" for name in (
    "H1_ENUM_PARAM", "H1_ENUM_RETURN", "H1_ENUM_STORAGE", "H1_ENUM_STORAGE_STEPPED",
    "H1_OPEN_CODE_PARAM", "H1_OPEN_CODE_STORAGE", "H1_ENUM_BITFIELD",
)]


def digest(blob: bytes) -> str:
    return hashlib.sha256(blob).hexdigest()


def tracked_files(root: Path) -> list[str]:
    # Include newly added files and source exports without .git. Directory
    # placeholders carry no source; other unsupported files still fail below.
    return sorted(path.relative_to(root).as_posix()
                  for directory in ("src", "include")
                  for path in (root / directory).rglob("*")
                  if path.is_file() and path.name != ".gitkeep")


def ctags_rows(root: Path, paths: list[str], executable: str) -> list[dict]:
    command = [executable, "--options=NONE", "--output-format=json", "--sort=no",
               "--fields=+neKSt", "--fields-C++=+{properties}",
               "--kinds-C++=fp", "--language-force=C++", "--if0=yes", "-I", IGNORES]
    for definition in DEFINES:
        command.extend(("-D", definition))
    command += ["-o", "-"] + paths
    # The C++ parser can suppress an #else macro after an inline body even with
    # --if0=yes (NextCreatureType in KB_TYPES.h). Index physical macro definitions
    # independently, without the enum expansions used only to expose C++ bodies.
    macro_command = [executable, "--options=NONE", "--output-format=json", "--sort=no",
                     "--fields=+neKSt", "--kinds-CPreProcessor=d",
                     "--language-force=CPreProcessor", "--if0=yes", "-o", "-"] + paths
    rows = []
    for invocation in (command, macro_command):
        result = subprocess.run(invocation, cwd=root, check=True,
                                capture_output=True, text=True)
        rows.extend(row for line in result.stdout.splitlines()
                    if (row := json.loads(line)).get("_type") == "tag")
    return rows


def tag_rows(tags: list[dict], blobs: dict[str, bytes]) -> tuple[list[dict], list[dict]]:
    functions, macros = [], []
    seen = set()
    for tag in tags:
        path, start = tag["path"], tag["line"]
        if path not in blobs:
            raise ValueError(f"Ctags returned an out-of-scope path: {path}")
        lines = blobs[path].splitlines(keepends=True)
        end = tag.get("end", start)
        if not 1 <= start <= end <= len(lines):
            raise ValueError(f"Invalid definition extent: {path}:{start}-{end}")
        scope = re.sub(r"\b__anon[0-9a-f]+\b", "(anonymous)", tag.get("scope", ""))
        name = "::".join(filter(None, (scope, tag["name"])))
        kind = tag["kind"]
        key = (path, start, end, kind, name)
        if key in seen:
            continue
        seen.add(key)
        defaulted = (kind == "prototype" and
                     re.search(rb"=\s*(?:default|delete)\s*;", b"".join(lines[start - 1:end])))
        if kind == "function" or defaulted:
            if tag["name"] in {"i8", "i16", "i32", "u8", "u16", "u32", "void", "bool"}:
                raise ValueError(f"Type mistaken for function name: {key}")
            if tag["name"].startswith("H1_") and "H1_" in tag["name"] and tag["name"].isupper():
                raise ValueError(f"Unexpanded project macro mistaken for function: {key}")
            if kind == "function" and "end" not in tag:
                raise ValueError(f"Function has no closing extent: {key}")
            row = dict(path=path, line=start, end=end, name=name,
                       signature=tag.get("signature", ""),
                       kind="cpp-default-or-delete" if defaulted else "cpp",
                       properties=tag.get("properties", ""), rva="",
                       body_sha256=digest(b"".join(lines[start - 1:end])))
            functions.append(row)
        elif kind == "macro":
            # Include object-like macros as well: some hide executable expressions.
            macros.append(dict(path=path, line=start, end=end, name=name,
                               body_sha256=digest(b"".join(lines[start - 1:end]))))
    return functions, macros


def assembly_rows(path: str, blob: bytes) -> list[dict]:
    text = blob.decode("utf-8")
    rows = []
    for match in PROC.finditer(text):
        name = match[1]
        tail = re.search(rf"^{re.escape(name)}\s+ENDP\b", text[match.end():], re.M | re.I)
        if tail is None:
            raise ValueError(f"Unterminated assembly function: {path}:{name}")
        start = text.count("\n", 0, match.start()) + 1
        end = text.count("\n", 0, match.end() + tail.end()) + 1
        rows.append(dict(path=path, line=start, end=end, name=name, signature="",
                         kind="asm", properties="", rva="",
                         body_sha256=digest(b"".join(blob.splitlines(keepends=True)[start - 1:end]))))
    return rows


def attach_addresses(functions: list[dict], blobs: dict[str, bytes]) -> int:
    count = 0
    for path, blob in blobs.items():
        text = blob.decode("utf-8")
        markers = list(RVA.finditer(text))
        for index, marker in enumerate(markers):
            first = text.count("\n", 0, marker.end()) + 1
            last = (text.count("\n", 0, markers[index + 1].start()) + 1
                    if index + 1 < len(markers) else len(text.splitlines()) + 1)
            candidates = [row for row in functions if row["path"] == path
                          and first <= row["line"] < last]
            # Unannotated helpers after an annotated body are legal; the immediate
            # next definition owns the marker, whose intervening text is checked.
            candidates.sort(key=lambda row: row["line"])
            if not candidates:
                raise ValueError(f"RVA marker has no definition: {path}:{first}")
            row = candidates[0]
            gap = "\n".join(text.splitlines()[first:row["line"] - 1])
            if "{" in re.sub(r"//[^\n]*|/\*.*?\*/", "", gap, flags=re.S):
                raise ValueError(f"Ctags missed a body after RVA marker: {path}:{first}")
            if row["rva"]:
                raise ValueError(f"Multiple RVA markers mapped to one body: {path}:{row['line']}")
            row["rva"] = marker[1].lower()
            count += 1
    return count


def reviewed(path: str, sha256: str, reviews: dict) -> tuple[str, str]:
    entry = reviews.get(path)
    if not entry:
        return "unread", ""
    if not entry.get("note") or not entry.get("sha256"):
        raise ValueError(f"Review needs a content hash and substantive note: {path}")
    if entry["sha256"] != sha256:
        return "unread", "STALE: " + entry["note"]
    return "read", entry["note"]


def tsv(rows: list[dict], columns: list[str]) -> str:
    stream = io.StringIO()
    writer = csv.DictWriter(stream, columns, delimiter="\t", lineterminator="\n")
    writer.writeheader()
    writer.writerows({key: row.get(key, "") for key in columns} for row in rows)
    return stream.getvalue()


def generate(root: Path, executable: str) -> dict[str, str]:
    paths = tracked_files(root)
    unsupported = [path for path in paths if Path(path).suffix not in (".cpp", ".cc", ".cxx", ".h", ".hh", ".hpp", ".inl", ".asm")]
    if unsupported:
        raise ValueError(f"Uninventoried source file types: {unsupported}")
    blobs = {path: (root / path).read_bytes() for path in paths}
    tags = ctags_rows(root, [p for p in paths if not p.endswith(".asm")], executable)
    functions, macros = tag_rows(tags, blobs)
    for path in paths:
        if path.endswith(".asm"):
            functions += assembly_rows(path, blobs[path])
    marker_count = attach_addresses(functions, blobs)
    review_path = root / REVIEWS
    reviews = json.loads(review_path.read_text()) if review_path.exists() else {}
    if set(reviews) - set(paths):
        raise ValueError(f"Reviews reference missing files: {sorted(set(reviews) - set(paths))}")
    files = []
    for path in paths:
        sha256 = digest(blobs[path])
        status, note = reviewed(path, sha256, reviews)
        note = note or "-"
        owned = [row for row in functions if row["path"] == path]
        owned_macros = [row for row in macros if row["path"] == path]
        for row in owned + owned_macros:
            row.update(status=status, review=note)
        files.append(dict(path=path, lines=len(blobs[path].splitlines()),
                          functions=len(owned), macros=len(owned_macros),
                          status=status, sha256=sha256, review=note))
    functions.sort(key=lambda row: (row["path"], row["line"], row["name"]))
    macros.sort(key=lambda row: (row["path"], row["line"], row["name"]))
    read_functions = sum(row["status"] == "read" for row in functions)
    read_files = sum(row["status"] == "read" for row in files)
    summary = (
        "# Reading progress\n\nGenerated by `homm1.audit.readability`; do not edit this file.\n\n"
        f"- Files read: {read_files} / {len(files)}\n"
        f"- Function definitions read: {read_functions} / {len(functions)}\n"
        f"- RVA-annotated definitions indexed: {marker_count}\n"
        f"- Macro definitions indexed (including guards/conditional variants): {len(macros)}\n\n"
        "Reading is human-reviewed, not inferred from searches or parser success. A file\n"
        "change invalidates its read status. Declarations without bodies are covered by\n"
        "the file checklist; compiler-generated bodies and vendor code are not invented\n"
        "as source functions. The deliberate pass must reconcile each file's index.\n"
    )
    return {
        "functions.tsv": tsv(functions, ["path", "line", "end", "name", "signature", "kind",
                                         "properties", "rva", "status", "body_sha256", "review"]),
        "macros.tsv": tsv(macros, ["path", "line", "end", "name", "status", "body_sha256", "review"]),
        "files.tsv": tsv(files, ["path", "lines", "functions", "macros", "status", "sha256", "review"]),
        "progress.md": summary,
    }


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=REPO)
    parser.add_argument("--ctags", default="ctags")
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true")
    mode.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)
    root = args.root.resolve()
    products = generate(root, args.ctags)
    stale = []
    for name, content in products.items():
        target = root / REPORT / name
        if args.write:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(content)
        elif not target.exists() or target.read_text() != content:
            stale.append(str(target.relative_to(root)))
    if stale:
        print("Stale or missing reading inventory: " + ", ".join(stale))
        return 1
    print(products["progress.md"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
