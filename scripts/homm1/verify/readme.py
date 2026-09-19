"""homm1.verify.readme - the README score block + the one-time reconciliation.

The block between the markers is generated; nothing outside them is touched.
The first overwrite of an OLD-pipeline block (recognized by its generator
signature) emits a RECONCILIATION - the old block's numbers against the newly
computed ones, delta attributed per class - printed loudly, never written into
the README. The new numbers are the truth; they are not bent to match the old
block.
"""

from __future__ import annotations

import re

from homm1.core.paths import REPO
from homm1.verify.baseline import EPS
from homm1.verify.scores import measure

README = REPO / "README.md"
RM_START = "<!-- match-score:start -->"
RM_END = "<!-- match-score:end -->"
OLD_SIGNATURE = "homm1.match.status"     # the frozen pipeline's generator tag
NEW_SIGNATURE = "homm1 verify check` and `homm1 verify bank"


def _pct(num: float, den: float) -> float:
    return 100.0 * num / den if den else 0.0


def module_of(source: str) -> str:
    """Group units for the rollup by the meaningful path component."""
    from pathlib import PurePosixPath
    parts = PurePosixPath(source).parts
    if not parts:
        return "?"
    if len(parts) > 2 and parts[:2] == ("src", "carcass"):
        return parts[2]
    if parts[0] in ("src", "vendor") and len(parts) > 1:
        return parts[1]
    return parts[0]


def unit_modules() -> dict[str, str]:
    from homm1 import manifest
    return {u["unit"].rsplit("/", 1)[-1]: module_of(u.get("source", ""))
            for u in manifest.units()}


def _md_table(headers: list[str], aligns: str, rows: list[list[str]]) -> list[str]:
    widths = [len(h) for h in headers]
    for r in rows:
        for i, c in enumerate(r):
            widths[i] = max(widths[i], len(c))

    def cell(text: str, i: int) -> str:
        return text.rjust(widths[i]) if aligns[i] == "r" else text.ljust(widths[i])

    def row(cells: list[str]) -> str:
        return "| " + " | ".join(cell(c, i) for i, c in enumerate(cells)) + " |"

    sep = ["-" * (w - 1) + ":" if a == "r" else ":" + "-" * (w - 1)
           for w, a in zip(widths, aligns)]
    return [row(headers), "| " + " | ".join(sep) + " |", *(row(r) for r in rows)]


def collect_modules(umeas: dict[str, dict]):
    """Per-module aggregates from the (EH-carved) unit measures, plus the
    started-unit fuzzy-weighted sum + code."""
    modules = unit_modules()
    mods: dict[str, dict] = {}
    started_fzw = 0.0
    started_code = 0
    for unit, m in umeas.items():
        mod = modules.get(unit, "?")
        tc = int(m.get("total_code") or 0)
        fz = float(m.get("fuzzy_match_percent") or 0.0)
        a = mods.setdefault(mod, {"tc": 0, "mc": 0, "fzw": 0.0, "tf": 0,
                                  "mf": 0, "units": 0, "cw": 0.0})
        a["tc"] += tc
        a["mc"] += int(m.get("matched_code") or 0)
        a["fzw"] += fz * tc
        a["tf"] += int(m.get("total_functions") or 0)
        a["mf"] += int(m.get("matched_functions") or 0)
        a["units"] += 1
        started_fzw += fz * tc
        started_code += tc
    return mods, started_fzw, started_code


def churn_weights(cur: dict, base: dict, sizes: dict,
                  mods: dict, modules: dict) -> None:
    """Per-module best-ever churn (code-weighted) for the `Fuzzy Max` column:
    sum (best - cur) * bytes over functions now below their peak."""
    for a in mods.values():
        a["cw"] = 0.0
    for (unit, fn), pct in cur.items():
        b = base.get((unit, fn))
        if not b:
            continue
        churn = b["best"] - pct
        if churn <= EPS:
            continue
        mod = modules.get(unit, "?")
        if mod in mods:
            mods[mod]["cw"] += churn * sizes.get((unit, fn), 0)


