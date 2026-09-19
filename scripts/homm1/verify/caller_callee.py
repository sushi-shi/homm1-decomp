"""homm1.verify.caller_callee - retail<->reconstruction call-graph
reconciliation (full tier).

Every retail direct call/jmp edge caller->callee where BOTH ends are
reconstructed functions must be reproduced by our source as a call resolving
to the same callee rva (clang IR over every TU; ILT thunks followed on the
retail side). The unreconciled count is the metric; each edge's CAUSE is the
worklist:

  FAKE-VIEW   our source reaches the callee through a view whose method
              mangles to a name resolving to NO rva, even after following
              source-defined inline forwarding bodies - retype the receiver
              and the edge reconciles. THE drive-to-0 slice (the board's
              `caller-callee FAKE-VIEW` floor ratchets it; the other causes
              are dominated by static-analysis false positives - inline
              member ops, indirect/PMF/virtual dispatch - and are reported,
              not gated).
  MISSING[-SPECIAL] / UNANALYZED   reported for navigation.

    python3 -m homm1.verify.caller_callee [--metric] [--worklist]
    python3 -m homm1.verify.caller_callee --check 0xb5460
"""

from __future__ import annotations

import concurrent.futures
import os
import re
import struct
import sys

from homm1.core.paths import REPO

_SPECIAL = {"ctor", "dtor", "vec-dtor", "scalar-dtor", "op"}

_DEF_RE = re.compile(r'^define\b[^@\n]*@("(?:[^"\\]|\\.)*"|[-\w.$?@]+)\s*\(',
                     re.M)
_CALL_RE = re.compile(r"^\s+(?:%\S+\s*=\s*)?(?:tail |musttail |notail )?"
                      r"(?:call|invoke)\b")
_CALL_TGT_RE = re.compile(r'@("(?:[^"\\]|\\.)*"|[-\w.$?@]+)\s*(?:\(|to\b)')


def parse_mangled(sym: str):
    """(class|None, member-token, kind) for a mangled fn name, else None.
    Special members map to generic tokens (ctor/dtor/...): their names carry
    no member identity, so a view's `??1V@@` cannot be tied by name."""
    if not sym.startswith("?"):
        return None
    body = sym[1:]
    if body.startswith("?"):
        rest = body[1:]
        if not rest:
            return None
        code, rest = (rest[:2], rest[2:]) if rest[0] == "_" else \
            (rest[0], rest[1:])
        member = {"0": "ctor", "1": "dtor", "_E": "vec-dtor",
                  "_G": "scalar-dtor"}.get(code, "op")
        quals = rest.split("@@", 1)[0].split("@")
        cls = quals[0] if quals and quals[0] else None
        return cls, member, "special"
    i = body.find("@")
    if i < 0:
        return None
    member = body[:i]
    rest = body[i:]
    j = rest.find("@@")
    if j < 0:
        return None
    quals = [q for q in rest[:j].split("@") if q]
    return (quals[0] if quals else None), member, "method"


def _ir_name(tok: str) -> str:
    if tok.startswith('"') and tok.endswith('"'):
        tok = tok[1:-1]
    if tok.startswith("\\01"):
        tok = tok[3:]
    return tok


def _tu_edges(tu_flags):
    from homm1.tool.clang import emit_ir
    tu, flags = tu_flags
    text = emit_ir(tu, flags)
    if not text:
        return tu, None
    out: dict[str, set] = {}
    cur = None
    for line in text.splitlines():
        dm = _DEF_RE.match(line)
        if dm:
            cur = _ir_name(dm.group(1))
            out.setdefault(cur, set())
            continue
        if cur is None:
            continue
        if line.startswith("}"):
            cur = None
            continue
        if _CALL_RE.match(line):
            for tok in _CALL_TGT_RE.findall(line):
                out[cur].add(_ir_name(tok))
    return tu, out


def _resolve_source_calls(sym: str, graph: dict[str, set], m2rva: dict[str, int]):
    """Resolve one IR callee through source-only helper definitions.

    Header-inline forwarding members have no retail RVA of their own when the
    optimizer expands them.  Clang's unoptimized IR nevertheless emits a call
    to the forwarding member plus a linkonce definition for its body.  Follow
    that body until reaching reconstructed retail functions; otherwise the
    member's class is falsely reported as a fake receiver view.
    """
    resolved: set[int] = set()
    leaves: set[str] = set()
    pending = [sym]
    seen: set[str] = set()
    while pending:
        cur = pending.pop()
        if cur in seen:
            continue
        seen.add(cur)
        rva = m2rva.get(cur)
        if rva is not None:
            resolved.add(rva)
            continue
        callees = graph.get(cur)
        if not callees:
            leaves.add(cur)
            continue
        pending.extend(callees)
    if not resolved and not leaves:
        leaves.add(sym)
    return resolved, leaves


