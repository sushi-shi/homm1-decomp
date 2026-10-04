"""Static, byte-preserving Buka localization using master's IDs and PO format.

Authored Tr("semantic.id") expressions become literal macros, never runtime calls.
The compiler view preserves every source byte offset and newline: Clang's source
annotations and AST-based tools still point into the authored file. Only generated
files contain CP1251 byte escapes. Both Clang and VC6 see the same literal macros.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import tempfile
from pathlib import Path


# Re-export the portable catalog API for existing analysis/build callers.
from homm1.graph.catalog import (Catalog, tokens, quoted, literal, parse_registry,
                                 parse_po, format_signature, hidden_text_errors)


def _write_generated(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = text.encode('utf-8')
    if path.is_file() and path.read_bytes() == payload:
        return
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as stream:
        stream.write(payload)
        temporary = stream.name
    os.replace(temporary, path)


def matching_locale(repo):
    """The pinned target owns the matching locale, never an environment variable."""
    target = Path(repo) / 'config/retail/targets.json'
    if not target.is_file():
        build = Path(repo) / 'build.json'
        return json.loads(build.read_text()).get('locale', 'ru') if build.is_file() else 'ru'
    locale = json.loads(target.read_text())['game'].get('locale', 'ru')
    if locale not in ('ru', 'en'):
        raise ValueError(f'unsupported matching locale: {locale}')
    return locale


def prepare(repo, source, *, locale=None):
    """Return (generated source, literal header, Clang VFS overlay, dependencies).

    Content-addressed files are safe for concurrent TU builds and probes. The
    overlay keeps authored paths AND offsets in libclang, including headers.
    """
    repo, source = Path(repo).resolve(), Path(source).resolve()
    if not (repo / 'locales/messages.def').is_file():
        return source, None, None, []  # Small standalone tool-test fixtures.
    from homm1.graph.scan import Scanner
    scanner = Scanner(repo)
    locale = locale or matching_locale(repo)
    catalog = Catalog.load(repo)
    generated = repo / ('build/localization' if locale == 'ru' else 'build/ordinary/en/localization')
    texts = {path: path.read_text(encoding='utf-8')
             for path in [source, *(repo / p for p in scanner.headers(str(source)))]}
    character_keys = {key for text in texts.values()
                      for _, _, key, method in catalog.typed_calls(text)
                      if method == 'Chars'}
    header_text = catalog.header(locale, character_keys=character_keys)
    digest = hashlib.sha256(header_text.encode()).hexdigest()
    header = generated / (digest + '.h')
    _write_generated(header, header_text)
    dependencies = [repo / 'locales/messages.def', repo / 'locales/ru.po', Path(__file__),
                    Path(__file__).with_name('catalog.py')]
    if (repo / 'locales/format-variants.json').is_file():
        dependencies.append(repo / 'locales/format-variants.json')
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
    return compiled, header, overlay, dependencies


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


def check_tree(repo):
    catalog = Catalog.load(repo)
    errors, used = [], set()
    for directory in ('src', 'include'):
        for path in sorted((Path(repo) / directory).rglob('*')):
            if path.suffix not in ('.cpp', '.h', '.c', '.hpp', '.inc', '.rc'):
                continue
            text = path.read_text(encoding='utf-8')
            errors.extend(f'{path}:{line}: {message}' for line, message in hidden_text_errors(text))
            try:
                used.update(key for _, _, key in catalog.calls(text))
            except ValueError as exc:
                errors.append(f'{path}: {exc}')
    return errors, used


from homm1.core.usage import logged


@logged
def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path.cwd())
    args = parser.parse_args(argv)
    try:
        errors, used = check_tree(args.root)
    except (ValueError, UnicodeError) as exc:
        errors, used = [str(exc)], set()
    for error in errors:
        print(error)
    print(f'[localization] {len(used)} used IDs; {len(errors)} errors')
    return int(bool(errors))


if __name__ == '__main__':
    raise SystemExit(main())