def render_block(overall: dict, mods: dict, started_fzw: float,
                 started_code: int, eng: dict) -> str:
    """The README score block (between the markers)."""
    matched_fn = int(overall.get("matched_functions") or 0)
    started_fn = int(overall.get("total_functions") or 0)
    tot_fn, tot_code = eng["real_fn"], eng["real_code"]

    rows = []
    for mod in sorted(mods, key=lambda k: -mods[k]["tf"]):
        a = mods[mod]
        fz = (a["fzw"] / a["tc"] if a["tc"] else 0.0)
        fzmax = fz + (a.get("cw", 0.0) / a["tc"] if a["tc"] else 0.0)
        rows.append([f"`{mod}`", f"{a['units']}",
                     f"{a['mf']:,} / {a['tf']:,} ({_pct(a['mf'], a['tf']):.1f}%)",
                     f"{fz:.1f}%", f"{fzmax:.1f}%"])
    if eng["unmatched_fn"]:
        rows.append(["`(unmatched)`", "—",
                     f"0 / {eng['unmatched_fn']:,} (0.0%)", "0.0%", "0.0%"])
    table = _md_table(["Module", "Units", "Functions exact", "Fuzzy",
                       "Fuzzy Max"], "lrrrr", rows)

    ex_rows = [[f"`{label}`", f"{fn:,}", f"{code:,}", note]
               for (label, fn, code, note) in eng["categories"]]
    excl = _md_table(["Category", "Functions", "Code (B)", "Why excluded"],
                     "lrrl", ex_rows)

    overall_fuzzy = started_fzw / tot_code if tot_code else 0.0
    started_fuzzy = started_fzw / started_code if started_code else 0.0
    overall_cw = sum(a.get("cw", 0.0) for a in mods.values())
    overall_fuzzy_max = overall_fuzzy + (overall_cw / tot_code if tot_code else 0.0)

    md_pct = measure(overall, "matched_data_percent")
    td = int(overall.get("total_data") or 0)
    if md_pct is not None and td:
        data_line = (
            f"_Data: objdiff `matched_data` {int(overall.get('matched_data') or 0):,} "
            f"of {td:,} B ({md_pct:.2f}%) - a per-unit sum (a shared COMDAT "
            "counts once per emitting unit), history, never a headline. The "
            "coverage/fidelity partition is the data-audit slice's product and "
            "is not re-derived here._")
    else:
        data_line = "_Data: not reported (no data measures in this report)._"

    block = [
        RM_START,
        "## Match status",
        "",
        f"_Auto-generated by `{NEW_SIGNATURE}`; do not hand-edit. Diff this "
        "block across commits to spot regressions._",
        "",
        f"**Overall (vs full engine): {matched_fn:,} / {tot_fn:,} functions "
        f"exact ({_pct(matched_fn, tot_fn):.2f}%) &middot; "
        f"{overall_fuzzy:.2f}% fuzzy &middot; {overall_fuzzy_max:.2f}% "
        f"fuzzy max.**",
        "",
        data_line,
        "",
        "_Totals are vs the whole engine = every in-`.text` reconstruction-"
        "target function; the generated/library categories tabled below are "
        "excluded from the denominator. `Fuzzy` = code-weighted partial "
        "credit; `Fuzzy Max` = the same with every function at its banked "
        "best-ever fuzzy% - a gap above `Fuzzy` is entropy churn since the "
        "last bank._",
        "",
        f"_Started units alone: {matched_fn:,}/{started_fn:,} fns exact, "
        f"{started_fuzzy:.2f}% fuzzy over {started_code:,} of {tot_code:,} "
        f"engine code bytes._",
        "",
        *table,
        "",
        "_Excluded from the % above — generated/library code, not independent "
        "reconstruction targets:_",
        "",
        *excl,
        RM_END,
    ]
    return "\n".join(block)


def current_block() -> str | None:
    text = README.read_text()
    if RM_START in text and RM_END in text:
        return text[text.index(RM_START): text.index(RM_END) + len(RM_END)]
    return None


def write_block(block: str) -> bool:
    """Replace the marked block; True when the README actually changed."""
    text = README.read_text()
    if RM_START in text and RM_END in text:
        pre = text[: text.index(RM_START)]
        post = text[text.index(RM_END) + len(RM_END):]
        new = pre + block + post
    else:
        i = text.index("\n## ")
        new = text[:i] + "\n" + block + "\n" + text[i:]
    if new == text:
        return False
    README.write_text(new)
    return True


def old_block_numbers(block: str) -> dict | None:
    """Pull the headline numbers out of an OLD-pipeline block for the
    reconciliation. None when the block is not the old pipeline's."""
    if OLD_SIGNATURE not in block:
        return None
    out: dict = {}
    m = re.search(r"\*\*Overall \(vs ([^)]+)\): ([\d,]+) / ([\d,]+) functions "
                  r"exact \(([\d.]+)%\) &middot; ([\d.]+)% fuzzy "
                  r"&middot; ([\d.]+)% fuzzy max", block)
    if m:
        out.update(scope=m.group(1),
                   exact=int(m.group(2).replace(",", "")),
                   engine_fn=int(m.group(3).replace(",", "")),
                   exact_pct=float(m.group(4)), fuzzy=float(m.group(5)),
                   fuzzy_max=float(m.group(6)))
    m = re.search(r"Started units alone: ([\d,]+)/([\d,]+) fns exact, "
                  r"([\d.]+)% fuzzy over ([\d,]+) of ([\d,]+)", block)
    if m:
        out.update(started_fn=int(m.group(2).replace(",", "")),
                   started_fuzzy=float(m.group(3)),
                   started_code=int(m.group(4).replace(",", "")),
                   engine_code=int(m.group(5).replace(",", "")))
    m = re.search(r"`matched_data` \(([\d.]+)%\)", block)
    if m:
        out["matched_data_pct"] = float(m.group(1))
    for cat in ("EH unwind funclets", "private lifecycle/cleanup helpers",
                "CRT/MFC library", "jump thunks"):
        m = re.search(re.escape(f"`{cat}`") + r"\s*\|\s*([\d,]+)\s*\|\s*([\d,]+)",
                      block)
        if m:
            out[f"cat:{cat}"] = (int(m.group(1).replace(",", "")),
                                 int(m.group(2).replace(",", "")))
    return out
