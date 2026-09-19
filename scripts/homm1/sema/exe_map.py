"""homm1.sema.exe_map - the retail .text layout map, generated from the Model.

    python3 -m homm1.sema.exe_map            # regenerate docs/exe-map/
                                              # scatter_core.{json,html}
    python3 -m homm1.sema.exe_map --out DIR  # write the pair somewhere else
    python3 -m homm1.sema.exe_map --check    # report only, write nothing

The one generator in the sema package (every other sema module is a read-only
view; this one exists to WRITE the docs/exe-map site, superseding the frozen
scripts/homm1-old/core/exe_map.py + docs/exe-map/scatter.py pair).

What the scatter encodes
------------------------
One dot per TU: x = number of core-body functions the Model attributes to the
unit, y = the number of contiguous FRAGMENTS those functions form in global
retail RVA order.  A fragment is a maximal run of rows that are adjacent in the
image-wide ordering of core rows - `frags == 1` means the unit's core bodies
are one solid block, which is what link.exe 5.10's contribution mechanism
produces for a correctly-partitioned TU.  The y == 1 flatline IS the
linker-layout invariant made visible.

Core-body filter (ported from the frozen scatter.py `is_pooled`, unchanged in
meaning): rows the linker places by COMDAT selection rather than by TU
contribution are dropped before fragment counting - compiler-generated
specials (`??0/??1/??_G/??_E/...`), dynamic-init fragments, and the
vtable-slot virtuals every including TU emits (`@@[UME]AE` + the known slot
names).  Their kept copies are the KEEPER unit's bytes (the Model's
`also_units` flip attributes the proven ones), not evidence of scatter.
Linker bands (`kind` thunk/eh) are not TU rows at all.

Inputs: build/gen/bindings.tsv (the Model), config/units.toml (unit -> source
path).  Outputs: docs/exe-map/scatter_core.json (same row shape as the frozen
generator) and docs/exe-map/scatter_core.html (self-contained, no external
assets).
"""

from __future__ import annotations

import json
import re
import statistics as st

from homm1.core.paths import BUILD, REPO

BINDINGS = BUILD / "gen/bindings.tsv"
OUT_DIR = REPO / "docs/exe-map"

#: COMDAT-pooled name classes (verbatim port of the frozen scatter.py filter).
_DTOR_RE = re.compile(
    r"^(?:\?\?1|\?\?_G|\?\?_E|_\$E|\?\?__E|\?InitStr|_\$S)"
    r"|DeletingDtor|InitStr")
_VSLOT_RE = re.compile(r"@@[UME]AE")
_SLOT_NAME_RE = re.compile(
    r"^\?(?:GetTypeTag|GetRuntimeClass|GetClassId|IsLoaded|"
    r"Serialize[A-Za-z]*|V?[Ss]lot[0-9a-f]{2})@")


def is_pooled(name: str) -> bool:
    return (name.startswith("??") or _DTOR_RE.match(name) is not None
            or _VSLOT_RE.search(name) is not None
            or _SLOT_NAME_RE.match(name) is not None)


def unit_sources() -> dict[str, str]:
    from homm1.manifest import units as manifest_units
    return {u["unit"]: u["source"] for u in manifest_units()}


def core_rows() -> list[dict]:
    """The Model's claimed .text rows minus linker bands and pooled COMDATs,
    in ascending retail RVA order."""
    rows = []
    with BINDINGS.open() as fh:
        header = None
        for ln in fh:
            if ln.startswith("#"):
                continue
            cells = ln.rstrip("\n").split("\t")
            if header is None:
                header = cells
                continue
            r = dict(zip(header, cells))
            if (r["space"] != "text" or not r["unit"]
                    or r["kind"] in ("thunk", "eh") or not r["name"]
                    or is_pooled(r["name"])):
                continue
            rows.append({"unit": r["unit"], "name": r["name"],
                         "rva": int(r["rva"], 16),
                         "size": int(r["size"], 16)})
    rows.sort(key=lambda r: r["rva"])
    return rows


def per_unit_stats(rows: list[dict], sources: dict[str, str]) -> list[dict]:
    stats: dict[str, dict] = {}
    prev = None
    for r in rows:
        key = r["unit"]
        s = stats.setdefault(key, {"n": 0, "bytes": 0, "lo": None, "hi": 0,
                                   "frags": 0})
        s["n"] += 1
        s["bytes"] += r["size"]
        s["lo"] = r["rva"] if s["lo"] is None else min(s["lo"], r["rva"])
        s["hi"] = max(s["hi"], r["rva"] + r["size"])
        if key != prev:
            s["frags"] += 1
        prev = key
    out = []
    for unit, s in stats.items():
        span = s["hi"] - s["lo"]
        out.append({"file": sources.get(unit, unit), "unit": unit,
                    "n": s["n"], "bytes": s["bytes"],
                    "lo": s["lo"], "hi": s["hi"], "span": span,
                    "frags": s["frags"],
                    "avg_cluster": s["n"] / s["frags"],
                    "frag_ratio": s["frags"] / s["n"],
                    "occupancy": s["bytes"] / span if span else 1.0,
                    "spread": span / s["bytes"] if s["bytes"] else 1.0})
    out.sort(key=lambda r: (-r["frags"], -r["n"]))
    return out


