"""Source-authoritative bindings, following Gruntz retail_labels and HoMM2 ASTs.

Annotations are read from the AST node they decorate, never from the next
textual declaration. The period object subsequently confirms the spelling.
"""
from dataclasses import asdict
import hashlib
import json
from pathlib import Path
import re

from homm1 import analysis
from homm1.symbols.source_symbols import symbols_for_file
from homm1.core.inputs import REPO
from homm1.core.matching import Claim
from homm1.core.cpp_tokens import fingerprint as source_hash

FUNCTIONS = {'FunctionDecl', 'CXXMethodDecl', 'CXXConstructorDecl', 'CXXDestructorDecl', 'CXXConversionDecl'}


def walk(node):
    yield node
    for child in node.get('inner', []):
        yield from walk(child)


def location(value):
    return value.get('expansionLoc', value)


def definitions(source, compiler='vc40', flags=(), declarations=None):
    source = Path(source).resolve()
    tree = analysis.run(source, compiler, ast=True, flags=flags)
    from homm1.verify import check_incomplete_types, check_incomplete_declarations
    check_incomplete_types(tree)
    # Reviews include the compilation context, independently of the function
    # token hash used by the observational MAX score. Header edits are
    # conservatively review-affecting, including macro/preprocessor changes.
    context = hashlib.sha256()
    context.update(json.dumps([compiler, analysis.clang_flags(flags)]).encode())
    for dependency in analysis.dependencies(source, compiler, flags):
        if dependency != source:
            context.update(dependency.name.encode() + b'\0' + dependency.read_bytes() + b'\0')
    declarations_text = source.read_bytes()
    excluded = []
    for n in walk(tree):
        if n.get('kind') in FUNCTIONS:
            loc = location(n.get('loc', {}))
            if loc.get('includedFrom') or Path(loc.get('file', source)).resolve() != source:
                continue
            b = next((c for c in n.get('inner', []) if c.get('kind') in ('CompoundStmt', 'CXXTryStmt')), None)
            if b:
                start, end = location(b['range']['begin']), location(b['range']['end'])
                if 'offset' in start and 'offset' in end:
                    excluded.append((start['offset'], end['offset'] + end.get('tokLen', 1)))
    for start, end in sorted(excluded, reverse=True):
        declarations_text = declarations_text[:start] + b'{}' + declarations_text[end:]
    context.update(declarations_text)
    context_hash = context.hexdigest()
    if declarations is not None:
        for node in walk(tree):
            if node.get('kind') not in FUNCTIONS and not (node.get('kind') == 'VarDecl' and node.get('storageClass') == 'extern'):
                continue
            symbol = node.get('mangledName')
            if not symbol or node.get('storageClass') == 'static':
                continue
            typ = node.get('type', {})
            signature = typ.get('desugaredQualType', typ.get('qualType'))
            if symbol in declarations and declarations[symbol] != signature:
                raise ValueError(f'conflicting source declarations for {symbol}: {declarations[symbol]} / {signature}')
            declarations[symbol] = signature
    # The copied Buka scanner owns annotation decoding and Microsoft names.
    # The JSON AST below supplies existing review spans/layout checks only.
    bindings = {}
    symbols_for_file(source, source.parent, REPO,
                     args=analysis.arguments(source, compiler, flags=flags)[1:-1], bindings=bindings,
                     validate_translation=check_incomplete_declarations)
    results, seen = [], set()
    for node in walk(tree):
        if node.get('kind') not in FUNCTIONS:
            continue
        body = next((c for c in node.get('inner', []) if c.get('kind') in ('CompoundStmt', 'CXXTryStmt')), None)
        if body is None:
            continue
        loc = location(node.get('loc', {}))
        if loc.get('includedFrom') or Path(loc.get('file', source)).resolve() != source:
            continue
        identity = bindings.get(loc.get('offset'))
        if identity is None:
            raise ValueError(f'{source}: function {node.get("name")} has no RVA identity')
        row, linkage = identity
        path, rva, size, symbol = source, row.rva, row.size, row.name
        begin, end = location(node['range']['begin']), location(node['range']['end'])
        if Path(end.get('file', path)).resolve() != path:
            raise ValueError('cross-file macro-generated definitions are not supported')
        snippet = path.read_bytes()[begin['offset']:end['offset'] + end.get('tokLen', 1)].decode()
        if not symbol or (rva, symbol) in seen:
            raise ValueError(f'{source}: duplicate or unnameable source definition')
        seen.add((rva, symbol))
        casts = [n for n in walk(body) if n.get('kind') == 'CStyleCastExpr']
        if casts:
            raise ValueError(f'{path}: {node["name"]}: C-style cast; use an evidenced named conversion')
        results.append(Claim(rva, size, symbol, source=str(path), src_hash=source_hash(snippet),
                             linkage=linkage, context_hash=context_hash))
    # Compiler-generated bodies have explicit ownership, not invented source.
    text = source.read_text()
    from homm1.verify import blank
    for marker in re.finditer(r'\bRVA_COMPGEN\s*\(', blank(text)):
        match = re.match(r'RVA_COMPGEN\s*\(\s*(0x[\da-fA-F]+|\d+)\s*,\s*(0x[\da-fA-F]+|\d+)\s*,\s*"([^"\n]+)"\s*,\s*(0x[\da-fA-F]+|\d+)\s*\)', text[marker.start():])
        if not match:
            raise ValueError('generated annotations require literal RVA, size, symbol and owner')
        rva, size, symbol, owner = match.groups()
        results.append(Claim(int(rva, 0), int(size, 0), symbol, source=str(source), parent=int(owner, 0)))
    parents = {c.rva: c for c in results if c.parent is None}
    for claim in results:
        if claim.parent is not None:
            if claim.parent not in parents:
                raise ValueError(f'generated body {claim.symbol} lacks its source owner')
            object.__setattr__(claim, 'src_hash', parents[claim.parent].src_hash)
            object.__setattr__(claim, 'context_hash', parents[claim.parent].context_hash)
    return results


def command(_args):
    from homm1.build import units
    config, entries = units()
    print(json.dumps([dict(asdict(c), unit=u['unit']) for u in entries for c in definitions(REPO / u['source'], config['build']['compiler'], config['flags'][u['flags']])], indent=2))
