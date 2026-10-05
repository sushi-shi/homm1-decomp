"""homm1.verify.lzhuf_oracle - retail golden comparison for tools/homm1-lzhuf.

    homm1 verify lzhuf-oracle [--data DIR] [--input FILE ...] [--quick]

Runs the same operation scripts through two engines and compares every
output byte:

* retail: `tools/homm1-lzhuf/oracle/` (a DLL plus an executable stub that
  holds the address range), compiled with the pinned VC6 toolchain under
  Wine, loads the hash-verified HEROES.EXE sections at their fixed base and
  calls the game's own EncodeData/DecodeData. PollSound is patched to RET;
  nothing else is substituted. Addresses come from the source claims
  (homm1.model), not from constants here.
* Rust: `homm1-lzhuf replay`, built with `cargo --offline`.

Phase 1 encodes a deterministic generated corpus plus any real files (game
saves, REMOTE.GAM, maps; `--data` scans an installed game folder and defaults
to the folder `homm1 play` remembered) from a fresh process state, and runs
stateful sessions that expose the shared window (stale look-ahead, a seeded
window, back-to-back calls, a fresh receiver). Phase 2 decodes the retail
streams with the encoder's space prefill and with a fresh zeroed window.
Decoded outputs must also equal the original inputs (prefill case).

Everything is written under build/lzhuf-oracle/. Exit status 1 on any
difference.
"""

from __future__ import annotations

import argparse
import filecmp
import shutil
import subprocess
import sys
from pathlib import Path

from homm1.core.paths import BUILD, REPO
from homm1.core.usage import logged

CRATE = REPO / "tools"
ORACLE = REPO / "tools/homm1-lzhuf/oracle"
WORK = BUILD / "lzhuf-oracle"

ENCODE = "?EncodeData@@YAHPAD0I@Z"
DECODE = "?DecodeData@@YAHPAD0@Z"
POLL = "_PollSound"
WINDOW = "_text_buf"

PREFILL = 4096 - 60
#: data files worth compressing from an installed game folder
DATA_SUFFIXES = (".gam", ".gm1", ".map", ".cmp", ".hs", ".dat")
DATA_LIMIT = 24


def _addresses() -> dict[str, int]:
    """Virtual addresses of the codec entry points and window, from claims."""
    from homm1.core.image import Image
    from homm1.core.inputs import read_verified, targets
    from homm1.model import resolve

    pin = targets()["game"]
    base = Image(read_verified(pin, pin.destination)).report()["image_base"]
    model = resolve()
    found: dict[str, tuple[int, int]] = {}
    for row in model.functions + model.data:
        names = {row.name, *(getattr(alias, "name", "") for alias in row.aliases)}
        for wanted in (ENCODE, DECODE, POLL, WINDOW):
            if wanted in names:
                found[wanted] = (row.rva, row.size)
    missing = [name for name in (ENCODE, DECODE, POLL, WINDOW) if name not in found]
    if missing:
        raise SystemExit(f"lzhuf-oracle: no claim for {', '.join(missing)}")
    window_rva, window_size = found[WINDOW]
    if window_size != 4096 + 60 - 1:
        raise SystemExit(f"lzhuf-oracle: {WINDOW} is {window_size} bytes, expected 4155")
    return {"encode": base + found[ENCODE][0], "decode": base + found[DECODE][0],
            "poll": base + found[POLL][0], "window": base + window_rva,
            "window_size": window_size}


def lcg_bytes(seed: int, count: int, mask: int = 0xFF) -> bytes:
    """ANSI C `rand` stream, also implemented by the crate's vector tests."""
    state, out = seed, bytearray(count)
    for index in range(count):
        state = (state * 1103515245 + 12345) & 0x7FFFFFFF
        out[index] = (state >> 16) & mask
    return bytes(out)


def geometric_bytes(seed: int, count: int) -> bytes:
    """Trailing zero count of each 15-bit LCG value: P(n) = 2^-(n+1)."""
    state, out = seed, bytearray(count)
    for index in range(count):
        state = (state * 1103515245 + 12345) & 0x7FFFFFFF
        value = (state >> 16) | 0x8000
        out[index] = (value & -value).bit_length() - 1
    return bytes(out)


