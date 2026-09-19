# apply.py - push the HoMM1 reconstruction's knowledge into the viewer DB.
#
# A GhidraScript, run under PyGhidra by headless.py (which injects the flat-API
# globals currentProgram/monitor/state). It is STATELESS with respect to this
# tree: its single input is the payload homm1.ghidra.export wrote, whose path
# arrives in $HOMM1_GHIDRA_PAYLOAD. Nothing here re-derives a fact.
#
# ONE-WAY: the export is a viewer. Nothing this script writes is ever read back
# into the matching pipeline - Ghidra's carve, its decompiler and its type
# guesses are not evidence, so where the payload states a boundary the payload
# WINS (analyzer-only .text functions are removed, admitted bodies are resized).
#
# Idempotent: the plate blocks it writes are delimited and replaced, names are
# skipped once the mangled label is present, and re-running changes nothing.
#@category HoMM1
import json
import os

from ghidra.app.cmd.disassemble import DisassembleCommand
from ghidra.app.cmd.label import DemanglerCmd
from ghidra.program.model.address import AddressSet
from ghidra.program.model.data import (ArrayDataType, DataUtilities,
                                       DoubleDataType, FloatDataType,
                                       PointerDataType,
                                       TerminatedStringDataType,
                                       Undefined1DataType)
from ghidra.program.model.listing import BookmarkType, CommentType
from ghidra.program.model.symbol import SourceType
from ghidra.util.task import TaskMonitor

PLATE = CommentType.PLATE
EOL = CommentType.EOL
SRC = SourceType.IMPORTED          # the reconstruction is an import, not a guess
BEGIN = "[homm1]"
END = "[/homm1]"
BAND_BEGIN = "[homm1-band]"
BAND_END = "[/homm1-band]"
BAND_CATEGORY = "homm1-band"

prog = currentProgram                                            # noqa: F821
listing = prog.getListing()
fm = prog.getFunctionManager()
st = prog.getSymbolTable()
bm = prog.getBookmarkManager()
mem = prog.getMemory()
space = prog.getAddressFactory().getDefaultAddressSpace()

PAYLOAD = os.environ["HOMM1_GHIDRA_PAYLOAD"]
BOOKMARKS = os.environ.get("HOMM1_GHIDRA_BOOKMARKS", "1") != "0"
REPORT = os.path.join(os.path.dirname(PAYLOAD), "apply-report.txt")

_report = []


def R(line):
    _report.append(line)
    print(line)


with open(PAYLOAD) as fh:
    DOC = json.load(fh)

BASE = DOC["image_base"]


def addr(rva):
    return space.getAddress(BASE + rva)


# ---------------------------------------------------------------- comments
def set_plate(a, block_lines, begin=BEGIN, end=END):
    """Replace one delimited plate block, preserving every other line.

    Row identity and band identity use DIFFERENT markers: a band's first
    address is normally also a row's, and a single marker would let whichever
    ran last erase the other."""
    old = listing.getComment(PLATE, a)
    kept = []
    if old:
        skipping = False
        for line in str(old).split("\n"):
            if line.startswith(begin):
                skipping = True
            elif line.startswith(end):
                skipping = False
            elif not skipping:
                kept.append(line)
    block = [begin + " " + block_lines[0]] + block_lines[1:] + [end]
    listing.setComment(a, PLATE, "\n".join(block + kept).rstrip())


def clear_reapplied_plate(a):
    """Drop the whole plate when a PREVIOUS apply owned it.

    DemanglerCmd APPENDS its signature line to the plate, so re-identifying an
    address would stack one line per apply. Our marker's presence proves the
    foreign lines below it are a previous demangle, not a human's note; without
    the marker (a first apply) Ghidra's own plate is left alone."""
    old = listing.getComment(PLATE, a)
    if old and BEGIN in str(old):
        listing.setComment(a, PLATE, None)


def set_eol(a, line):
    """Replace this script's single EOL line, preserving any other text."""
    old = listing.getComment(EOL, a)
    kept = [ln for ln in str(old).split("\n")
            if old and not ln.startswith(BEGIN)]
    listing.setComment(a, EOL, "\n".join([BEGIN + " " + line] + kept).rstrip())


def mark(rec):
    """The one-line identity summary shared by the plate and the EOL comment."""
    bits = ["size=0x%x" % rec["size"]]
    for key in ("kind", "channel", "unit", "band"):
        if rec.get(key):
            bits.append("%s=%s" % (key, rec[key]))
    return " ".join(bits)


# ------------------------------------------------------------------- names
def names_at(a):
    return set(str(s.getName()) for s in st.getSymbols(a))