class Recon:
    def __init__(self, jobs=None):
        from homm1.model import resolve
        from homm1.sema.image import retail
        self.img = retail()
        m = resolve()
        src = {b.rva: (b.name, b.unit) for b in m.functions
               if b.name and b.channel.startswith("src")}
        self.rva2sym = {rva: (nm, unit, *(parse_mangled(nm) or
                                          (None, None, None))[:2])
                        for rva, (nm, unit) in src.items()}
        self.m2rva = {}
        for b in m.functions:
            if b.name:
                self.m2rva.setdefault(b.name, b.rva)
        self.func_rvas = set(src)
        self.fn_starts = sorted(b.rva for b in m.functions)
        self.fn_size = {b.rva: b.size for b in m.functions}
        self.thunk_rvas = {b.rva for b in m.functions if b.kind == "thunk"}
        self.tgt, self.allcallees = self._target_edges()
        self.base, self.base_defined, self.unresolved, self.ir_failed = \
            self._base_graph(jobs)

    # --- retail graph -------------------------------------------------------
    def _chase(self, rva, depth=0):
        while depth < 4 and rva in self.thunk_rvas:
            nxt = self.img.jmp_target(rva)
            if nxt is None:
                break
            rva = nxt
            depth += 1
        return rva

    def _owner(self, site):
        import bisect
        k = bisect.bisect_right(self.fn_starts, site) - 1
        if k < 0:
            return None
        start = self.fn_starts[k]
        sz = self.fn_size.get(start)
        if sz and site >= start + sz:
            return None
        return start

    def _target_edges(self):
        t = self.img.pe.section(".text")
        tva = t["va"]
        tb = self.img.pe.data[t["rptr"]:t["rptr"] + t["rsize"]]
        edges, allc = set(), {}
        n = len(tb) - 4
        i = 0
        while i < n:
            op = tb[i]
            if op == 0xE8 or op == 0xE9:
                site = tva + i
                rel = struct.unpack_from("<i", tb, i + 1)[0]
                tgt = site + 5 + rel
                if self.img.text_lo <= tgt < self.img.text_hi:
                    body = self._chase(tgt)
                    o = self._owner(site)
                    if o is not None and o != body and o in self.func_rvas \
                            and o not in self.thunk_rvas:
                        allc.setdefault(o, set()).add(body)
                        if body in self.func_rvas:
                            edges.add((o, body))
            i += 1
        return edges, allc

    # --- base graph (clang IR) ----------------------------------------------
    def _base_graph(self, jobs=None):
        from homm1.tool.clang import compdb
        db = compdb()
        tus = sorted(k for k in db
                     if "/src/" in k.replace("\\", "/") and k.endswith(".cpp"))
        work = [(tu, db[tu]) for tu in tus]
        graph: dict[str, set] = {}
        failed = []
        jobs = jobs or min(24, (os.cpu_count() or 4) * 2)
        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as ex:
            for tu, res in ex.map(_tu_edges, work):
                if res is None:
                    failed.append(os.path.relpath(tu, REPO))
                    continue
                for caller, callees in res.items():
                    graph.setdefault(caller, set()).update(callees)

        edges, defined, unresolved = set(), set(), {}
        for caller, callees in graph.items():
            cr = self.m2rva.get(caller)
            if cr is None:
                continue
            defined.add(cr)
            for callee in callees:
                rvas, leaves = _resolve_source_calls(callee, graph,
                                                     self.m2rva)
                edges.update((cr, ce) for ce in rvas if ce != cr)
                if leaves:
                    unresolved.setdefault(cr, set()).update(leaves)
        return edges, defined, unresolved, failed

    # --- reconcile ----------------------------------------------------------
    def name(self, rva):
        s = self.rva2sym.get(rva)
        return s[0] if s else f"FUN_{rva:x}"

    def unit(self, rva):
        s = self.rva2sym.get(rva)
        return s[1] if s else "?"

    def classify(self, caller, callee):
        if caller not in self.base_defined:
            return ("UNANALYZED", None)
        rmember = self.rva2sym.get(callee, (None, None, None, None))[3]
        if rmember in _SPECIAL or not rmember:
            return ("MISSING-SPECIAL" if rmember in _SPECIAL
                    else "MISSING", None)
        views = []
        for mangled in self.unresolved.get(caller, ()):
            pm = parse_mangled(mangled)
            if not pm:
                continue
            vcls, vmember, _k = pm
            if vmember == rmember and vmember not in _SPECIAL and vcls:
                views.append(vcls)
        if views:
            uniq = sorted(set(views))
            return ("FAKE-VIEW",
                    uniq[0] if len(uniq) == 1 else "|".join(uniq))
        return ("MISSING", None)

    def missing(self):
        out = []
        for (caller, callee) in self.tgt:
            if (caller, callee) in self.base:
                continue
            cause, view = self.classify(caller, callee)
            out.append((caller, callee, cause, view))
        return out