def generated(quick: bool = False) -> dict[str, bytes]:
    """Deterministic inputs covering the codec's edge cases. The crate's
    `tests/retail_vectors.rs` regenerates the same inputs and pins the retail
    outputs this oracle records in build/lzhuf-oracle/vectors.tsv."""
    cases: dict[str, bytes] = {
        "empty": b"",
        "byte-00": b"\x00",
        "byte-20": b" ",
        "byte-41": b"A",
        "byte-ff": b"\xff",
        "two": b"AB",
        "three-spaces": b"   ",
        "abc": b"abc",
        "abcabcabc": b"abcabcabc",
        "spaces-100": b" " * 100,
        "leading-spaces": b"    leading spaces reach into the prefill" * 3,
        "ramp": bytes(range(256)) * 4,
        "zeros-1000": bytes(1000),
        "a-59": b"a" * 59,
        "a-60": b"a" * 60,
        "a-61": b"a" * 61,
        "a-4095": b"a" * 4095,
        "a-4096": b"a" * 4096,
        "a-4097": b"a" * 4097,
        "random-17": lcg_bytes(17, 17),
        "random-1000": lcg_bytes(1000, 1000),
        "random-4096": lcg_bytes(4096, 4096),
    }
    if not quick:
        cases.update({
            "zeros-100000": bytes(100000),
            "spaces-5000": b" " * 5000,
            "a-70000": b"a" * 70000,
            "random-65536": lcg_bytes(65536, 65536),
            "random-300000": lcg_bytes(300000, 300000),
            # Four symbols: long, frequent matches and many tree rebuilds.
            "alphabet4-200000": lcg_bytes(4, 200000, 3),
            # Geometric symbol frequencies drive deep, rebuilt Huffman trees.
            "geometric-150000": geometric_bytes(2, 150000),
        })
    return cases


def _corpus(quick: bool) -> dict[str, bytes]:
    cases = generated(quick)
    cases["text-encoder"] = (REPO / "vendor/lzhuf/encoder.cpp").read_bytes()
    if not quick:
        cases["text-readme"] = (REPO / "README.md").read_bytes()
        cases["retail-image"] = (BUILD / "orig/HEROES.EXE").read_bytes()
    return cases


def _data_files(data: Path | None, inputs: list[Path]) -> dict[str, bytes]:
    files: list[Path] = list(inputs)
    if data is not None and data.is_dir():
        found = sorted(p for p in data.rglob("*")
                       if p.is_file() and p.suffix.lower() in DATA_SUFFIXES)
        files += found[:DATA_LIMIT]
    out: dict[str, bytes] = {}
    for path in files:
        name = "real-" + "".join(c if c.isalnum() or c in ".-" else "_"
                                 for c in path.name)
        while name in out:
            name += "_"
        out[name] = path.read_bytes()
    return out


def _sessions() -> tuple[dict[str, bytes], list[str]]:
    """Stateful scripts: the window persists between calls, as in the game."""
    files = {
        "session-seed.window": lcg_bytes(4155, 4155),
        "session-short-1": b"hello",
        "session-short-2": b"abcabcab",
        "session-short-3": lcg_bytes(59, 59, 7),
        "session-medium": lcg_bytes(5000, 5000, 15),
        # Short enough that stale look-ahead bytes decide the match.
        "session-stale": lcg_bytes(17511, 11, 3),
    }
    lines = [
        "# stale look-ahead: a seeded window changes short encodings",
        "reset", "window session-seed.window",
        "encode session-short-1 session-seeded-1.lz",
        "dump-window session-seeded-1.window",
        "encode session-short-2 session-seeded-2.lz",
        "encode session-short-3 session-seeded-3.lz",
        "dump-window session-seeded-3.window",
        "# back-to-back calls in one process",
        "reset", "encode session-medium session-chain-1.lz",
        "encode session-short-2 session-chain-2.lz",
        "encode session-short-1 session-chain-3.lz",
        "dump-window session-chain.window",
        "# the same input from a fresh and from a seeded window",
        "reset", "encode session-stale session-stale-fresh.lz",
        "reset", "window session-seed.window",
        "encode session-stale session-stale-seeded.lz",
    ]
    return files, lines


