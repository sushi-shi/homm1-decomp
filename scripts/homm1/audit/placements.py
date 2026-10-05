"""Place game identities in another image by retail byte correspondence.

    homm1 --image editor audit placements                 # report
    homm1 --image editor audit placements --write-config  # write the tables

The game's resolved bindings (build/gen/bindings.tsv: names, units, exact
extents) are carried into the selected image only where the selected image's
retail bytes prove the same entity:

  functions  The game body (its claimed extent, trailing NOP/INT3 fill
             trimmed) with every absolute field of the reviewed game manifest
             and every rel32 branch/call operand masked must occur in the
             image's .text at a census start. A body that occurs more than
             once is placed only at the occurrence its unit's uniquely placed
             bodies predict (one contiguous contribution moves as a block).
  data       Each absolute field of a placed body pairs the game target with
             the image's target at the same body offset. A named game datum
             is placed when every pair that lands inside its extent names the
             same image address (code users, never address arithmetic).
  thunks     Import thunks are paired by their IAT import, not by bytes.

Every placement is keyed by (image, rva) and joined to the game by
(game rva, name); nothing is joined by address alone. Outputs under the
image's config/retail directory:

  placements.tsv         source claims of units the image shares with the game
                         (they translate the shared source's game-space claims)
  function_referents.tsv names of placed functions and paired import thunks
  data_vtables.tsv, data_compgen.tsv   placed provider rows

Units are shared when config/units.toml lists the image in their `images`.
"""

from __future__ import annotations

import argparse
import re
import struct
from collections import defaultdict
from pathlib import Path

from homm1.core.paths import BUILD, REPO, RETAIL_ROOT, image_key, retail_dir, retail_exe
from homm1.core.pe import Pe
from homm1.core.tsv import read as read_tsv, write as write_tsv

FILL = (0x90, 0xCC)
#: Literal-pool names derived from the game rva (data_manifest, msvc_names):
#: they never carry to another image, whose literal pool names its own slots.
RVA_NAMED = re.compile(r"^\$(?:SG|T)[0-9]+$")
SRC_CHANNELS = ("src", "src_compgen", "src_dyninit", "src_data_compgen")


def _text(pe: Pe) -> tuple[bytes, int]:
    t = pe.section(".text")
    return pe.data[t["rptr"]:t["rptr"] + t["vsize"]], t["va"]


def _sites(manifest: Path) -> list[int]:
    return sorted(int(line.split("\t")[0], 16) for line in manifest.read_text().splitlines()
                  if line.startswith("0x"))


def _rel32_sites(body: bytes, rva: int) -> list[int]:
    """Body offsets of rel32 branch/call operands (E8/E9/0F 8x)."""
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    out = []
    for insn in md.disasm(body, 0x400000 + rva):
        raw = bytes(insn.bytes)
        if raw[0] in (0xE8, 0xE9) and len(raw) == 5:
            out.append(insn.address - 0x400000 - rva + 1)
        elif len(raw) == 6 and raw[0] == 0x0F and 0x80 <= raw[1] <= 0x8F:
            out.append(insn.address - 0x400000 - rva + 2)
    return out


def _pattern(body: bytes, masked: list[int]) -> re.Pattern:
    mask = bytearray(len(body))
    for site in masked:
        for i in range(site, min(len(body), site + 4)):
            if i >= 0:
                mask[i] = 1
    parts, i = [], 0
    while i < len(body):
        j = i
        while j < len(body) and mask[j] == mask[i]:
            j += 1
        parts.append(b".{%d}" % (j - i) if mask[i] else re.escape(body[i:j]))
        i = j
    return re.compile(b"".join(parts), re.S)