def render_html(rows: list[dict]) -> str:
    n_units = len(rows)
    n_fns = sum(r["n"] for r in rows)
    n_frags = sum(r["frags"] for r in rows)
    multi = [r for r in rows if r["frags"] > 1]
    max_n = max(r["n"] for r in rows)
    max_f = max(r["frags"] for r in rows)

    # log-log scatter as inline SVG: x = functions, y = fragments
    import math
    w, h, pad = 760, 420, 48

    def sx(n):
        return pad + (w - 2 * pad) * math.log(n) / math.log(max(max_n, 2))

    def sy(f):
        return h - pad - (h - pad * 2) * (math.log(f)
                                          / math.log(max(max_f, 2)))

    dots = []
    for r in rows:
        c = "#c0392b" if r["frags"] > 1 else "#2c7fb8"
        dots.append(
            f'<circle cx="{sx(max(r["n"], 1)):.1f}" cy="{sy(r["frags"]):.1f}"'
            f' r="3.5" fill="{c}" fill-opacity="0.65">'
            f'<title>{r["unit"]}: {r["n"]} fns, {r["frags"]} frag(s)</title>'
            f'</circle>')
    flat_y = sy(1)
    ticks = []
    t = 1
    while t <= max_n:
        ticks.append(f'<text x="{sx(t):.0f}" y="{h - pad + 16}" '
                     f'text-anchor="middle" class="tick">{t}</text>')
        t *= 10
    multi_rows = "\n".join(
        f"<tr><td>{r['frags']}</td><td>{r['n']}</td><td>{r['unit']}</td>"
        f"<td>{r['file']}</td></tr>" for r in multi)
    return f"""<!doctype html>
<meta charset="utf-8">
<title>HoMM1 exe-map</title>
<style>
 body {{ font: 14px/1.5 system-ui, sans-serif; margin: 2rem auto;
        max-width: 60rem; color: #1a1a1a; background: #fff; }}
 h1 {{ font-size: 1.3rem; }} .tick {{ font-size: 11px; fill: #666; }}
 table {{ border-collapse: collapse; }} td, th {{ border: 1px solid #ccc;
        padding: 2px 8px; font-size: 13px; }}
 .flat {{ stroke: #2c7fb8; stroke-dasharray: 4 3; }}
 code {{ background: #f4f4f4; padding: 0 3px; }}
</style>
<h1>Retail .text core-body scatter (generated from the Model)</h1>
<p>One dot per TU: <b>x</b> = core-body functions the Model attributes to the
unit (log), <b>y</b> = contiguous fragments those functions form in global
retail RVA order (log). link.exe 5.10 lays each obj's contribution as ONE
contiguous block, so a correctly-partitioned TU sits on the dashed
<b>y&nbsp;=&nbsp;1 flatline</b>. Pooled COMDATs (compiler specials, vtable-slot
virtuals, init fragments) are linker-selected, not TU-contributed, and are
excluded before counting; linker bands (ILT thunks, the EH funclet band)
are not TU rows.</p>
<p><b>{n_units}</b> TUs, <b>{n_fns}</b> core functions,
<b>{n_frags}</b> fragments, <b>{len(multi)}</b> multi-fragment TU(s).</p>
<svg viewBox="0 0 {w} {h}" width="{w}" height="{h}"
     style="max-width:100%">
 <line x1="{pad}" y1="{flat_y:.1f}" x2="{w - pad}" y2="{flat_y:.1f}"
       class="flat"/>
 <text x="{w - pad}" y="{flat_y - 6:.1f}" text-anchor="end" class="tick">
   frags = 1 (the linker invariant)</text>
 <line x1="{pad}" y1="{h - pad}" x2="{w - pad}" y2="{h - pad}"
       stroke="#999"/>
 <line x1="{pad}" y1="{pad}" x2="{pad}" y2="{h - pad}" stroke="#999"/>
 <text x="{w // 2}" y="{h - 8}" text-anchor="middle" class="tick">
   core functions per TU (log)</text>
 {"".join(ticks)}
 {"".join(dots)}
</svg>
{"<h2>Multi-fragment TUs (each is a partition defect)</h2>"
 f"<table><tr><th>frags</th><th>fns</th><th>unit</th><th>source</th></tr>"
 f"{multi_rows}</table>" if multi else
 "<p><b>FLATLINE:</b> every TU is a single contiguous fragment.</p>"}
<p>Regenerate: <code>python3 -m homm1.sema.exe_map</code>
(reads <code>build/gen/bindings.tsv</code>).</p>
"""


def main(argv: list[str] | None = None) -> int:
    import argparse
    from pathlib import Path
    ap = argparse.ArgumentParser(
        prog="python3 -m homm1.sema.exe_map", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", type=Path, default=OUT_DIR,
                    help=f"output directory (default {OUT_DIR})")
    ap.add_argument("--check", action="store_true",
                    help="report the fragment census, write nothing")
    args = ap.parse_args(argv)
    out_dir = args.out
    rows = core_rows()
    stats = per_unit_stats(rows, unit_sources())
    if not args.check:
        out_dir.mkdir(parents=True, exist_ok=True)
        json_rows = [{k: v for k, v in r.items() if k != "unit"} for r in stats]
        (out_dir / "scatter_core.json").write_text(
            json.dumps(json_rows, indent=1) + "\n")
        (out_dir / "scatter_core.html").write_text(render_html(stats))
    multi = [r for r in stats if r["frags"] > 1]
    frs = [r["frag_ratio"] for r in stats]
    print(f"[exe-map] {len(stats)} TUs, {sum(r['n'] for r in stats)} core "
          f"functions, {sum(r['frags'] for r in stats)} fragments; "
          f"frag-ratio med {st.median(frs):.2f}; "
          f"multi-fragment TUs: {len(multi)}")
    for r in multi[:20]:
        print(f"    {r['frags']:>3} frags  {r['n']:>3} fns  {r['unit']}")
    print(f"[exe-map] wrote {out_dir / 'scatter_core.json'} + scatter_core.html"
          if not args.check else "[exe-map] --check: nothing written")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
