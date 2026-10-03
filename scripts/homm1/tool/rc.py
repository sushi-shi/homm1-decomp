"""homm1.tool.rc - the era resource compiler (.rc -> .res), gated against retail.

    homm1 tool rc --out <res> --src <rc> [--verify-exe <exe>] [--report <json>]

In-process (same function):
    from homm1.tool import rc
    rc.compile(rc_path, res_path, retail=retail_exe())

RC.EXE is the VC4 driver over RCDLL.DLL from the pinned media
(`config/toolchains.json` vc40 `resource_files`). Repo include/ and the
source's own directory are passed as /i; the HoMM1 script needs no system
header.

With `retail`, the script is compiled from a temporary stage beside the one
retail binary input, the 32x32 program icon. The icon is rebuilt as an
ordinary .ico container from the user's retail RT_ICON payload and
RT_GROUP_ICON directory. It exists only while RC.EXE runs and never becomes a
repository input. Every compiled payload (type, name, language, bytes and
order) is then compared with the retail resource directory in both
directions. Any drift fails and removes the .res. Retail payload order is the
data order in `.rsrc`, which LINK/CVTRES take from the .res record order.
The success signal is the produced .res.
"""

from __future__ import annotations

import json
import shutil
import struct
import tempfile
from pathlib import Path

from homm1.core.paths import INCLUDE
from homm1.tool import ToolError
from homm1.tool.wine import era_tool, run, toolchain_root, winepath

#: The toolchain whose pinned RC/CVTRES compile the candidate resources.
RESOURCE_TOOLCHAIN = "vc41"
#: The file name the resource script gives the retail icon.
RETAIL_ICON = "heroes.ico"

RT_ICON, RT_GROUP_ICON = 3, 14


def _align4(value: int) -> int:
    return (value + 3) & ~3


def read_res(blob: bytes) -> list[dict]:
    """RES32 records in file order, without the leading null record."""

    def identifier(offset: int):
        if struct.unpack_from("<H", blob, offset)[0] == 0xFFFF:
            return struct.unpack_from("<H", blob, offset + 2)[0], offset + 4
        end = offset
        while blob[end:end + 2] != b"\0\0":
            end += 2
        return blob[offset:end].decode("utf-16le"), end + 2

    records, offset = [], 0
    while offset + 8 <= len(blob):
        data_size, header_size = struct.unpack_from("<II", blob, offset)
        rtype, cursor = identifier(offset + 8)
        name, cursor = identifier(cursor)
        cursor = _align4(cursor)
        language = struct.unpack_from("<IHH", blob, cursor)[2]
        data = blob[offset + header_size:offset + header_size + data_size]
        if not (rtype == 0 and name == 0):
            records.append({"type": rtype, "name": name, "language": language,
                            "data": data})
        offset = _align4(offset + header_size + data_size)
    return records


def retail_resources(exe: Path | str) -> list[dict]:
    """The PE resource leaves of `exe`, in payload (data RVA) order."""
    from homm1.core.pe import Pe
    pe = Pe(exe)
    section = pe.section(".rsrc")
    base = section["va"]
    tree = pe.read(base, section["rsize"])
    if tree is None:
        raise ToolError(f"{exe}: unreadable .rsrc")

    def name(value: int):
        if not value & 0x80000000:
            return value
        offset = value & 0x7FFFFFFF
        length = struct.unpack_from("<H", tree, offset)[0]
        return tree[offset + 2:offset + 2 + 2 * length].decode("utf-16le")

    leaves = []

    def walk(offset: int, path: list):
        named, ids = struct.unpack_from("<HH", tree, offset + 12)
        for index in range(named + ids):
            key, target = struct.unpack_from("<II", tree, offset + 16 + 8 * index)
            if target & 0x80000000:
                walk(target & 0x7FFFFFFF, path + [name(key)])
                continue
            rva, size = struct.unpack_from("<II", tree, target)
            rtype, rname = path
            leaves.append({"type": rtype, "name": rname, "language": name(key),
                           "rva": rva, "data": pe.read(rva, size)})

    walk(0, [])
    return sorted(leaves, key=lambda leaf: leaf["rva"])