class Placer:
    def __init__(self, image: str):
        from homm1.manifest import all_units, unit_images
        self.image = image
        self.game = Pe(retail_exe("game"))
        self.pe = Pe(retail_exe(image))
        self.gtext, self.gva = _text(self.game)
        self.etext, self.eva = _text(self.pe)
        self.gsites = _sites(RETAIL_ROOT / "absolute_relocations.tsv")
        self.esites = set(_sites(retail_dir(image) / "absolute_relocations.tsv"))
        self.starts = {int(r["rva"], 16) for r in
                       read_tsv(retail_dir(image) / "functions.tsv")[2]}
        self.shared = {u["unit"] for u in all_units() if image in unit_images(u)}
        _b, _h, rows = read_tsv(BUILD / "gen/bindings.tsv")
        self.bindings = [dict(r, rva=int(r["rva"], 16), size=int(r["size"], 16))
                         for r in rows]
        self.functions: dict[int, tuple[int, str]] = {}   # game rva -> (rva, evidence)
        self.data: dict[int, tuple[int, str]] = {}
        self.problems: list[str] = []

    # -- functions ----------------------------------------------------------
    def body(self, rva: int, size: int) -> tuple[bytes, list[int]]:
        raw = self.gtext[rva - self.gva:rva - self.gva + size]
        end = len(raw)
        while end > 1 and raw[end - 1] in FILL:
            end -= 1
        raw = raw[:end]
        import bisect
        i, j = bisect.bisect_left(self.gsites, rva), bisect.bisect_left(self.gsites, rva + end)
        masked = [s - rva for s in self.gsites[i:j]] + _rel32_sites(raw, rva)
        return raw, sorted(set(masked))

    def place_functions(self) -> None:
        candidates: dict[int, list[int]] = {}
        meta = {}
        for b in self.bindings:
            if b["space"] != "text" or not b["name"] or b["kind"] in ("thunk", "pad", "eh"):
                continue
            raw, masked = self.body(b["rva"], b["size"])
            if not raw:
                continue
            hits = [self.eva + m.start() for m in _pattern(raw, masked).finditer(self.etext)]
            hits = [h for h in hits if h in self.starts]
            if hits:
                candidates[b["rva"]] = hits
                meta[b["rva"]] = (b, len(raw), len(masked))
        # unique placements first, then disambiguate by the unit's block delta
        from collections import Counter
        deltas: dict[str, Counter] = defaultdict(Counter)
        self.deltas = deltas
        for grva, hits in candidates.items():
            if len(hits) == 1 and meta[grva][1] >= 16:
                b, n, m = meta[grva]
                self.functions[grva] = (hits[0], f"masked game body 0x{grva:x} ({n} bytes, "
                                                 f"{m} fields masked) unique at a census start")
                if b["unit"]:
                    deltas[b["unit"]][hits[0] - grva] += 1
        # A unit's contribution moves as a block: a delta at which two or
        # more of its bodies occur places every body found at that delta.
        by_unit: dict[str, Counter] = defaultdict(Counter)
        for grva, hits in candidates.items():
            unit = meta[grva][0]["unit"]
            if unit:
                for h in set(hits):
                    by_unit[unit][h - grva] += 1
        for grva, hits in sorted(candidates.items()):
            if grva in self.functions:
                continue
            b, n, m = meta[grva]
            block = by_unit.get(b["unit"], Counter())
            ranked = sorted(((block.get(h - grva, 0), h) for h in set(hits)), reverse=True)
            best = ranked[0] if ranked else (0, None)
            runner = ranked[1][0] if len(ranked) > 1 else 0
            predicted = [best[1]] if best[0] >= 2 and best[0] > runner else []
            if len(predicted) == 1:
                self.functions[grva] = (predicted[0], f"masked game body 0x{grva:x} ({n} bytes) "
                                        f"at its unit's block offset, shared by "
                                        f"{block[predicted[0] - grva]} bodies "
                                        f"({len(hits)} identical occurrence(s))")

    def drop_collisions(self) -> None:
        """Two game bodies proven at one image address are byte-identical
        (fread/fwrite wrappers): the bytes cannot name either."""
        by_image = defaultdict(list)
        for grva, (erva, _why) in self.functions.items():
            by_image[erva].append(grva)
        from collections import Counter
        unit = {b["rva"]: b["unit"] for b in self.bindings}
        support: dict[str, Counter] = defaultdict(Counter)
        for grva, (erva, _why) in self.functions.items():
            if unit.get(grva):
                support[unit[grva]][erva - grva] += 1
        for erva, grvas in by_image.items():
            # supported by another placed body of the same unit at that delta
            predicted = [g for g in grvas
                         if support.get(unit.get(g), {}).get(erva - g, 0) >= 2]
            if len(grvas) > 1 and len(predicted) == 1:
                for grva in grvas:
                    if grva != predicted[0]:
                        del self.functions[grva]
                continue
            if len(grvas) > 1:
                self.problems.append(f"0x{erva:x}: {len(grvas)} game bodies are byte-identical "
                                     f"({', '.join(hex(g) for g in sorted(grvas))}); unnamed")
                for grva in grvas:
                    del self.functions[grva]

    def check_calls(self) -> None:
        """A placed body's rel32 calls must reach the placements of the game
        callees wherever those are placed."""
        from capstone import CS_ARCH_X86, CS_MODE_32, Cs
        md = Cs(CS_ARCH_X86, CS_MODE_32)
        size = {b["rva"]: b["size"] for b in self.bindings if b["space"] == "text"}
        bad = []
        for grva, (erva, _why) in sorted(self.functions.items()):
            raw = self.gtext[grva - self.gva:grva - self.gva + size.get(grva, 0)]
            for insn in md.disasm(raw, 0x400000 + grva):
                if insn.mnemonic != "call" or insn.bytes[0] != 0xE8:
                    continue
                gt = insn.address + 5 + struct.unpack_from("<i", bytes(insn.bytes), 1)[0] - 0x400000
                if gt not in self.functions:
                    continue
                off = insn.address - 0x400000 - grva
                esite = erva + off + 1
                et = esite + 4 + struct.unpack_from("<i", self.etext, esite - self.eva)[0]
                if et != self.functions[gt][0]:
                    bad.append(grva)
                    break
        for grva in bad:
            self.problems.append(f"0x{grva:x}: placed body calls disagree with callee placements")
            del self.functions[grva]

    def callee_names(self) -> dict[int, tuple[str, str]]:
        """{image rva: (name, evidence)} for unplaced callees of placed
        shared-unit bodies: one source call site names one callee, so the
        game callee's name carries to the image's target of the same site."""
        from capstone import CS_ARCH_X86, CS_MODE_32, Cs
        md = Cs(CS_ARCH_X86, CS_MODE_32)
        named = {b["rva"]: b["name"] for b in self.bindings
                 if b["space"] == "text" and b["name"]}
        size = {b["rva"]: b["size"] for b in self.bindings if b["space"] == "text"}
        unit = {b["rva"]: b["unit"] for b in self.bindings}
        placed = {erva for erva, _w in self.functions.values()}
        out: dict[int, tuple[str, str]] = {}
        conflicts: set[int] = set()
        for grva, (erva, _why) in sorted(self.functions.items()):
            if unit.get(grva) not in self.shared:
                continue
            raw = self.gtext[grva - self.gva:grva - self.gva + size.get(grva, 0)]
            for insn in md.disasm(raw, 0x400000 + grva):
                if insn.mnemonic != "call" or insn.bytes[0] != 0xE8:
                    continue
                gt = insn.address + 5 + struct.unpack_from("<i", bytes(insn.bytes), 1)[0] - 0x400000
                if gt not in named or gt in self.functions:
                    continue
                esite = erva + insn.address - 0x400000 - grva + 1
                et = esite + 4 + struct.unpack_from("<i", self.etext, esite - self.eva)[0]
                if et in placed or et not in self.starts:
                    continue
                name = named[gt]
                if et in out and out[et][0] != name:
                    conflicts.add(et)
                out.setdefault(et, (name, f"callee of placed {unit[grva]} body at site "
                                          f"0x{esite - 1:x}; game callee 0x{gt:x}"))
        for et in conflicts:
            self.problems.append(f"0x{et:x}: call sites name different callees; unnamed")
            del out[et]
        return out

    # -- data ---------------------------------------------------------------
    def place_data(self) -> None:
        import bisect
        pairs: list[tuple[int, int]] = []          # (game target, image target)
        size = {b["rva"]: b["size"] for b in self.bindings if b["space"] == "text"}
        for grva, (erva, _why) in self.functions.items():
            i = bisect.bisect_left(self.gsites, grva)
            j = bisect.bisect_left(self.gsites, grva + size.get(grva, 0))
            for site in self.gsites[i:j]:
                esite = erva + site - grva
                if esite not in self.esites:
                    continue
                gt = struct.unpack("<I", self.game.read(site, 4))[0] - 0x400000
                et = struct.unpack("<I", self.pe.read(esite, 4))[0] - 0x400000
                pairs.append((gt, et))
        pairs.sort()
        self.pairs, self.keys = pairs, [g for g, _e in pairs]
        for b in self.bindings:
            if b["space"] == "text" or not b["name"]:
                continue
            placed = self.place_datum(b["rva"], b["size"], b["name"], b["space"])
            if placed is not None:
                self.data[b["rva"]] = placed
        # every game census data start (extent to the next start in its
        # section) by its code users
        game_rows = sorted((int(r["rva"], 16), r.get("kind", ""))
                           for r in read_tsv(RETAIL_ROOT / "data.tsv")[2])
        sections = [(x["va"], x["va"] + max(x["vsize"], x["rsize"]), x["name"])
                    for x in self.game.sections]

        def section_end(rva):
            return next((hi for lo, hi, _n in sections if lo <= rva < hi), rva + 1)
        rows = []
        for (rva, kind), nxt in zip(game_rows, game_rows[1:] + [(None, None)]):
            end = section_end(rva)
            if nxt[0] is not None and nxt[0] < end:
                end = nxt[0]
            rows.append({"rva": rva, "size": end - rva, "kind": kind, "region": "data"})
        self.census_rows: dict[int, tuple[int, str]] = {}
        for r in rows:
            # an unnamed start's extent is only "to the next start": only a
            # reference to the start itself places it
            placed = self.place_datum(r["rva"], 1, f"census 0x{r['rva']:x}", "bss",
                                      quiet=True)
            if placed is not None:
                self.census_rows[r["rva"]] = (placed[0], r["kind"])
        claimed_extents = [(b["rva"], b["size"]) for b in self.bindings
                           if b["space"] != "text" and b["name"] and b["channel"]]
        # reviewed data names (data_symbols.tsv) need no binding of their own
        self.symbols: dict[int, tuple[int, str, str, int]] = {}
        for r in read_tsv(RETAIL_ROOT / "data_symbols.tsv")[2]:
            grva, size = int(r["rva"], 16), int(r["size"], 0)
            if RVA_NAMED.match(r["symbol"]):
                continue        # `$SG<rva>`/`$T<rva>` spell the game address
            if any(lo < grva < lo + size_ for lo, size_ in claimed_extents):
                self.problems.append(f"game data_symbols {r['symbol']} at 0x{grva:x} lies "
                                     "inside a claimed game datum; not placed")
                continue
            placed = self.place_datum(grva, size, r["symbol"], "data")
            if placed is not None:
                self.symbols[grva] = (placed[0], r["symbol"], placed[1], size)

    def place_datum(self, lo: int, size: int, name: str, space: str, quiet: bool = False):
        """(image rva, evidence) for a game datum, or None."""
        import bisect
        hi = lo + max(size, 1)
        pairs, keys = self.pairs, self.keys
        i, j = bisect.bisect_left(keys, lo), bisect.bisect_left(keys, hi)
        found = {e - (g - lo) for g, e in pairs[i:j]}
        if len(found) == 1:
            return found.pop(), f"{j - i} code reference(s) from placed bodies"
        if len(found) > 1:
            if not quiet:
                self.problems.append(f"data {name} at 0x{lo:x}: code users disagree "
                                     f"({sorted(hex(x) for x in found)[:4]})")
            return None
        if space not in ("data", "rdata") or size < 6:
            return None
        # No code user: the datum's complete initialized bytes, free of
        # absolute fields, must occur exactly once in the image.
        raw = self.game.read(lo, size)
        k = bisect.bisect_left(self.gsites, lo)
        if raw is None or sum(1 for c in raw if c) < 4 or (
                k < len(self.gsites) and self.gsites[k] < hi):
            return None
        hits = []
        for s in self.pe.sections:
            if s["name"] in (".rdata", ".data") and s["rsize"]:
                blob = self.pe.data[s["rptr"]:s["rptr"] + s["rsize"]]
                at = blob.find(raw)
                while at != -1:
                    hits.append(s["va"] + at)
                    at = blob.find(raw, at + 1)
        if len(hits) == 1:
            return hits[0], f"complete initialized bytes ({size}) unique in the image; no code user"
        return None

    # -- thunks ---------------------------------------------------------------
    def thunks(self) -> list[tuple[int, str, str]]:
        import json
        from homm1.delink.image import Image
        census = json.loads((retail_dir(self.image) / "census.json").read_text())
        image_thunks = {(t["dll"].upper(), t["import_key"]): int(t["rva"], 16)
                        for t in census["import_thunks"]}
        slots = {slot: (dll.upper(), name or f"ordinal_{ordinal}")
                 for slot, name, dll, ordinal in Image(self.game).import_slots()}
        out = []
        for b in self.bindings:
            if b["kind"] != "thunk" or not b["name"]:
                continue
            raw = self.game.read(b["rva"], 6)
            if raw[:2] != b"\xff\x25":
                continue
            key = slots.get(struct.unpack_from("<I", raw, 2)[0] - 0x400000)
            if key in image_thunks:
                out.append((image_thunks[key], b["name"],
                            f"FF25 thunk of {key[0]}!{key[1]}; name of game thunk 0x{b['rva']:x}"))
        return out

    def run(self) -> None:
        self.place_functions()
        self.drop_collisions()
        self.check_calls()
        self.place_data()


