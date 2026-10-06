"""homm1.verify.link_diff - the linked candidate may not drift from retail.

    homm1 verify link-diff            # relink if stale, compare, exit 1 on a rise
    homm1 verify link-diff --update   # bless the current counts as the ceiling

Counts the bytes in which the linked candidate (`homm1 link`) differs from
the retail image: the headers before the first section, each section's raw
data, the overlay after the last section, and the file size. The committed
ceiling, config/link_diff.tsv, holds the highest count each region may have.
A count above it fails the gate; a lower count passes and is reported as
bankable with `--update`.

Each image with a linked candidate banks its own ceiling beside its retail
facts (`homm1 --image editor verify link-diff --update`:
config/retail/editor/link_diff.tsv), and the gate checks every image that
has one.
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

from homm1.core.paths import CONFIG, DEFAULT_IMAGE, REPO, image_key, retail_dir


def ceiling_path(image: str | None = None) -> Path:
    """The image's committed ceiling: config/link_diff.tsv for the game,
    config/retail/<image>/link_diff.tsv for another image."""
    key = image or image_key()
    return CONFIG / "link_diff.tsv" if key == DEFAULT_IMAGE else retail_dir(key) / "link_diff.tsv"


CEILING = ceiling_path()


def _sections(data: bytes) -> list[tuple[str, int, int]]:
    """[(name, raw offset, raw size)] in file order."""
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    opt = struct.unpack_from("<H", data, pe + 20)[0]
    out = []
    for i in range(count):
        o = pe + 24 + opt + 40 * i
        name = data[o:o + 8].rstrip(b"\0").decode("latin-1")
        raw_size, raw_ptr = struct.unpack_from("<II", data, o + 16)
        out.append((name, raw_ptr, raw_size))
    return out


def _differing(a: bytes, b: bytes) -> int:
    n = min(len(a), len(b))
    return sum(1 for i in range(n) if a[i] != b[i]) + abs(len(a) - len(b))


def regions(retail: bytes, cand: bytes) -> dict[str, int]:
    """{region: differing byte count} over the retail section layout."""
    secs = _sections(retail)
    out = {"size": abs(len(retail) - len(cand))}
    first = min(p for _n, p, _s in secs)
    out["headers"] = _differing(retail[:first], cand[:first])
    end = first
    for name, ptr, size in secs:
        out[name] = _differing(retail[ptr:ptr + size], cand[ptr:ptr + size])
        end = max(end, ptr + size)
    out["overlay"] = _differing(retail[end:], cand[end:])
    return out


def read_ceiling(image: str | None = None) -> dict[str, int]:
    path = ceiling_path(image)
    if not path.is_file():
        return {}
    out = {}
    for ln in path.read_text().splitlines():
        if not ln or ln.startswith("#") or ln.startswith("region\t"):
            continue
        name, count = ln.split("\t")[:2]
        out[name] = int(count)
    return out


def write_ceiling(counts: dict[str, int], image: str | None = None) -> None:
    lines = ["# Linked-candidate ceiling: bytes differing from retail per region.",
             "# Written only by `homm1 verify link-diff --update`.",
             "region\tdiffering"]
    lines += [f"{k}\t{v}" for k, v in counts.items()]
    ceiling_path(image).write_text("\n".join(lines) + "\n")


def measure(relink: bool = True, image: str | None = None) -> dict[str, int]:
    """Per-region counts of the image's linked candidate (relinked first
    unless `relink` is false; only the selected image can relink)."""
    from homm1 import graph
    from homm1.core.paths import retail_exe
    key = image or image_key()
    if relink:
        if key != image_key():
            raise SystemExit(f"relink {key} under `homm1 --image {key}`")
        from homm1.graph.verbs import link_main
        if link_main([]):
            raise SystemExit("homm1 link failed")
    cand = REPO / graph.image_paths(key)["CANDIDATE_EXE"]
    if not cand.is_file():
        raise SystemExit(f"no linked candidate at {cand}")
    return regions(Path(retail_exe(key)).read_bytes(), cand.read_bytes())


def gated_images() -> list[str]:
    """The game and every image that banked a ceiling."""
    from homm1.core.paths import images
    return [i for i in images() if i == DEFAULT_IMAGE or ceiling_path(i).is_file()]


def gate_findings(relink: bool = True) -> list[str]:
    out = []
    for image in gated_images():
        tag = "" if image == DEFAULT_IMAGE else f"{image} "
        ceiling = read_ceiling(image)
        if not ceiling:
            out.append(f"no ceiling in {ceiling_path(image).relative_to(REPO)}: run "
                       "`homm1 verify link-diff --update`")
            continue
        counts = measure(relink and image == image_key(), image)
        for name, count in counts.items():
            limit = ceiling.get(name)
            if limit is None:
                out.append(f"{tag}{name}: {count} differing byte(s), no banked ceiling")
            elif count > limit:
                out.append(f"{tag}{name}: {count} differing byte(s) > ceiling {limit}")
    return out


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 verify link-diff",
                                 description=__doc__.splitlines()[0])
    ap.add_argument("--update", action="store_true",
                    help="bless the current per-region counts as the ceiling")
    ap.add_argument("--no-link", action="store_true",
                    help="compare the existing candidate without relinking")
    a = ap.parse_args(argv)
    counts = measure(relink=not a.no_link)
    ceiling = read_ceiling()
    for name, count in counts.items():
        limit = ceiling.get(name)
        mark = ("" if limit is None or count == limit
                else " (above ceiling)" if count > limit else " (bankable)")
        print(f"[link-diff] {name:9} {count:7d} differing"
              f"{'' if limit is None else f'  ceiling {limit}'}{mark}")
    print(f"[link-diff] total {sum(counts.values())}")
    if a.update:
        write_ceiling(counts)
        print(f"[link-diff] ceiling written to {ceiling_path().relative_to(REPO)}")
        return 0
    bad = [n for n, c in counts.items() if ceiling.get(n) is None or c > ceiling[n]]
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
