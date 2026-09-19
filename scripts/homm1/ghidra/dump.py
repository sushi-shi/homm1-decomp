# dump.py - read the viewer DB back out, for round-trip verification.
#
# A GhidraScript run under PyGhidra by headless.py. It WRITES NOTHING: it opens
# the applied database and prints what Ghidra now holds at the requested RVAs
# (comma-separated in $HOMM1_GHIDRA_DUMP), so `homm1 ghidra verify` can prove
# the export landed rather than trusting the apply script's own counters.
#@category HoMM1
import os

from ghidra.program.model.listing import CommentType

prog = currentProgram                                            # noqa: F821
listing = prog.getListing()
fm = prog.getFunctionManager()
st = prog.getSymbolTable()
bm = prog.getBookmarkManager()
space = prog.getAddressFactory().getDefaultAddressSpace()
BASE = prog.getImageBase().getOffset()

rvas = [int(t, 0) for t in os.environ.get("HOMM1_GHIDRA_DUMP", "").split(",")
        if t.strip()]

# the DOMAIN FILE name is what the project tree shows; ProgramDB.getName() is
# still the imported file's name, which for a nix-store image is hash-prefixed
print("== program: %s  image_base=0x%x"
      % (prog.getDomainFile().getName(), BASE))
print("== functions in DB: %d  symbols: %d"
      % (fm.getFunctionCount(), st.getNumSymbols()))

for rva in rvas:
    a = space.getAddress(BASE + rva)
    print("")
    print("== 0x%06x -> %s" % (rva, a))
    prim = st.getPrimarySymbol(a)
    print("   primary : %s" % (prim.getName(True) if prim else "(none)"))
    syms = [str(s.getName()) for s in st.getSymbols(a)]
    print("   symbols : %s" % (", ".join(syms) if syms else "(none)"))
    fn = fm.getFunctionAt(a)
    if fn is not None:
        print("   function: %s  body=0x%x bytes  sig=%s"
              % (fn.getName(True), fn.getBody().getNumAddresses(),
                 fn.getPrototypeString(False, False)))
    else:
        owner = fm.getFunctionContaining(a)
        if owner is not None:
            print("   function: (interior of %s)" % owner.getName(True))
    data = listing.getDataAt(a)
    if data is not None:
        print("   data    : %s  len=0x%x" % (data.getDataType().getName(),
                                             data.getLength()))
    for label, ctype in (("plate", CommentType.PLATE), ("eol", CommentType.EOL)):
        c = listing.getComment(ctype, a)
        if c:
            for i, line in enumerate(str(c).split("\n")):
                print("   %-8s %s" % (label if i == 0 else "", line))
    marks = bm.getBookmarks(a)
    if marks:
        print("   bookmark: %s" % ", ".join(
            "%s/%s" % (m.getCategory(), m.getComment()) for m in marks))