def apply_name(a, mangled):
    """Apply the retail mangled name, letting Ghidra's MSVC demangler produce
    the readable name + class namespace + signature. The mangled spelling is
    kept as a secondary label so the symbol table round-trips it.

    Returns 'demangled' | 'literal' | 'already' | 'failed'."""
    if mangled in names_at(a):
        return "already"          # our label is here: nothing to reapply
    # Reaching here on an EXISTING project means the Model renamed this rva.
    # Drop what a previous apply left (its mangled label and whatever the
    # demangler made of it) so `update` REPLACES an identity instead of
    # accumulating stale spellings. USER_DEFINED labels are a human's and
    # are never touched.
    clear_reapplied_plate(a)
    # The PRIMARY symbol is never deleted (deleting a function's primary
    # deletes the function); demoting it to DEFAULT erases the old name and
    # is also what makes the demangler willing to name this address.
    for sym in list(st.getSymbols(a)):
        if sym.isPrimary() or sym.getSource() in (SourceType.USER_DEFINED,
                                                  SourceType.DEFAULT):
            continue
        try:
            sym.delete()
        except Exception:
            pass
    prim = st.getPrimarySymbol(a)
    if prim is not None and prim.getSource() != SourceType.DEFAULT:
        try:
            prim.setName(None, SourceType.DEFAULT)
        except Exception:
            pass
    demangled = False
    try:
        demangled = bool(DemanglerCmd(a, mangled).applyTo(prog, TaskMonitor.DUMMY))
    except Exception:
        demangled = False
    try:
        st.createLabel(a, mangled, SRC)         # the round-trip spelling
    except Exception:
        pass
    if demangled:
        return "demangled"
    prim = st.getPrimarySymbol(a)
    return "literal" if prim is not None else "failed"