def icon_container(retail: list[dict]) -> bytes:
    """The .ico file RC splits back into the retail RT_ICON + RT_GROUP_ICON."""
    groups = [r for r in retail if r["type"] == RT_GROUP_ICON]
    if len(groups) != 1:
        raise ToolError(f"expected one retail RT_GROUP_ICON, found {len(groups)}")
    group = groups[0]["data"]
    reserved, kind, count = struct.unpack_from("<HHH", group, 0)
    if (reserved, kind, count) != (0, 1, 1):
        raise ToolError(f"unexpected retail RT_GROUP_ICON directory {group.hex()}")
    width, height, colors, flags, planes, bits, size, ordinal = struct.unpack_from(
        "<BBBBHHIH", group, 6)
    image = next((r["data"] for r in retail
                  if r["type"] == RT_ICON and r["name"] == ordinal), None)
    if image is None or len(image) != size:
        raise ToolError(f"retail RT_ICON {ordinal} does not match its group entry")
    return (struct.pack("<HHH", 0, 1, 1)
            + struct.pack("<BBBBHHII", width, height, colors, flags, planes,
                          bits, size, 22)
            + image)


def compare(ours: list[dict], retail: list[dict]) -> list[str]:
    """Every payload mismatch, both directions; empty when exact."""
    problems = []
    if len(ours) != len(retail):
        problems.append(f"payload count {len(ours)} != retail {len(retail)}")
    for index, (a, b) in enumerate(zip(ours, retail)):
        mine = (a["type"], a["name"], a["language"])
        theirs = (b["type"], b["name"], b["language"])
        if mine != theirs:
            problems.append(f"[{index}] identity {mine} != retail {theirs}")
        elif a["data"] != b["data"]:
            first = next((i for i, (x, y) in enumerate(zip(a["data"], b["data"]))
                          if x != y), min(len(a["data"]), len(b["data"])))
            problems.append(f"[{index}] {mine}: bytes differ (sizes "
                            f"{len(a['data'])}/{len(b['data'])}, first at {first:#x})")
    return problems


def _check_tools() -> None:
    from homm1 import toolchain
    try:
        toolchain.verify_resources(RESOURCE_TOOLCHAIN, toolchain_root())
    except (KeyError, ValueError) as e:
        raise ToolError(f"{e} (`homm1 toolchain install {RESOURCE_TOOLCHAIN} "
                        "--media <iso>` provides RC.EXE/RCDLL.DLL/CVTRES.EXE)") from e


def compile(src: Path | str, out: Path | str, *, flags: list[str] = (),
            extra_includes: list[Path] = (), timeout: float | None = None,
            retail: Path | str | None = None,
            report: Path | str | None = None) -> str:
    """Compile one .rc; return rc.exe's output. Raises ToolError without a
    .res, or (with `retail`) when any payload differs from retail."""
    src, out = Path(src).resolve(), Path(out).resolve()
    if not src.exists():
        raise ToolError(f"resource script missing: {src}")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.unlink(missing_ok=True)

    _check_tools()
    rc_exe = era_tool("rc.exe")
    retail_leaves = retail_resources(retail) if retail is not None else None
    with tempfile.TemporaryDirectory(prefix=".rc-", dir=out.parent) as stage_name:
        script = src
        if retail_leaves is not None:
            script = Path(stage_name) / src.name
            shutil.copyfile(src, script)
            (script.parent / RETAIL_ICON).write_bytes(icon_container(retail_leaves))
        inc = [src.parent, *([INCLUDE] if INCLUDE.is_dir() else []), *extra_includes]
        argv = ["wine", str(rc_exe), "/r", *[f"/i{winepath(d)}" for d in inc],
                *flags, f"/fo{winepath(out)}", script.name]
        output, rc_ = run(argv, cwd=script.parent, timeout=timeout, success=out)
    if not out.exists():
        tail = "\n".join(output.strip().splitlines()[-12:])
        raise ToolError(f"rc produced no .res for {src.name} (rc={rc_}):\n{tail}")
    if retail_leaves is None:
        return output

    ours = read_res(out.read_bytes())
    problems = compare(ours, retail_leaves)
    if report is not None:
        Path(report).write_text(json.dumps({
            "source": src.name,
            "exact": not problems,
            "problems": problems,
            "resources": [{"type": r["type"], "name": r["name"],
                           "language": r["language"], "size": len(r["data"])}
                          for r in ours],
        }, indent=2) + "\n")
    if problems:
        out.unlink(missing_ok=True)
        raise ToolError(f"{src.name} differs from the retail resources:\n  "
                        + "\n  ".join(problems))
    return output


from homm1.core.usage import logged


@logged
def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True)
    ap.add_argument("--src", required=True)
    ap.add_argument("--verify-exe", type=Path,
                    help="retail image: stage its icon and gate every payload")
    ap.add_argument("--report", type=Path,
                    help="JSON payload report (with --verify-exe)")
    ap.add_argument("flags", nargs=argparse.REMAINDER)
    a = ap.parse_args()
    flags = a.flags[1:] if a.flags and a.flags[0] == "--" else a.flags
    try:
        compile(a.src, a.out, flags=flags, retail=a.verify_exe, report=a.report)
    except (ToolError, OSError) as e:
        print(f"[rc] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
