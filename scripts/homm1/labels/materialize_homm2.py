"""Materialize reviewed HoMM2 correspondences as ordinary HoMM1 source stubs.

The input is ``donor_align --graph-output``.  A row is emitted only when the
same HoMM1 function was located independently in Buka 2.1 and PoL 2.0, because
the PoL carcass supplies a VC4-compilable source declaration while Buka keeps
name precedence.  The generated files use the original HoMM2 TU paths and the
normal ``VA`` annotation path; there is no label-provider side channel.
"""

from __future__ import annotations

import argparse
import csv
import re
from bisect import bisect_right
from collections import defaultdict
from pathlib import Path


MARKER = re.compile(r"(?m)^VA\((0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+)\)\s*$")
ALTERNATE = re.compile(
    r"alternate=(buka21|pol20):(.*?)@(0x[0-9a-fA-F]+)(?:;|$)")

# These bodies are already reconstructed in hand-owned source.  Keeping one
# definition avoids duplicate symbols while the two files are folded into
# their donor TUs during subsequent reconstruction.
HAND_OWNED = {0x4F640, 0x4F6B2, 0x5C15C}
DEAD_CODE = {0x52934}
SKIP_RVAS = {
    # An empty virtual-class constructor emits a vftable reference whose three
    # methods are not yet located. Keep the name in evidence, but do not turn
    # it into a source definition until that closure is known.
    0x766E0,
}
OWNED_SNIPPETS = {
    Path("SOURCE/KB.cpp"): [(0x4F640, Path(__file__).with_name("owned") / "KB.inc")],
    Path("SOURCE/kbwin.cpp"): [(0x5C15C, Path(__file__).with_name("owned") / "AppAbout.inc")],
}


def donor_blocks(root: Path):
    blocks = {}
    preambles = {}
    for path in sorted(root.rglob("*.cpp")):
        text = path.read_text(errors="replace")
        matches = list(MARKER.finditer(text))
        if not matches:
            continue
        relative = path.relative_to(root)
        preambles[relative] = text[:matches[0].start()]
        for index, match in enumerate(matches):
            end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
            tail = text.find("// ---- data", match.end(), end)
            if tail >= 0:
                end = tail
            va = int(match.group(1), 16)
            blocks[va - 0x400000] = (relative, text[match.start():end].rstrip())
    return blocks, preambles


def buka_identity(symbol: str) -> str:
    """Comparable ``class::function`` identity from a VC decorated name."""
    if symbol.startswith("??0"):
        owner = symbol[3:].split("@@", 1)[0]
        return f"{owner}::{owner}".casefold()
    if symbol.startswith("??1"):
        owner = symbol[3:].split("@@", 1)[0]
        return f"{owner}::~{owner}".casefold()
    if symbol.startswith("?"):
        fields = symbol[1:].split("@@", 1)[0].split("@")
        return "::".join(reversed(fields)).casefold()
    return re.sub(r"@\d+$", "", symbol.lstrip("@_")).casefold()


def pol_identity(symbol: str) -> str:
    """Comparable identity from the CodeView materializer's declaration."""
    head = symbol.split(";", 1)[0].split("(", 1)[0].strip()
    name = head.split()[-1].lstrip("*&")
    if "::constructor" in name:
        owner = name.split("::", 1)[0]
        name = f"{owner}::{owner}"
    elif "::~destructor" in name:
        owner = name.split("::", 1)[0]
        name = f"{owner}::~{owner}"
    return re.sub(r"@\d+$", "", name.lstrip("@_")).casefold()


def buka_symbols(path: Path) -> dict[int, tuple[str, str]]:
    out = {}
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream):
            if row.get("kind") == "func":
                out[int(row["rva"], 0)] = (row["name"], row["unit"])
    return out


def load_segments(path: Path | None):
    """Return sorted HoMM1 address ranges with the Buka-inferred TU owner."""
    if path is None:
        return []
    out = []
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            out.append((int(row["start_rva"], 0), int(row["end_rva"], 0),
                        row["unit"]))
    return sorted(out)


def segment_owner(rva: int, segments) -> str | None:
    if not segments:
        return None
    starts = [row[0] for row in segments]
    index = bisect_right(starts, rva) - 1
    if index >= 0 and rva < segments[index][1]:
        return segments[index][2]
    return None