def fnv1a64(data: bytes) -> int:
    value = 0xCBF29CE484222325
    for byte in data:
        value = ((value ^ byte) * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return value


def _cargo_binary() -> Path:
    target = BUILD / "cargo"
    cmd = ["cargo", "build", "--offline", "--release", "--quiet",
           "--manifest-path", str(CRATE / "Cargo.toml"), "--target-dir", str(target)]
    try:
        subprocess.run(cmd, check=True)
    except FileNotFoundError as exc:
        raise SystemExit("lzhuf-oracle: cargo not found - run inside `nix develop`") from exc
    return target / "release/homm1-lzhuf"


def _harness() -> Path:
    """Build retail_oracle.dll and its image_stub.exe with the pinned VC6."""
    from homm1.tool import ToolError
    from homm1.tool.wine import ensure_link_deps, era_tool, run, winepath

    out = WORK.parent / "lzhuf-oracle-harness"
    out.mkdir(parents=True, exist_ok=True)
    dll, exe = out / "retail_oracle.dll", out / "image_stub.exe"
    sources = [ORACLE / "retail_oracle.c", ORACLE / "image_stub.c"]
    newest = max(source.stat().st_mtime for source in sources)
    if exe.exists() and dll.exists() and min(exe.stat().st_mtime, dll.stat().st_mtime) >= newest:
        return exe
    ensure_link_deps()
    cl = str(era_tool("cl.exe"))
    steps = [
        (dll, ["/LD", f"/Fo{winepath(out / 'retail_oracle.obj')}", f"/Fe{winepath(dll)}",
               winepath(sources[0]), "/link", "/BASE:0x10000000"]),
        (exe, [f"/Fo{winepath(out / 'image_stub.obj')}", f"/Fe{winepath(exe)}",
               winepath(sources[1]), winepath(out / "retail_oracle.lib"),
               "/link", "/BASE:0x400000", "/FIXED"]),
    ]
    for target, flags in steps:
        target.unlink(missing_ok=True)
        try:
            output, rc = run(["wine", cl, "/nologo", "/O2", "/W3", "/MT", *flags],
                             cwd=out, success=target)
        except ToolError as exc:
            raise SystemExit(f"lzhuf-oracle: {exc}") from exc
        if not target.exists():
            raise SystemExit(f"lzhuf-oracle: building {target.name} failed (rc={rc}):\n{output}")
    return exe


def _run_retail(exe: Path, script: Path, outdir: Path, addresses: dict[str, int]) -> None:
    from homm1.core.inputs import targets
    from homm1.tool.wine import run, winepath

    outdir.mkdir(parents=True, exist_ok=True)
    argv = ["wine", str(exe), winepath(targets()["game"].destination), winepath(script),
            winepath(outdir), *(hex(addresses[k]) for k in ("encode", "decode", "poll", "window")),
            str(addresses["window_size"])]
    output, rc = run(argv, cwd=outdir, timeout=1800)
    if rc != 0 or "retail_oracle:" in output:
        raise SystemExit(f"lzhuf-oracle: retail run of {script.name} failed (rc={rc}):\n{output}")


def _run_rust(binary: Path, script: Path, outdir: Path) -> None:
    outdir.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(binary), "replay", str(script), str(outdir)], check=True)


def _compare(names: list[str], left: Path, right: Path) -> list[str]:
    findings = []
    for name in names:
        a, b = left / name, right / name
        if not a.exists() or not b.exists():
            findings.append(f"{name}: missing {'retail' if not a.exists() else 'rust'} output")
        elif not filecmp.cmp(a, b, shallow=False):
            findings.append(f"{name}: retail and rust outputs differ "
                            f"({a.stat().st_size} vs {b.stat().st_size} bytes)")
    return findings


