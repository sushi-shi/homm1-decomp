"""Static, byte-preserving localization: catalog IDs become literal macros.

Authored localization::Tr("id") / Chars("id") expressions become literal
macros, never runtime calls. The compiler view preserves every source byte
offset and newline: Clang's source annotations and AST-based tools still point
into the authored file. Only generated files contain code page byte escapes.
Both Clang and VC6 see the same literal macros.

    homm1 localization check            validate the catalogs and source text
    homm1 localization update [--check] regenerate locales/messages.pot and
                                        rewrite every .po in template order
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import tempfile
from pathlib import Path


# Re-export the portable catalog API for existing analysis/build callers.
from homm1.graph.catalog import (Catalog, tokens, quoted, literal, parse_po,  # noqa: F401
                                 format_signature, hidden_text_errors, LOCALES, TEMPLATE)


def _write_generated(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = text.encode('utf-8')
    if path.is_file() and path.read_bytes() == payload:
        return
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as stream:
        stream.write(payload)
        temporary = stream.name
    os.replace(temporary, path)


def available_locales(repo):
    """Every language with a descriptor (locales/<code>.json)."""
    return sorted(path.stem for path in (Path(repo) / LOCALES).glob('*.json'))


def matching_locale(repo):
    """The pinned target owns the matching locale, never an environment variable."""
    from homm1.graph import catalog
    locale = catalog.matching_locale(repo) or 'ru'
    if (Path(repo) / LOCALES).is_dir() and locale not in available_locales(repo):
        raise ValueError(f'the matching locale {locale} has no locales/{locale}.json')
    return locale


def output_dir(repo, locale):
    """Generated localization files: the matching language under build/, any
    other language beside its nonmatching objects (build/ordinary/<locale>)."""
    repo = Path(repo)
    if locale == matching_locale(repo):
        return repo / 'build/localization'
    return repo / 'build/ordinary' / locale / 'localization'


def check_locale(repo, locale, out, kind):
    """The ToolError message for an unknown or misplaced `locale`, else None."""
    repo = Path(repo)
    if locale not in available_locales(repo):
        return f'unsupported locale: {locale} (have {", ".join(available_locales(repo))})'
    if locale != matching_locale(repo) and \
            not Path(out).resolve().is_relative_to(repo / 'build/ordinary' / locale):
        return f'nonmatching {locale} {kind} must live under build/ordinary/{locale}'
    return None


def dependencies(repo):
    """Every catalog input of a localized compile."""
    root = Path(repo) / LOCALES
    return [*sorted(p for p in root.iterdir() if p.suffix in ('.pot', '.po', '.json')),
            Path(__file__), Path(__file__).with_name('catalog.py')]


def prepare(repo, source, *, locale=None):
    """Return (generated source, literal header, Clang VFS overlay, dependencies).

    Content-addressed files are safe for concurrent TU builds and probes. The
    overlay keeps authored paths AND offsets in libclang, including headers.
    """
    repo, source = Path(repo).resolve(), Path(source).resolve()
    if not (repo / LOCALES / TEMPLATE).is_file():
        return source, None, None, []  # Small standalone tool-test fixtures.
    from homm1.graph.scan import Scanner
    scanner = Scanner(repo)
    locale = locale or matching_locale(repo)
    catalog = Catalog.load(repo)
    generated = output_dir(repo, locale)
    texts = {path: path.read_text(encoding='utf-8')
             for path in [source, *(repo / p for p in scanner.headers(str(source)))]}
    character_keys = {key for text in texts.values()
                      for _, _, key, method in catalog.typed_calls(text)
                      if method == 'Chars'}
    header_text = catalog.header(locale, character_keys=character_keys)
    digest = hashlib.sha256(header_text.encode()).hexdigest()
    header = generated / (digest + '.h')
    _write_generated(header, header_text)
    roots, compiled = [], source
    views = {}
    for path, text in texts.items():
        rendered = catalog.render(text, locale=locale)
        views[path] = rendered
        if rendered == text:
            continue
        target = generated / hashlib.sha256((str(path) + rendered).encode()).hexdigest() / path.name
        _write_generated(target, rendered)
        roots.append({'type': 'file', 'name': str(path), 'external-contents': str(target)})
        if path == source:
            compiled = target
    if any(Path(entry['name']) != source for entry in roots):
        # VC6 has no VFS support. A self-contained reachable-header mirror also
        # handles a quoted sibling include reached through an unchanged header.
        fingerprint = json.dumps([(str(p), value) for p, value in sorted(views.items())])
        mirror = generated / hashlib.sha256((header_text + fingerprint).encode()).hexdigest()
        header = mirror / 'messages.h'
        _write_generated(header, header_text)
        for path, rendered in views.items():
            relative = path.relative_to(repo) if path.is_relative_to(repo) else Path('probe') / path.name
            target = mirror / relative
            _write_generated(target, rendered)
            if path == source:
                compiled = target
    overlay_text = json.dumps({'version': 0, 'use-external-names': False, 'roots': roots})
    overlay = generated / (hashlib.sha256(overlay_text.encode()).hexdigest() + '.json')
    _write_generated(overlay, overlay_text)
    return compiled, header, overlay, dependencies(repo)


def clang_args(repo, source, *, locale=None):
    _, header, overlay, _ = prepare(repo, source, locale=locale)
    return ['-Xclang', '-ivfsoverlay', '-Xclang', str(overlay),
            '-Xclang', '-include', '-Xclang', str(header)] if header else []


def check_formats(repo, source, *, locale=None):
    """Check the expanded literals against actual call arguments with Clang."""
    import subprocess
    from homm1.tool import clang
    flags = clang.compdb().get(str(Path(source).resolve()))
    if flags is None:
        flags = [*clang.MS_FLAGS, *clang.inc_gcc()]
    else:
        flags = ['--driver-mode=cl', *flags]
    result = subprocess.run(
        [clang._clang(), *flags, *clang_args(repo, source, locale=locale),
         '-Wformat=2', '-Werror=format', '-Wno-error=format-security',
         '-Wno-error=format-nonliteral', '-fsyntax-only', str(source)],
        capture_output=True, text=True)
    return [result.stderr] if result.returncode else []


def check(repo):
    """Every localization error of the checkout (an empty list passes)."""
    from homm1.graph import catalog
    try:
        reference = matching_locale(repo)
    except ValueError as exc:
        return [str(exc)]
    return catalog.check(repo, reference)


from homm1.core.usage import logged


@logged
def main(argv=None):
    from homm1.core.paths import REPO
    from homm1.graph import catalog
    parser = argparse.ArgumentParser(prog='homm1 localization', description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', nargs='?', choices=('check', 'update'), default='check')
    parser.add_argument('--root', type=Path, default=REPO)
    parser.add_argument('--check', action='store_true',
                        help='update: change nothing; fail when a file is out of date')
    args = parser.parse_args(argv)
    try:
        reference = matching_locale(args.root)
    except ValueError as exc:
        print(f'[localization] {exc}')
        return 1
    return catalog.run(args.command, args.root, reference, check_only=args.check)


if __name__ == '__main__':
    raise SystemExit(main())