_METRIC_CACHE: tuple | None = None


def _summary():
    global _METRIC_CACHE
    if _METRIC_CACHE is None:
        rc = Recon()
        miss = rc.missing()
        causes: dict[str, int] = {}
        for _f, _r, c, _v in miss:
            causes[c] = causes.get(c, 0) + 1
        _METRIC_CACHE = (rc, miss, causes)
    return _METRIC_CACHE


def fake_view_count() -> int:
    _rc, _miss, causes = _summary()
    return causes.get("FAKE-VIEW", 0)


def gate_findings() -> list[str]:
    """The gate: FAKE-VIEW edges above the board's committed floor."""
    from homm1.verify.board import load_baseline
    rc, miss, causes = _summary()
    if not rc.tgt:
        # No retail edges means the call graph never loaded; 0 FAKE-VIEW then
        # says nothing about the tree and must not read as a clean bill.
        return ["caller-callee: 0 retail reconstructed<->reconstructed call "
                "edges - the graph did not load (unbuilt tree, or a broken "
                "report/Model join), so 0 FAKE-VIEW is vacuous, not clean. "
                "Run `homm1 build` and re-run."]
    n = causes.get("FAKE-VIEW", 0)
    floor = load_baseline().get("caller-callee FAKE-VIEW", 0)
    if n <= floor:
        return []
    out = [f"caller-callee: FAKE-VIEW {n} exceeds the committed floor "
           f"{floor}"]
    for caller, callee, cause, view in sorted(miss):
        if cause == "FAKE-VIEW":
            out.append(f"  0x{caller:06x} {rc.name(caller)} "
                       f"[{rc.unit(caller)}] -> 0x{callee:06x} "
                       f"{rc.name(callee)} via view `{view}`")
    return out


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 verify caller-callee",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--metric", action="store_true",
                    help="print the unreconciled count and stop")
    ap.add_argument("--worklist", action="store_true",
                    help="list every unreconciled edge with its cause")
    ap.add_argument("--check", help="explain one caller (rva or name)")
    ap.add_argument("--jobs", type=int, default=None,
                    help="parallel TU IR jobs (default: one per core)")
    a = ap.parse_args(argv)

    rc = Recon(a.jobs)
    if a.check:
        try:
            caller = int(a.check, 16)
        except ValueError:
            caller = rc.m2rva.get(a.check)
            if caller is None:
                sys.exit(f"unknown {a.check}")
        print(f"caller 0x{caller:08x} {rc.name(caller)} [{rc.unit(caller)}] "
              f"analysed={caller in rc.base_defined}")
        for callee in sorted(r for (f, r) in rc.tgt if f == caller):
            ok = (caller, callee) in rc.base
            print(f"    [{'OK' if ok else 'MISSING':7s}] 0x{callee:08x} "
                  f"{rc.name(callee)}")
        for m in sorted(rc.unresolved.get(caller, set()))[:30]:
            print(f"    emitted-unresolved: {m}")
        return 0

    miss = rc.missing()
    causes: dict[str, int] = {}
    for _f, _r, c, _v in miss:
        causes[c] = causes.get(c, 0) + 1
    print(f"[caller-callee] retail reconstructed<->reconstructed edges: "
          f"{len(rc.tgt)}")
    print(f"  reconciled (emitted by our IR): {len(rc.tgt) - len(miss)}")
    print(f"  UNRECONCILED (metric, drive to 0): {len(miss)}")
    for c in sorted(causes, key=lambda k: -causes[k]):
        print(f"      {causes[c]:5d}  {c}")
    print(f"  {causes.get('FAKE-VIEW', 0)} FAKE-VIEW (the gated slice)")
    print(f"  base IR: {len(rc.base_defined)} callers analysed, "
          f"{len(rc.base)} edges, {len(rc.ir_failed)} TU IR failures")
    if a.metric:
        return 0
    if a.worklist:
        for caller, callee, cause, view in sorted(miss):
            if cause != "FAKE-VIEW":
                continue
            print(f"  0x{caller:08x} {rc.name(caller)} [{rc.unit(caller)}] "
                  f"-> 0x{callee:08x} {rc.name(callee)}  via `{view}`")
        return 0
    ranked = sorted(miss, key=lambda t: (t[2] != "FAKE-VIEW", t[0]))
    for caller, callee, cause, view in ranked[:40]:
        vs = f" via `{view}`" if view else ""
        print(f"  {cause:15s} 0x{caller:08x} {rc.name(caller)} "
              f"[{rc.unit(caller)}] -> 0x{callee:08x} {rc.name(callee)}{vs}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