def rewrite_preamble(relative: Path) -> str:
    # The historical PoL carcass carried each original TU's guessed globals,
    # local views and external prototypes because it was also a source-recovery
    # workspace.  HoMM1 uses it only for signatures at this stage.  Shared H2
    # headers already own the class and by-value record declarations, so keep
    # generated TUs clean from their first commit.
    text = ("// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 "
            "declaration.\n\n"
            "#include <match.h>\n\n")
    if relative == Path("SOURCE/kbwin.cpp"):
        text += "#define WIN32_LEAN_AND_MEAN\n#include <windows.h>\n\n"
    if relative in {Path("SOURCE/KB.cpp"), Path("SOURCE/kbwin.cpp")}:
        text += "#include <H1/KB.h>\n"
    text += "#include <H2/_all.h>\n"
    return text


def write_units_config(path: Path, rows) -> None:
    text = """# Per-TU build contract, following Gruntz. See config/README.md.
[build]
platform = "win32"
compiler = "vc40"
compiler_status = "VC4.0 /Od compatible with AppAbout and PollSound fragments; historical compiler revision unconfirmed"

[flags]
cpp_od = ["/nologo", "/c", "/Od", "/Z7"]
# HoMM2 supplies declarations and ownership. HoMM1's exact PollSound family
# proves the free-function convention is the compiler default (/Gd), unlike
# the donor's /Gr build; reconstruction replaces these bodies in place.
cpp_carcass = ["/nologo", "/c", "/Od", "/Z7", "/G5", "/Ob1"]
cpp_carcass_oi = ["/nologo", "/c", "/Od", "/Z7", "/G5", "/Ob1", "/Oi"]
"""
    for unit, source, _count in rows:
        flags = "cpp_carcass_oi" if unit == "BASE/RESMGR" else "cpp_carcass"
        text += (f"\n[[unit]]\nunit = \"{unit}\"\n"
                 f"source = \"{source}\"\nflags = \"{flags}\"\n")
    path.write_text(text)


def remove_previous_outputs(manifest: Path, output: Path) -> None:
    """Remove only files emitted by the preceding materialization pass.

    Generated donor TUs live in the ordinary source tree.  Recursively
    clearing ``output`` would therefore delete hand-reconstructed and newly
    added sources as the campaign progresses.  The generated manifest is the
    exact ownership record for stale-output cleanup.
    """
    if not manifest.is_file():
        return
    root = output.resolve()
    with manifest.open(newline="") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            source = Path(row["source"])
            if not source.is_absolute():
                source = Path.cwd() / source
            source = source.resolve()
            try:
                source.relative_to(root)
            except ValueError:
                continue
            if source.is_file():
                source.unlink()


def empty_body(block: str) -> str:
    """Replace donor code with the minimal compiling source body."""
    start = block.find("{")
    if start < 0:
        raise ValueError("donor function block has no body")
    depth = 0
    for index in range(start, len(block)):
        if block[index] == "{":
            depth += 1
        elif block[index] == "}":
            depth -= 1
            if depth == 0:
                original = block[start:index + 1]
                # The donor carcass already has the correct zero expression
                # for awkward by-value returns such as tag_message.
                if re.fullmatch(r"\{\s*return\s+[^;]+;\s*\}", original, re.S):
                    replacement = original
                else:
                    signature = block[MARKER.search(block).end():start].strip()
                    head = signature.split("(", 1)[0].strip()
                    name = head.split()[-1].lstrip("*")
                    qualified = name.split("::")
                    constructor = (len(qualified) >= 2 and
                                   qualified[-1].lstrip("~") == qualified[-2])
                    prefix = head[:head.rfind(name)].strip()
                    plain_prefix = re.sub(
                        r'(?:extern\s+"C"|__cdecl|__stdcall|__fastcall)',
                        " ", prefix,
                    )
                    returns_void = bool(re.search(r"\bvoid\s*$", plain_prefix.strip()))
                    aggregate = re.fullmatch(r"\s*(struct|class)\s+([A-Za-z_][A-Za-z0-9_]*)\s*",
                                             plain_prefix)
                    aggregate_alias = re.fullmatch(r"\s*([A-Z][A-Za-z0-9_]*)\s*",
                                                   plain_prefix)
                    if constructor or returns_void:
                        replacement = "{}"
                    elif aggregate:
                        replacement = ("{ return *(%s %s *)0; }" %
                                       (aggregate.group(1), aggregate.group(2)))
                    elif aggregate_alias:
                        replacement = ("{ return *(%s *)0; }" %
                                       aggregate_alias.group(1))
                    else:
                        replacement = "{ return 0; }"
                return block[:start] + replacement + block[index + 1:]
    raise ValueError("unterminated donor function body")