def write_tables(p: Placer) -> dict:
    out = retail_dir(p.image)
    digest_line = f"# Generated by `homm1 --image {p.image} audit placements --write-config`."
    by_rva = {b["rva"]: b for b in p.bindings}
    placements, referents, vtables, compgen = [], [], [], []
    for grva, (erva, why) in sorted(p.functions.items(), key=lambda kv: kv[1][0]):
        b = by_rva[grva]
        if b["channel"] in SRC_CHANNELS and b["unit"] in p.shared:
            placements.append([f"0x{erva:08x}", f"0x{b['size']:x}", "func", b["name"],
                               b["unit"], b["channel"], f"0x{grva:08x}", why])
        else:
            referents.append([f"0x{erva:08x}", b["name"], f"game 0x{grva:x}: {why}"])
    for erva, name, why in p.thunks():
        referents.append([f"0x{erva:08x}", name, why])
    taken = {int(r[0], 16) for r in referents}
    for erva, (name, why) in sorted(p.callee_names().items()):
        if erva not in taken:
            referents.append([f"0x{erva:08x}", name, why])
    # LIBCMT publics at census starts whose whole member contribution matched
    named = {int(r[0], 16) for r in referents} | {int(r[0], 16) for r in placements}
    import json
    census = json.loads((out / "census.json").read_text())
    for start in census["starts"]:
        rva = int(start["rva"], 16)
        if "library_symbol" in start and rva not in named:
            referents.append([f"0x{rva:08x}", start["library_symbol"],
                              f"{start['evidence'][0]} (public symbol)"])
    for grva, (erva, why) in sorted(p.data.items(), key=lambda kv: kv[1][0]):
        b = by_rva[grva]
        if b["channel"] in SRC_CHANNELS and b["unit"] in p.shared:
            placements.append([f"0x{erva:08x}", f"0x{b['size']:x}", "data", b["name"],
                               b["unit"], b["channel"], f"0x{grva:08x}", why])
        elif b["channel"] == "data_vtables":
            vtables.append([f"0x{erva:08x}", f"0x{b['size']:x}", b["name"], "primary",
                            f"game 0x{grva:x}: {why}"])
        elif b["channel"] == "data_compgen" and b["unit"] in p.shared \
                and not RVA_NAMED.match(b["name"]):
            compgen.append([f"0x{erva:08x}", f"0x{b['size']:x}", b["name"], b["unit"],
                            b["kind"] or "common"])
    write_tsv(out / "data_symbols.tsv", [
        digest_line, "# Reviewed game data names placed by their code users."],
        ["rva", "size", "symbol", "provenance"],
        sorted([f"0x{erva:08x}", str(size), name, f"game 0x{grva:x}: {why}"]
               for grva, (erva, name, why, size) in p.symbols.items())
        + sorted([f"0x{erva:08x}", str(by_rva[grva]["size"]), by_rva[grva]["name"],
                  f"game 0x{grva:x} ({by_rva[grva]['unit'] or 'provider'}): {why}"]
                 for grva, (erva, why) in p.data.items()
                 if by_rva[grva]["channel"] in SRC_CHANNELS
                 and by_rva[grva]["unit"] not in p.shared
                 and erva not in {e for e, *_ in p.symbols.values()}))
    if not (out / "reloc_referents.tsv").is_file():
        write_tsv(out / "reloc_referents.tsv", [
            "# Reviewed relocation referents requiring more than containment (none yet)."],
            ["function_rva", "target_rva", "site_rva", "owner", "addend", "occurrences",
             "provenance"], [])
    # Shared units' code contributions move as blocks: a game link_order row
    # carries over when every placed body inside it has one delta.
    rows = []
    for r in read_tsv(RETAIL_ROOT / "link_order.tsv")[2]:
        if r["unit"] not in p.shared:
            continue
        lo, hi = int(r["lo"], 16), int(r["hi"], 16)
        deltas = {erva - grva for grva, (erva, _w) in p.functions.items() if lo <= grva < hi}
        if len(deltas) == 1:
            d = deltas.pop()
            rows.append((lo + d, hi + d, r["unit"], r["class"]))
    rows.sort()
    write_tsv(out / "link_order.tsv", [
        digest_line,
        "# Shared units' contributions: game link_order rows moved by their placed bodies' delta.",
        "# Exact body spans only; padding gaps are not assigned."],
        ["index", "unit", "lo", "hi", "class"],
        [[str(i), u, f"0x{lo:08x}", f"0x{hi:08x}", c] for i, (lo, hi, u, c) in enumerate(rows)])
    # The data census: a start for every placed datum, with its game kind.
    game_kinds = {int(r["rva"], 16): r.get("kind", "")
                  for r in read_tsv(RETAIL_ROOT / "data.tsv")[2]}
    starts = {erva: kind for _g, (erva, kind) in p.census_rows.items()}
    starts.update({erva: game_kinds.get(grva, "") for grva, (erva, _w) in p.data.items()})
    for grva, (erva, _n, _w, _s) in p.symbols.items():
        starts.setdefault(erva, game_kinds.get(grva, ""))
    data_rows = sorted(starts.items())
    write_tsv(out / "data.tsv", [
        digest_line,
        "# Data starts placed from the game census by their code users (placements.tsv)."],
        ["rva", "kind"], [[f"0x{rva:08x}", kind] for rva, kind in data_rows])
    placements.sort()
    referents.sort()
    write_tsv(out / "placements.tsv", [
        digest_line,
        "# Source claims of shared units, placed by retail byte correspondence;",
        "# game_rva/name join the game binding the shared source spells."],
        ["rva", "size", "kind", "name", "unit", "channel", "game_rva", "evidence"], placements)
    write_tsv(out / "function_referents.tsv", [
        digest_line, "# Names of game functions and import thunks placed in this image."],
        ["rva", "name", "provenance"], referents)
    write_tsv(out / "data_vtables.tsv", [digest_line],
              ["rva", "size", "name", "kind", "note"], vtables)
    write_tsv(out / "data_compgen.tsv", [digest_line],
              ["rva", "size", "name", "owner", "class"], compgen)
    return {"placements": len(placements), "referents": len(referents),
            "vtables": len(vtables), "compgen": len(compgen)}


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="homm1 audit placements", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--write-config", action="store_true")
    a = ap.parse_args(argv)
    image = image_key()
    if image == "game":
        ap.error("select the image to place game identities in (homm1 --image KEY)")
    p = Placer(image)
    p.run()
    shared_funcs = sum(1 for b in p.bindings if b["space"] == "text" and b["unit"] in p.shared
                       and b["channel"] in SRC_CHANNELS)
    placed_shared = sum(1 for g in p.functions if g in {b["rva"] for b in p.bindings
                                                        if b["unit"] in p.shared
                                                        and b["channel"] in SRC_CHANNELS})
    print(f"[placements] {image}: {len(p.functions)} function(s), {len(p.data)} datum/data "
          f"placed; shared-unit source functions {placed_shared}/{shared_funcs}")
    for problem in p.problems[:20]:
        print(f"[placements] {problem}")
    if a.write_config:
        counts = write_tables(p)
        print("[placements] wrote " + ", ".join(f"{k}={v}" for k, v in counts.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
