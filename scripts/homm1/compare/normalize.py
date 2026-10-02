"""Batch-normalize base + target COFF objs into disposable objdiff comparison copies.

    python3 -m homm1.compare.normalize --base-dir B --target-dir T --out-dir O

A driver around `homm1.compare.canonicalize.canonicalize_coff`: for every unit
it is given it rewrites the compiler-private data names (`$SG`/`$T`/`name$S<n>`),
resolves COFF weak externals to their default, names OLDNAMES references by
the runtime function LINK binds them to, and rewrites same-function
jump-table `DIR32` labels of both the recompiled base obj and its delinked
target obj into a content-addressed, side-by-side view under `<out-dir>/`.
`objdiff.json` points at these copies; the real base and target objects are
never touched, so the transform is matching-NEUTRAL (see canonicalize.py and
the sibling homm2 docs/data-symbol-normalization).

Per-object work is skipped when the normalized copy is already newer than its
input and the normalizer modules, so a single-file edit only re-normalizes that
one obj; `force=True` writes unconditionally. A `.symbols.tsv` sidecar is
emitted next to each copy for auditing, and a stamp lists the processed set.

The unit list is an ARGUMENT: callers pass the manifest census. This module
does not read config/units.toml, and it never predicts which units have a
target - `target_object` looks on disk.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path
from dataclasses import dataclass
from homm1.core import data_matching

from homm1.compare import canonicalize as canon
from homm1.compare import runtime_aliases
from homm1.delink import eh_band

_MODULE_MTIME = max(
    Path(canon.__file__).stat().st_mtime,
    Path(runtime_aliases.__file__).stat().st_mtime,
    Path(canon.msvc_names.__file__).stat().st_mtime,
    *([data_matching.COMPARE_TOML.stat().st_mtime]
      if data_matching.COMPARE_TOML.is_file() else []),
    Path(eh_band.__file__).stat().st_mtime,
    Path(__file__).stat().st_mtime,
)
SYMBOL_SIZE = canon.SYMBOL_SIZE
#: The delinker spells its per-unit objects `<unit>.c.obj`; a plain `<unit>.obj`
#: is accepted too, so a target directory written either way still pairs.
TARGET_SUFFIXES = (".c.obj", ".obj")


def target_object(target_dir: Path, unit: str) -> Path | None:
    """The delinked object for `unit`, or None. Read off disk, never predicted."""
    for suffix in TARGET_SUFFIXES:
        path = target_dir / f"{unit}{suffix}"
        if path.exists():
            return path
    return None


def units_with_a_target(target_dir: Path) -> set[str]:
    """Unit stems the target directory actually holds an object for.

    THE OBJS ON DISK ARE THE ANSWER. Predicting this set from a name census is
    what once left data-only units pointing at the empty dummy.obj while their
    real target objs sat unopened beside it - a pairing objdiff scores 100.00%
    on every measure with zero totals, so the unit reports MATCHING while being
    entirely unscored.
    """
    if not target_dir.is_dir():
        return set()
    units: set[str] = set()
    for path in target_dir.rglob("*.obj"):
        relative = path.relative_to(target_dir).as_posix()
        for suffix in TARGET_SUFFIXES:
            if relative.endswith(suffix):
                units.add(relative[:-len(suffix)])
                break
    return units


#: The one undefined external every relaxed data relocation targets.
DATA_SINK = "$data"
#: IMAGE_REL_I386_DIR32 / DIR32NB: the types whose addend is stored inline as
#: a plain symbol offset (the field objdiff-score-reloc-addend.patch scores).
_INLINE_ADDEND_TYPES = (canon.DIR32, 0x0007)
_CNT_CODE = 0x00000020


@dataclass(frozen=True)
class RelaxedRelocation:
    relocation_offset: int     # file offset of the 10-byte relocation record
    section: int
    site: int
    original_symbol: str
    original_addend: int


def _is_function_symbol(coff: canon.CoffObject, symbol: canon.Symbol) -> bool:
    """COFF type DT_FUNCTION (0x20), or any symbol defined in a code section
    (a jump-table label, a funclet, a section symbol of `.text`)."""
    if (symbol.typ >> 4) & 0x3 == 2:
        return True
    if symbol.section > 0:
        flags = coff.sections[symbol.section - 1].characteristics
        return bool(flags & (canon.MEM_EXECUTE | _CNT_CODE))
    return False


def _stays_strict(symbol: canon.Symbol) -> bool:
    """Non-function targets that are nevertheless CALL or EH identities.

    `__imp_*` is an IAT slot: `call [__imp__Foo@4]` names the callee, so it
    stays a function call target. The EH band's records (`__ehfuncinfo$` and
    friends) are the /GX handling, which stays strict."""
    name = symbol.name
    return (name == DATA_SINK or name.startswith("__imp_")
            or eh_band.is_band_symbol(name.removeprefix(canon.DUP_PREFIX))
            or eh_band.is_band_data_symbol(name.removeprefix(canon.DUP_PREFIX)))


def is_relaxed(coff: canon.CoffObject, relocation: canon.Relocation) -> bool:
    """Whether `relax_data_relocations` retargets this relocation to `$data`:
    a DIR32/DIR32NB in a code section whose target is neither a function nor
    a call/EH identity. `homm1.verify.data_identity` audits exactly this set."""
    section = coff.sections[relocation.section - 1]
    if not section.characteristics & (canon.MEM_EXECUTE | _CNT_CODE):
        return False
    if relocation.typ not in _INLINE_ADDEND_TYPES:
        return False
    target = coff.symbols[relocation.symbol_index]
    return not (_is_function_symbol(coff, target) or _stays_strict(target))


def relax_data_relocations(payload: bytes) -> tuple[bytes, tuple]:
    """Stop scoring DATA identity: the data_matching = false step.

    Every relocation in a CODE section whose target is not a function
    (`_is_function_symbol`) and not a call/EH identity (`_stays_strict`) is
    retargeted to one undefined external `$data` appended at the END of the
    symbol table (so no existing index moves), and its inline DIR32/DIR32NB
    addend is zeroed. Both copies get the same treatment, so objdiff sees
    `$data+0` on both sides wherever both sides referenced some datum:
    two undefined externals compare by name and addend, which is exactly the
    comparison this removes. objdiff's own `functionRelocDiffs` cannot say
    this - its relaxed modes drop FUNCTION call identity too.

    Still strict: every instruction byte and immediate, the relocation's
    presence, site and type, function call targets (REL32 and DIR32 function
    pointers), IAT calls, jump tables, the EH band, and every relocation in a
    data section (the data-section referent audit, verify.data_relocs, reads
    those). A reference to a datum on one side and a function on the other
    still differs. `_assert_only_data_relaxation` re-proves byte for byte that
    nothing else moved.
    """
    coff = canon.CoffObject(payload)
    if any(s.name == DATA_SINK for s in coff.symbols.values()):
        raise ValueError(f"object already defines the relaxation sink {DATA_SINK}")
    relaxed: list[RelaxedRelocation] = []
    for relocation in coff.relocations:
        if not is_relaxed(coff, relocation):
            continue
        section = coff.sections[relocation.section - 1]
        target = coff.symbols[relocation.symbol_index]
        operand = section.raw_offset + relocation.site
        relaxed.append(RelaxedRelocation(
            relocation.offset, relocation.section, relocation.site, target.name,
            struct.unpack_from("<I", payload, operand)[0]))
    if not relaxed:
        return payload, ()

    sink = coff.symbol_count
    data = bytearray(payload[:coff.string_offset])
    for row in relaxed:
        section = coff.sections[row.section - 1]
        struct.pack_into("<I", data, section.raw_offset + row.site, 0)
        struct.pack_into("<I", data, row.relocation_offset + 4, sink)
    struct.pack_into("<I", data, 12, coff.symbol_count + 1)
    data += struct.pack("<8sIhHBB", DATA_SINK.encode("ascii").ljust(8, b"\0"),
                        0, 0, 0, canon.EXTERNAL_STORAGE, 0)
    data += payload[coff.string_offset:]
    result = bytes(data)
    _assert_only_data_relaxation(coff, payload, result, relaxed)
    return result, tuple(relaxed)


def _assert_only_data_relaxation(original: canon.CoffObject, payload: bytes,
                                 result: bytes, relaxed) -> None:
    """Fail closed unless ONLY data relocation targets and their addend
    fields changed (plus the appended sink and the symbol count)."""
    new = canon.CoffObject(result)
    sink = original.symbol_count
    if new.symbol_count != sink + 1 or new.symbol_offset != original.symbol_offset:
        raise RuntimeError("data relaxation changed the symbol table shape")
    added = new.symbols.get(sink)
    if added is None or (added.name, added.value, added.section, added.typ,
                         added.storage_class, added.aux_count) != (
                             DATA_SINK, 0, 0, 0, canon.EXTERNAL_STORAGE, 0):
        raise RuntimeError("data relaxation appended a malformed sink symbol")
    by_offset = {row.relocation_offset: row for row in relaxed}
    if len(by_offset) != len(relaxed):
        raise RuntimeError("data relaxation recorded a relocation twice")
    if len(original.relocations) != len(new.relocations):
        raise RuntimeError("data relaxation changed the relocation count")
    for before, after in zip(original.relocations, new.relocations):
        if (before.offset, before.section, before.site, before.typ) != (
                after.offset, after.section, after.site, after.typ):
            raise RuntimeError("data relaxation moved a relocation")
        row = by_offset.get(before.offset)
        if row is None:
            if after.symbol_index != before.symbol_index:
                raise RuntimeError("data relaxation retargeted a strict relocation")
            continue
        target = original.symbols[before.symbol_index]
        if (_is_function_symbol(original, target) or _stays_strict(target)
                or after.symbol_index != sink):
            raise RuntimeError(
                f"data relaxation touched a non-data relocation to {target.name}")
    # Byte proof: the file before the old string table is identical except the
    # symbol count, each relaxed record's symbol index and each relaxed
    # addend; the sink record follows; the string table is unchanged.
    mask = [(12, 16)]
    for row in relaxed:
        operand = original.sections[row.section - 1].raw_offset + row.site
        if struct.unpack_from("<I", result, operand)[0] != 0:
            raise RuntimeError("data relaxation left a nonzero addend")
        mask += [(operand, operand + 4),
                 (row.relocation_offset + 4, row.relocation_offset + 8)]
    before = bytearray(payload[:original.string_offset])
    after = bytearray(result[:original.string_offset])
    for lo, hi in mask:
        before[lo:hi] = after[lo:hi] = bytes(hi - lo)
    if before != after:
        raise RuntimeError("data relaxation changed bytes outside its fields")
    if result[original.string_offset + canon.SYMBOL_SIZE:] != \
            payload[original.string_offset:]:
        raise RuntimeError("data relaxation changed the string table")


def comparison_copy(payload: bytes, *, data_matching_on: bool | None = None):
    """canonicalize, then (data matching off) relax: (bytes, sidecar rows)."""
    result = canon.canonicalize_coff(payload)
    rows = list(result.rows)
    on = data_matching.enabled() if data_matching_on is None else data_matching_on
    if on:
        return result.data, tuple(rows)
    data, relaxed = relax_data_relocations(result.data)
    if relaxed:
        rows.append(canon.CanonicalRow(
            DATA_SINK, DATA_SINK, "data-relax", "undefined", 0, 0, 0, 0,
            len(relaxed), "-", "data-matching-off-data-reloc-relaxed",
            ",".join(sorted({row.original_symbol for row in relaxed}))[:200]))
    return data, tuple(rows)


def _stale(src: Path, out: Path) -> bool:
    if not out.exists():
        return True
    out_mtime = out.stat().st_mtime
    return out_mtime < src.stat().st_mtime or out_mtime < _MODULE_MTIME


def _normalize_one(src: Path, out_obj: Path, out_sidecar: Path, *,
                   force: bool = False,
                   function_claims: tuple[tuple[str, int], ...] = ()) -> str:
    """Normalize src -> out_obj (+ sidecar) when stale. Return a state token."""
    if not force and not _stale(src, out_obj) and not _stale(src, out_sidecar):
        return "skip"
    data, rows = comparison_copy(src.read_bytes())
    data = canon.add_function_padding_boundaries(data, function_claims)
    canon._atomic_write(out_obj, data)
    canon._atomic_write(out_sidecar, canon.sidecar_bytes(rows))
    return "wrote"


def _weak_and_strong_names(path: Path) -> tuple[set[str], set[str]]:
    """({weakly referenced}, {strongly defined}) symbol names of one COFF object.

    A raw symbol-table scan, not a full parse: the whole-link check below only
    needs the two name sets and runs over every processed object each build.
    """
    data = path.read_bytes()
    symptr, nsym = struct.unpack_from("<II", data, 8)
    strtab = symptr + nsym * SYMBOL_SIZE
    weak: set[str] = set()
    strong: set[str] = set()
    index = 0
    while index < nsym:
        base = symptr + index * SYMBOL_SIZE
        section, _typ, storage, aux = struct.unpack_from("<hHBB", data, base + 12)
        if struct.unpack_from("<I", data, base)[0]:
            name = data[base:base + 8].split(b"\0")[0].decode("latin1")
        else:
            offset = strtab + struct.unpack_from("<I", data, base + 4)[0]
            name = data[offset:data.index(b"\0", offset)].decode("latin1")
        if storage == canon.WEAK_EXTERNAL_STORAGE:
            weak.add(name)
        elif storage == canon.EXTERNAL_STORAGE and section > 0:
            strong.add(name)
        index += 1 + aux
    return weak, strong


def _assert_weak_externals_have_no_strong_definition(paths: list[Path]) -> int:
    """Fail if a weakly-referenced name is defined strongly anywhere.

    `canonicalize_coff` resolves each weak external to its auxiliary default,
    which is what the linker does ONLY while no object supplies a strong
    definition of that name. That is a whole-link fact, so it is re-proven here
    over the whole processed set rather than assumed per object. Measured today:
    508 weak `??_E<C>@@UAEPAXI@Z` references, 6 strong `??_E` definitions, and
    the two name sets are disjoint (the strong ones are non-virtual `QAEPAXI@Z`
    bodies and `W7AEPAXI@Z` thunks, a different mangling).
    """
    weak: set[str] = set()
    strong: set[str] = set()
    for path in paths:
        one_weak, one_strong = _weak_and_strong_names(path)
        weak |= one_weak
        strong |= one_strong
    clash = sorted(weak & strong)
    if clash:
        raise SystemExit(
            "[normalize] FATAL: %d weak external(s) also have a strong definition, "
            "so resolving them to their default is no longer what the linker does: %s"
            % (len(clash), ", ".join(clash[:8])))
    return len(weak)


def _assert_runtime_aliases_are_not_defined(paths: list[Path]) -> int:
    """Fail if a compared object strongly defines an OLDNAMES alias name.

    `canonicalize_coff` names a reference to `chdir` by `_chdir`, which is what
    LINK does only while nothing else defines `chdir`. Like the weak-external
    check above, that is a whole-link fact, re-proven over the processed set.
    """
    table = runtime_aliases.aliases()
    strong: set[str] = set()
    for path in paths:
        strong |= _weak_and_strong_names(path)[1]
    clash = sorted(strong & set(table))
    if clash:
        raise SystemExit(
            "[normalize] FATAL: %d OLDNAMES alias name(s) are defined by a "
            "compared object, so the runtime function is no longer what LINK "
            "binds them to: %s" % (len(clash), ", ".join(clash[:8])))
    return len(table)


def normalize(base_dir: Path, target_dir: Path, out_dir: Path,
              units: list[str], *, stamp: Path | None = None,
              force: bool = False, quiet: bool = False) -> dict:
    """Normalize both sides of `units` into `<out_dir>/{base,target}/`.

    Returns the counts the stamp records. `units` comes from the caller (the
    manifest census); which of them have a delinked target is read off disk.
    """
    base_dir, target_dir, out_dir = Path(base_dir), Path(target_dir), Path(out_dir)
    base_out = out_dir / "base"
    target_out = out_dir / "target"
    stamp = stamp if stamp is not None else out_dir / "normalize.stamp"

    # A selected-unit build must not mix scoring modes in one objdiff project.
    mode_path = out_dir / "data-matching.mode"
    mode = str(data_matching.enabled()).lower() + "\n"
    if not mode_path.exists() or mode_path.read_text() != mode:
        units = sorted(set(units) | {
            p.relative_to(base_dir).with_suffix("").as_posix()
            for p in base_dir.rglob("*.obj")
        } | units_with_a_target(target_dir))
        force = True

    wrote = skipped = base_n = target_n = 0
    processed: list[str] = []
    ordered = sorted(units)
    inputs = [p for unit in ordered
              for p in (base_dir / f"{unit}.obj", target_object(target_dir, unit))
              if p is not None and p.exists()]
    weak_n = _assert_weak_externals_have_no_strong_definition(inputs)
    _assert_runtime_aliases_are_not_defined(inputs)

    for unit in ordered:
        from homm1.graph.fixed_asm import unit as fixed_asm_unit
        fixed = fixed_asm_unit(unit)
        function_claims = tuple(
            (claim.name, claim.size) for claim in fixed.claims
            if claim.kind == "func") if fixed is not None else ()
        base_src = base_dir / f"{unit}.obj"
        if base_src.exists():
            state = _normalize_one(
                base_src, base_out / f"{unit}.obj", base_out / f"{unit}.symbols.tsv",
                force=force, function_claims=function_claims)
            wrote += state == "wrote"
            skipped += state == "skip"
            base_n += 1
            processed.append(f"base/{unit}")
        target_src = target_object(target_dir, unit)
        target_sidecar = target_out / f"{unit}.symbols.tsv"
        if target_src is not None:
            target_obj = target_out / target_src.relative_to(target_dir)
            state = _normalize_one(
                target_src, target_obj, target_sidecar, force=force,
                function_claims=function_claims)
            wrote += state == "wrote"
            skipped += state == "skip"
            target_n += 1
            processed.append(f"target/{unit}")
        else:
            target_obj = None
        # A unit that lost its delinked target (e.g. all names removed), or whose
        # target changed suffix, must not leave a stale normalized copy behind for
        # objdiff to pair against.
        stale_paths = [target_out / f"{unit}{suffix}" for suffix in TARGET_SUFFIXES]
        if target_src is None:
            stale_paths.append(target_sidecar)
        for stale in stale_paths:
            if stale != target_obj and stale.exists():
                stale.unlink()

    digest = hashlib.sha256("\n".join(processed).encode("utf-8")).hexdigest()
    counts = {"base_objects": base_n, "target_objects": target_n,
              "wrote": wrote, "skipped": skipped, "weak_externals": weak_n,
              "set_sha256": digest}
    canon._atomic_write(
        stamp,
        ("# normalized objdiff comparison copies\n"
         f"base_objects\t{base_n}\n"
         f"target_objects\t{target_n}\n"
         f"wrote\t{wrote}\n"
         f"skipped\t{skipped}\n"
         f"set_sha256\t{digest}\n").encode("utf-8"))
    canon._atomic_write(mode_path, mode.encode("ascii"))
    if not quiet:
        print(f"[normalize] base={base_n} target={target_n} wrote={wrote} "
              f"skipped={skipped} weak-externals-resolved={weak_n}")
    return counts


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        prog="python3 -m homm1.compare.normalize", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--base-dir", required=True, type=Path)
    ap.add_argument("--target-dir", required=True, type=Path)
    ap.add_argument("--out-dir", required=True, type=Path)
    ap.add_argument("--stamp", type=Path)
    ap.add_argument("--force", action="store_true",
                    help="rewrite every copy, ignoring the stale check")
    ap.add_argument("--unit", action="append", default=[], dest="units",
                    help="repeatable; defaults to the config/units.toml census")
    args = ap.parse_args(argv)

    units = args.units
    if not units:
        from homm1.manifest import units as manifest_units
        units = [u["unit"] for u in manifest_units()]
    normalize(args.base_dir, args.target_dir, args.out_dir, units,
              stamp=args.stamp, force=args.force)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