from homm1.core.usage import logged


@logged
def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--matches", type=Path, required=True)
    parser.add_argument("--buka21-symbols", type=Path, required=True)
    parser.add_argument("--donor-stubs", type=Path, required=True)
    parser.add_argument("--segments", type=Path,
                        help="Buka-inferred contiguous HoMM1 TU ranges")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--config", type=Path,
                        help="rewrite units.toml with the materialized donor TUs")
    args = parser.parse_args(argv)

    blocks, preambles = donor_blocks(args.donor_stubs)
    buka_by_rva = buka_symbols(args.buka21_symbols)
    segments = load_segments(args.segments)
    selected = defaultdict(list)
    with args.matches.open(newline="") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            rva = int(row["rva"], 0)
            if rva in HAND_OWNED or rva in SKIP_RVAS or row["unit"].startswith("("):
                continue
            if ("alternate=" not in row["evidence"]
                    and "reviewed-anchor" not in row["evidence"]):
                continue
            alternate = ALTERNATE.search(row["evidence"])
            if alternate is None:
                continue
            alternate_donor, alternate_symbol = alternate.group(1), alternate.group(2)
            alternate_rva = int(alternate.group(3), 0)
            donor_rva = None
            pol_symbol = None
            preferred = None
            if row["donor"] == "pol20":
                donor_rva = int(row["donor_rva"], 0)
                pol_symbol = row["symbol"]
                if alternate_donor == "buka21":
                    preferred = buka_by_rva.get(alternate_rva)
            else:
                preferred = (row["symbol"], row["unit"])
                if alternate_donor == "pol20":
                    donor_rva = alternate_rva
                    pol_symbol = alternate_symbol
            if (donor_rva is None or donor_rva not in blocks or preferred is None
                    or pol_symbol is None):
                continue
            # Independent builds confirm a name only when they identify the
            # same logical function. A different symbol at the same target RVA
            # is contradictory evidence, not a second vote.
            if buka_identity(preferred[0]) != pol_identity(pol_symbol):
                continue
            _pol_relative, block = blocks[donor_rva]
            owner = segment_owner(rva, segments) or preferred[1]
            if owner.startswith("("):
                continue
            relative = Path(owner + ".cpp")
            selected[relative].append((rva, int(row["size"], 0), donor_rva, block,
                                       preferred[0], preferred[1], row["evidence"]))

    manifest_rows = []
    remove_previous_outputs(args.manifest, args.output)
    for relative, rows in sorted(selected.items(), key=lambda item: str(item[0])):
        destination = args.output / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        chunks = [rewrite_preamble(relative)]
        snippets = list(OWNED_SNIPPETS.get(relative, []))
        for rva, size, donor_rva, block, symbol, donor_unit, evidence in sorted(rows):
            while snippets and snippets[0][0] < rva:
                _owned_rva, owned_path = snippets.pop(0)
                chunks.append(owned_path.read_text().rstrip() + "\n")
            block = empty_body(block)
            block = MARKER.sub(f"VA(0x{0x400000 + rva:08x}, 0x{size:x})", block, count=1)
            block = block.replace('extern "C"', "H2_C_LINKAGE")
            block = re.sub(r"\ba1\b", "firstValue", block)
            block = re.sub(r"\bp6\b", "cellFlags", block)
            dead = ("// @dead-code\n"
                    "// Zero-ref: no effective incoming retail reference.\n"
                    if rva in DEAD_CODE else "")
            chunks.append(
                f"// donor PoL RVA 0x{donor_rva:08x}; preferred Buka symbol {symbol}\n"
                f"// donor Buka TU {donor_unit}; HoMM1 owner inferred from contiguous order\n"
                f"// evidence: {evidence}\n{dead}{block.strip()}\n"
            )
        for _owned_rva, owned_path in snippets:
            chunks.append(owned_path.read_text().rstrip() + "\n")
        destination.write_text("\n".join(chunks).rstrip() + "\n")
        unit = str(relative.with_suffix(""))
        manifest_rows.append((unit, str(destination),
                              len(rows) + len(OWNED_SNIPPETS.get(relative, []))))

    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    with args.manifest.open("w", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(("unit", "source", "functions"))
        writer.writerows(manifest_rows)
    if args.config:
        write_units_config(args.config, manifest_rows)
    print(f"materialized {sum(len(rows) for rows in selected.values())} functions "
          f"in {len(selected)} donor TUs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