def _phase(name: str, lines: list[str], outputs: list[str], corpus: Path,
           exe: Path, binary: Path, addresses: dict[str, int]) -> list[str]:
    script = corpus / f"{name}.script"
    script.write_text("\n".join(lines) + "\n")
    retail, rust = WORK / "retail", WORK / "rust"
    _run_retail(exe, script, retail, addresses)
    _run_rust(binary, script, rust)
    findings = _compare(outputs, retail, rust)
    print(f"[lzhuf-oracle] {name}: {len(outputs) - len(findings)}/{len(outputs)} outputs identical")
    return findings


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="homm1 verify lzhuf-oracle", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--data", type=Path,
                    help="installed game folder to scan for saves and maps "
                         "(default: the folder `homm1 play` remembered)")
    ap.add_argument("--input", type=Path, action="append", default=[],
                    help="additional real input file (repeatable)")
    ap.add_argument("--quick", action="store_true", help="small generated corpus only")
    a = ap.parse_args(argv)

    data = a.data
    if data is None:
        from homm1.graph import play
        from homm1.graph.verbs import GAME_ENV
        data = play.remembered(REPO / GAME_ENV, "data", None)

    addresses = _addresses()
    if WORK.exists():
        shutil.rmtree(WORK)
    corpus = WORK / "corpus"
    corpus.mkdir(parents=True)

    inputs = _corpus(a.quick)
    inputs.update(_data_files(data, a.input))
    session_files, session_lines = _sessions()
    for name, payload in {**inputs, **session_files}.items():
        (corpus / name).write_bytes(payload)
    (corpus / "prefill.window").write_bytes(b" " * PREFILL)

    binary = _cargo_binary()
    exe = _harness()

    lines, outputs = [], []
    for name in inputs:
        lines += ["reset", f"encode {name} {name}.lz", f"dump-window {name}.enc-window"]
        outputs += [f"{name}.lz", f"{name}.enc-window"]
    lines += session_lines
    outputs += [line.split()[-1] for line in session_lines
                if line.split()[0] in ("encode", "dump-window")]
    findings = _phase("encode", lines, outputs, corpus, exe, binary, addresses)

    retail = WORK / "retail"
    lines, outputs = [], []
    for name in inputs:
        shutil.copyfile(retail / f"{name}.lz", corpus / f"{name}.lz")
        lines += ["reset", "window prefill.window", f"decode {name}.lz {name}.out",
                  "reset", f"decode {name}.lz {name}.fresh",
                  f"dump-window {name}.dec-window"]
        outputs += [f"{name}.out", f"{name}.fresh", f"{name}.dec-window"]
    findings += _phase("decode", lines, outputs, corpus, exe, binary, addresses)

    vectors = WORK / "vectors.tsv"
    rows = []
    for name in sorted(set(generated()) & set(inputs)):
        for suffix in (".lz", ".fresh", ".enc-window"):
            path = retail / f"{name}{suffix}"
            if path.exists():
                payload = path.read_bytes()
                rows.append(f"{name}{suffix}\t{len(payload)}\t{fnv1a64(payload):016x}")
    for line in session_lines:
        words = line.split()
        if words[0] in ("encode", "dump-window"):
            payload = (retail / words[-1]).read_bytes()
            rows.append(f"{words[-1]}\t{len(payload)}\t{fnv1a64(payload):016x}")
    vectors.write_text("name\tlength\tfnv1a64\n" + "\n".join(rows) + "\n")

    roundtrip = [name for name in inputs
                 if (retail / f"{name}.out").read_bytes() != inputs[name]]
    findings += [f"{name}: retail decode of the retail stream is not the input"
                 for name in roundtrip]
    fresh = sum((retail / f"{name}.fresh").read_bytes() != inputs[name] for name in inputs)
    total = sum(len(v) for v in inputs.values())
    real = sum(name.startswith("real-") for name in inputs)
    print(f"[lzhuf-oracle] {len(inputs)} inputs ({real} real files, {total} bytes); "
          f"{len(inputs) - len(roundtrip)} retail round trips exact; "
          f"{fresh} differ when decoded with a fresh zeroed window")
    for finding in findings:
        print(f"[lzhuf-oracle] FAIL {finding}")
    print(f"[lzhuf-oracle] {'ok' if not findings else f'{len(findings)} difference(s)'}")
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