# --------------------------------------------------------------- data types
def data_type(rec):
    """The type a census kind PROVES. Nothing is guessed: a kind with no
    proven decoding is labelled and commented, never typed."""
    kind, size = rec["kind"], rec["size"]
    if kind == "vtable" and size >= 4 and size % 4 == 0:
        return ArrayDataType(PointerDataType(), size // 4, 4)
    if kind == "string" and size > 0:
        return TerminatedStringDataType()
    if kind == "fppool":
        if size == 4:
            return FloatDataType()
        if size == 8:
            return DoubleDataType()
    return None


# ===================================================================== RUN
R("=== homm1 ghidra export: applying %s ===" % PAYLOAD)
R("payload schema=%d digest=%s image_base=0x%x"
  % (DOC["schema"], DOC["digest"][:12], BASE))

tx = prog.startTransaction("homm1-knowledge-export")
committed = False
try:
    text_rows, data_rows, band_rows = DOC["text"], DOC["data"], DOC["bands"]

    # ---- (1) OUR boundaries replace the auto-carve --------------------
    admitted = set(r["rva"] for r in text_rows)
    text_block = mem.getBlock(".text")
    n_removed = 0
    if text_block is not None:
        for fn in list(fm.getFunctions(True)):
            entry = fn.getEntryPoint()
            if not text_block.contains(entry):
                continue
            if (entry.getOffset() - BASE) not in admitted:
                if fm.removeFunction(entry):
                    n_removed += 1
    R("auto-carved .text functions removed (not an admitted start): %d" % n_removed)

    # Seed disassembly only where Ghidra left an admitted start undecoded;
    # the flow-follow is restricted to .text so no data is decoded as code.
    seeds = AddressSet()
    for rec in text_rows:
        a = addr(rec["rva"])
        if listing.getInstructionAt(a) is None:
            seeds.addRange(a, a)
    if not seeds.isEmpty():
        restrict = AddressSet(text_block.getStart(), text_block.getEnd()) \
            if text_block is not None else None
        DisassembleCommand(seeds, restrict, True).applyTo(prog, monitor)  # noqa: F821
    R("admitted starts needing disassembly: %d" % seeds.getNumAddresses())

    n_created = n_resized = n_kept = n_nofunc = 0
    for rec in text_rows:
        a = addr(rec["rva"])
        size = rec["size"]
        body = AddressSet(a, a.add(size - 1)) if size else None
        fn = fm.getFunctionAt(a)
        if fn is None:
            try:
                fn = fm.createFunction(None, a, body, SRC)
            except Exception:
                fn = None
            if fn is None:
                # an incidental carve may still own these bytes; evict and retry
                owner = fm.getFunctionContaining(a)
                if owner is not None:
                    fm.removeFunction(owner.getEntryPoint())
                    try:
                        fn = fm.createFunction(None, a, body, SRC)
                    except Exception:
                        fn = None
            if fn is None:
                n_nofunc += 1
                continue
            n_created += 1
        elif body is not None and fn.getBody().getNumAddresses() != size:
            try:
                fn.setBody(body)
                n_resized += 1
            except Exception:
                n_kept += 1
        else:
            n_kept += 1
    R("functions: created=%d resized=%d unchanged=%d uncreatable=%d"
      % (n_created, n_resized, n_kept, n_nofunc))

    # ---- (2) function identity: name, plate, TU bookmark --------------
    n_named = n_demangled = n_literal = n_already = n_namefail = 0
    n_plate = n_bookmark = 0
    for rec in text_rows:
        a = addr(rec["rva"])
        if rec["name"]:
            outcome = apply_name(a, rec["name"])
            n_named += 1
            n_demangled += outcome == "demangled"
            n_literal += outcome == "literal"
            n_already += outcome == "already"
            n_namefail += outcome == "failed"

        lines = ["0x%08x  %s" % (rec["rva"], rec["name"] or "(unclaimed)"),
                 "  " + mark(rec)]
        if rec["also"]:
            lines.append("  also_units: " + ", ".join(rec["also"]))
        for al in rec["aliases"]:
            lines.append("  alias: " + al)
        set_plate(a, lines)
        n_plate += 1

        if BOOKMARKS and rec["unit"]:
            bm.setBookmark(a, BookmarkType.INFO, "TU/" + rec["unit"],
                           rec["name"] or "")
            n_bookmark += 1
    R("function names applied: %d (demangled=%d literal=%d already=%d failed=%d)"
      % (n_named, n_demangled, n_literal, n_already, n_namefail))
    R("function plates written: %d | TU bookmarks: %d" % (n_plate, n_bookmark))

    # ---- (3) data identity: label, proven type, extent comment --------
    n_dlabel = n_ddemangled = n_dtyped = n_dtypefail = n_deol = 0
    n_dextent = 0
    for rec in data_rows:
        a = addr(rec["rva"])
        if rec["name"]:
            outcome = apply_name(a, rec["name"])
            n_dlabel += 1
            n_ddemangled += outcome == "demangled"
        dt = data_type(rec)
        if dt is not None:
            try:
                DataUtilities.createData(
                    prog, a, dt, -1, False,
                    DataUtilities.ClearDataMode.CLEAR_ALL_CONFLICT_DATA)
                n_dtyped += 1
            except Exception:
                n_dtypefail += 1
        elif rec["size"] > 1:
            # No kind PROVES a type here, but the census proves the EXTENT.
            # `undefined1[n]` is Ghidra's own "an object of this size, type
            # unknown" and is laid only into free space, so a type the
            # analyzer already recovered is never overwritten by a guess.
            try:
                DataUtilities.createData(
                    prog, a, ArrayDataType(Undefined1DataType(), rec["size"], 1),
                    -1, False, DataUtilities.ClearDataMode.CHECK_FOR_SPACE)
                n_dextent += 1
            except Exception:
                pass
        line = mark(rec) + " space=" + rec["space"]
        if rec["aliases"]:
            line += " aliases=" + ",".join(rec["aliases"])
        set_eol(a, line)
        n_deol += 1
        if BOOKMARKS and rec["unit"]:
            bm.setBookmark(a, BookmarkType.INFO, "TU/" + rec["unit"],
                           rec["name"] or "")
    R("data labels applied: %d (demangled=%d) | typed: %d (failed %d) | "
      "extents laid: %d | comments: %d"
      % (n_dlabel, n_ddemangled, n_dtyped, n_dtypefail, n_dextent, n_deol))

    # ---- (4) link-layout bands: a program tree, plates and bookmarks --
    tree = None
    try:
        tree = listing.getRootModule("homm1-bands")
        if tree is None:
            tree = listing.createRootModule("homm1-bands")
    except Exception:
        tree = None
    n_band = n_frag = 0
    for band in band_rows:
        lo, hi = addr(band["lo"]), addr(band["hi"] - 1)
        set_plate(lo, ["BAND %s  [0x%06x, 0x%06x)"
                       % (band["band"], band["lo"], band["hi"]),
                       "  " + band["note"]], BAND_BEGIN, BAND_END)
        if BOOKMARKS:
            bm.setBookmark(lo, BookmarkType.INFO, BAND_CATEGORY,
                           "%s: %s" % (band["band"], band["note"]))
        n_band += 1
        if tree is not None:
            try:
                name = "%s_%06x" % (band["band"], band["lo"])
                frag = listing.getFragment("homm1-bands", name)
                if frag is None:
                    frag = tree.createFragment(name)
                frag.move(lo, hi)
                frag.setComment(band["note"])
                n_frag += 1
            except Exception:
                pass
    R("bands annotated: %d (program-tree fragments: %d)" % (n_band, n_frag))

    committed = True
finally:
    prog.endTransaction(tx, committed)

c = DOC["counts"]
R("payload census: functions %d (%d claimed / %d census-only), data %d "
  "(%d claimed), units %d, aliases %d, pad rows dropped %d"
  % (c["functions"], c["functions_claimed"], c["functions_census_only"],
     c["data"], c["data_claimed"], c["units"], c["aliases"],
     c["pad_rows_dropped"]))
R("=== apply %s ===" % ("COMMITTED" if committed else "ROLLED BACK"))

try:
    with open(REPORT, "w") as fh:
        fh.write("\n".join(_report) + "\n")
except Exception:
    pass
