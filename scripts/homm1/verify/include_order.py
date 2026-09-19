"""homm1.verify.include_order - the canonical #include block (fast tier).

Ported: DUPLICATES, ORDER, and header SELF-SUFFICIENCY are gated. The order
(groups, blank-line separated): 0 config #defines; 1 <rva.h>; 2 the TU's own
header; 3 the platform preludes in DEPENDENCY order (Mfc.h, MfcNoInline.h,
MfcWin.h, windows.h - they configure how later headers parse, so group 3 is
RANKED, not sorted); 4 project headers; 5 libraries. A header that names a
platform type pulls its own prelude (self-sufficiency; proven by the
2026-08-02 standalone-compile sweep). Anything unrecognised in the block
makes the file MANUAL: reported, never mangled.

    python3 -m homm1.verify.include_order            # report
    python3 -m homm1.verify.include_order --gate     # exit 1 on violations
    python3 -m homm1.verify.include_order --fix-dupes | --fix | --fix-prelude
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

from homm1.core.paths import REPO

SRC_DIRS = ("src", "include")
EXTS = (".h", ".cpp", ".hpp", ".inl", ".c")

INC_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
IFNDEF_RE = re.compile(r"^\s*#\s*ifndef\s+(\w+)\s*$")
DEFINE_RE = re.compile(r"^\s*#\s*define\s+(\w+)\s*$")
PP_RE = re.compile(r"^\s*#\s*(\w+)")

RVA_H = "match.h"

PRELUDE_RANK = {"Mfc.h": 0, "MfcNoInline.h": 1, "MfcWin.h": 2, "windows.h": 3}

G_RVA, G_OWN, G_PRELUDE, G_PROJECT, G_LIBRARY = 1, 2, 3, 4, 5
GROUPS = (G_RVA, G_OWN, G_PRELUDE, G_PROJECT, G_LIBRARY)

AFXWIN_TOKENS = re.compile(
    r"\b(CWnd|CDialog|CDC|CClientDC|CPaintDC|CWindowDC|CRgn|CBitmap|CPalette|"
    r"CFont|CBrush|CPen|CGdiObject|CWinApp|CWinThread|CFrameWnd|CView|"
    r"CDocument|CMenu|CButton|CEdit|CListBox|CComboBox|CStatic|CScrollBar|"
    r"CRect|CPoint|CSize|"
    r"DECLARE_MESSAGE_MAP|BEGIN_MESSAGE_MAP)\b")
AFX_TOKENS = re.compile(
    r"\b(CString|CObject|CFile|CArchive|CException|CMemFile|CRuntimeClass|"
    r"CPtrArray|CPtrList|CObList|CObArray|CStringList|CStringArray|CByteArray|"
    r"CWordArray|CDWordArray|CUIntArray|CMapPtrToPtr|CMapPtrToWord|"
    r"CMapStringToPtr|CMapStringToOb|CMapStringToString|CMapWordToPtr|"
    r"CMapWordToOb|CTime|CTimeSpan|POSITION|DECLARE_DYNAMIC|DECLARE_DYNCREATE|"
    r"DECLARE_SERIAL|IMPLEMENT_DYNAMIC|IMPLEMENT_DYNCREATE|IMPLEMENT_SERIAL)\b")
WIN_TOKENS = re.compile(
    r"\b(HWND|HDC|HINSTANCE|HBITMAP|HPALETTE|HMODULE|HRESULT|HGLOBAL|LPARAM|"
    r"WPARAM|LRESULT|tagRECT|tagPOINT|PALETTEENTRY|WINAPI|CALLBACK|IUnknown|"
    r"CRITICAL_SECTION|LARGE_INTEGER|WNDPROC|COLORREF|LPDIRECT\w+)\b")
MFC_SUPPLIERS = {"Mfc.h", "MfcWin.h", "MfcNoInline.h", "afx.h", "afxwin.h",
                 "afxtempl.h", "afxcmn.h"}
AFXWIN_SUPPLIERS = {"MfcWin.h", "afxwin.h", "afxcmn.h"}
WIN_SUPPLIERS = MFC_SUPPLIERS | {"windows.h"}

FWD_RE = re.compile(r"^\s*(?:class|struct|union)\s+(\w+)\s*;", re.M)
ELAB_RE = re.compile(r"\b(?:class|struct|union)\s+(\w+)")


def _vendor_win_suppliers():
    out = set()
    vendor = REPO / "vendor"
    if vendor.is_dir():
        for p in vendor.rglob("*"):
            if p.suffix.lower() == ".h":
                try:
                    txt = p.read_text(encoding="utf-8", errors="replace")
                except OSError:
                    continue
                if re.search(r'^\s*#\s*include\s*[<"]windows\.h[>"]', txt, re.M):
                    out.add(p.name)
    return out


def repo_files():
    for d in SRC_DIRS:
        for p in sorted((REPO / d).rglob("*")):
            if p.suffix in EXTS:
                yield p


PROJECT_HEADERS = {p.relative_to(REPO / "include").as_posix()
                   for p in (REPO / "include").rglob("*.h")}


def own_header(path: Path):
    if path.suffix != ".cpp":
        return None
    rel = path.relative_to(REPO / "src")
    cand = rel.with_suffix(".h").as_posix()
    if cand in PROJECT_HEADERS:
        return cand
    bare = rel.name[:-4] + ".h"
    return bare if bare in PROJECT_HEADERS else None


def classify(header: str, own: str | None) -> int:
    if header == RVA_H:
        return G_RVA
    if header in PRELUDE_RANK:
        return G_PRELUDE
    if own and header == own:
        return G_OWN
    if header in PROJECT_HEADERS:
        return G_PROJECT
    return G_LIBRARY


def sort_key(group: int, header: str):
    if group == G_PRELUDE:
        return (PRELUDE_RANK[header],)
    return (header.lower(),)


class Manual(Exception):
    """The include block holds something this tool will not rewrite."""


def parse(path: Path):
    """-> (head, entries, tail). A comment inside the block travels with the
    include it introduces; a trailing comment with no include belongs to the
    code below (keeps `// @early-stop` attached to its function)."""
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    i, n, head = 0, len(lines), []

    def trivia():
        nonlocal i
        while i < n and (not lines[i].strip()
                         or lines[i].lstrip().startswith(("//", "/*", "*"))):
            head.append(lines[i])
            i += 1

    trivia()
    if i + 1 < n and (m := IFNDEF_RE.match(lines[i])):
        m2 = DEFINE_RE.match(lines[i + 1])
        if m2 and m2.group(1) == m.group(1):
            head.extend(lines[i:i + 2])
            i += 2
    trivia()
    while i < n and DEFINE_RE.match(lines[i]):
        head.append(lines[i])
        i += 1
        trivia()

    entries, start = [], i
    pending: list[str] = []
    pending_at = i
    while i < n:
        s = lines[i].strip()
        if not s:
            i += 1
            continue
        if s.startswith(("//", "/*", "*")):
            if not pending:
                pending_at = i
            pending.append(lines[i])
            i += 1
            continue
        if m := INC_RE.match(lines[i]):
            entries.append((pending, m.group(1)))
            pending = []
            i += 1
            pending_at = i
            continue
        break

    if pending:
        i = pending_at
    if not entries:
        return head, [], lines[start:]

    depth = 0
    for j, line in enumerate(lines[i:]):
        s = line.strip()
        if m := PP_RE.match(s):
            d = m.group(1)
            if d in ("if", "ifdef", "ifndef"):
                depth += 1
            elif d == "endif":
                depth -= 1
            elif d == "include":
                raise Manual(f"include below the block (+{j})")
        elif s and not s.startswith(("//", "/*", "*")):
            break
    return head, entries, lines[i:]


def render(head, entries, tail, own):
    groups: dict[int, list] = {}
    seen: set[str] = set()
    for comments, h in entries:
        if h in seen:
            for g in groups.values():
                for k, (c, hh) in enumerate(g):
                    if hh == h:
                        g[k] = (c + comments, hh)
            continue
        seen.add(h)
        groups.setdefault(classify(h, own), []).append((comments, h))
    out = list(head)
    while out and not out[-1].strip():
        out.pop()
    if out:
        out.append("")
    first = True
    for g in GROUPS:
        if g not in groups:
            continue
        if not first:
            out.append("")
        first = False
        for comments, h in sorted(groups[g], key=lambda e: sort_key(g, e[1])):
            out.extend(comments)
            out.append(f"#include <{h}>")
    body = list(tail)
    while body and not body[0].strip():
        body.pop(0)
    if body:
        out.append("")
        out.extend(body)
    return out


def assert_conserved(path: Path, before, after, dropped):
    """Nothing but blank lines and duplicate includes may change - the reorder
    is a permutation, not an edit (a silent loss once ate 38 markers)."""
    from collections import Counter
    b = Counter(ln.strip() for ln in before if ln.strip())
    a = Counter(ln.strip() for ln in after if ln.strip())
    for h in dropped:
        for cand in (f"#include <{h}>", f'#include "{h}"'):
            if b[cand]:
                b[cand] -= 1
                if not b[cand]:
                    del b[cand]
                break
    if a != b:
        lost, gained = (b - a), (a - b)
        raise SystemExit(
            f"[include-order] ABORT: rewrite of {path} is not line-conserving\n"
            f"   lost:   {list(lost.elements())[:8]}\n"
            f"   gained: {list(gained.elements())[:8]}")


_SUPPLY_CACHE: dict[str, tuple[bool, bool, bool]] = {}
_VENDOR_WIN: set[str] | None = None


def _header_text(name: str) -> str:
    try:
        return (REPO / "include" / name).read_text(encoding="utf-8",
                                                   errors="replace")
    except OSError:
        return ""


def _supply(name: str, stack=()) -> tuple[bool, bool, bool]:
    global _VENDOR_WIN
    if _VENDOR_WIN is None:
        _VENDOR_WIN = _vendor_win_suppliers()
    if name in AFXWIN_SUPPLIERS:
        return True, True, True
    if name in MFC_SUPPLIERS:
        return True, False, True
    if name in WIN_SUPPLIERS or name in _VENDOR_WIN:
        return False, False, True
    if name not in PROJECT_HEADERS or name in stack:
        return False, False, False
    if name in _SUPPLY_CACHE:
        return _SUPPLY_CACHE[name]
    afx = afxwin = win = False
    for line in _header_text(name).splitlines():
        if m := INC_RE.match(line):
            a, aw, w = _supply(m.group(1), stack + (name,))
            afx, afxwin, win = afx or a, afxwin or aw, win or w
    _SUPPLY_CACHE[name] = (afx, afxwin, win)
    return afx, afxwin, win


def missing_prelude(path: Path, headers) -> list[str]:
    if path.suffix != ".h" or path.name in PRELUDE_RANK:
        return []
    txt = path.read_text(encoding="utf-8", errors="replace")
    incs = [m.group(1) for m in (INC_RE.match(ln) for ln in txt.splitlines())
            if m]
    afx_ok = any(_supply(h)[0] or h in MFC_SUPPLIERS for h in incs)
    afxwin_ok = any(_supply(h)[1] or h in AFXWIN_SUPPLIERS for h in incs)
    win_ok = afx_ok or any(_supply(h)[2] or h in WIN_SUPPLIERS for h in incs)
    # comments are stripped: a type named in PROSE needs no declaration
    body = "\n".join(ln for ln in txt.splitlines() if not INC_RE.match(ln))
    body = re.sub(r"/\*.*?\*/", " ", body, flags=re.S)
    body = re.sub(r"//[^\n]*", "", body)
    declared = set(FWD_RE.findall(body)) | set(ELAB_RE.findall(body))
    for h in incs:
        if h in PROJECT_HEADERS:
            declared |= set(FWD_RE.findall(_header_text(h)))
    want = []
    afxwin_hits = set(AFXWIN_TOKENS.findall(body)) - declared
    if afxwin_hits and not afxwin_ok:
        want.append("MfcWin.h")
    afx_hits = set(AFX_TOKENS.findall(body)) - declared
    if not want and afx_hits and not afx_ok:
        want.append("Mfc.h")
    win_hits = set(WIN_TOKENS.findall(body)) - declared
    if not want and win_hits and not win_ok:
        want.append("Mfc.h")
    return want


def audit(fix=False, fix_dupes=False, fix_prelude=False):
    """(dupes, preludes, unordered, manual, changed)."""
    dupes, preludes, unordered, manual = {}, {}, [], {}
    changed = 0
    for path in repo_files():
        rel = path.relative_to(REPO).as_posix()
        try:
            head, entries, tail = parse(path)
        except Manual as e:
            manual[rel] = str(e)
            continue
        if not entries:
            continue
        own = own_header(path)
        headers = [h for _, h in entries]
        dropped = [h for i, h in enumerate(headers) if h in headers[:i]]
        if dropped:
            dupes[rel] = sorted(set(dropped))
        want_add = missing_prelude(path, headers)
        if want_add:
            preludes[rel] = want_add
        work = list(entries)
        if fix_prelude:
            work.extend(([], h) for h in want_add)
        want = render(head, work, tail, own)
        have = path.read_text(encoding="utf-8", errors="replace").splitlines()
        if want != have:
            if not dropped and not want_add:
                unordered.append(rel)
            do = fix or (fix_dupes and dropped) or (fix_prelude and want_add)
            if do:
                assert_conserved(
                    path, have,
                    want if not fix_prelude else
                    [ln for ln in want
                     if ln.strip() not in {f"#include <{h}>" for h in want_add}],
                    dropped)
                path.write_text("\n".join(want) + "\n", encoding="utf-8")
                changed += 1
    return dupes, preludes, unordered, manual, changed


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="homm1 verify include-order",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on duplicates, missing preludes or a disordered block")
    ap.add_argument("--fix-dupes", action="store_true",
                    help="rewrite files to drop duplicate includes")
    ap.add_argument("--fix-prelude", action="store_true",
                    help="rewrite files to add the missing platform prelude")
    ap.add_argument("--fix", action="store_true",
                    help="rewrite files into the full canonical order")
    ap.add_argument("--verbose", "-v", action="store_true",
                    help="list every file, not just the violations")
    a = ap.parse_args(argv)

    dupes, preludes, unordered, manual, changed = audit(
        a.fix, a.fix_dupes, a.fix_prelude)
    ndupe = sum(len(v) for v in dupes.values())
    print(f"[include-order] duplicate includes:      {ndupe} in "
          f"{len(dupes)} file(s)")
    print(f"[include-order] headers missing prelude: {len(preludes)}")
    print(f"[include-order] files out of order:      {len(unordered)}")
    print(f"[include-order] MANUAL (untouched):      {len(manual)}")
    if a.verbose:
        for rel, d in sorted(dupes.items()):
            print(f"   dup  {rel}: {', '.join(d)}")
        for rel, w in sorted(preludes.items()):
            print(f"   pre  {rel}: {', '.join(w)}")
        for rel in unordered:
            print(f"   ord  {rel}")
    for rel, why in sorted(manual.items()):
        print(f"   MANUAL {rel}: {why}")

    if changed:
        print(f"[include-order] rewrote {changed} file(s)")
        return 0
    if a.gate and (ndupe or unordered or preludes):
        print("[include-order] FATAL: include block is not canonical - fix "
              "with `python3 -m homm1.verify.include_order --fix-dupes "
              "--fix` (preludes: add the includer-side prelude by hand)")
        return 1
    if a.gate:
        print("[include-order] OK - deduped, canonical order, every header "
              "self-sufficient")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
