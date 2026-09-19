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
from homm1.core.inputs import REPO
from homm1.core.matching import Claim

FUNCTIONS = {'FunctionDecl', 'CXXMethodDecl', 'CXXConstructorDecl', 'CXXDestructorDecl', 'CXXConversionDecl'}
TOKEN = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|0[xX][\da-fA-F]+|\d+(?:\.\d+)?|[^\s]', re.S)


def source_hash(text):
    tokens = [m[0] for m in TOKEN.finditer(text) if not m[0].startswith(('//', '/*'))]
    return hashlib.sha256(json.dumps(tokens).encode()).hexdigest()


def walk(node):
    yield node
    for child in node.get('inner', []):
        yield from walk(child)


def location(value):
    return value.get('expansionLoc', value)


def definitions(source, compiler='vc40', flags=(), declarations=None):
    source = Path(source).resolve()
    tree = analysis.run(source, compiler, ast=True, flags=flags)
    from homm1.verify import check_incomplete_types
    check_incomplete_types(tree)
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
    nodes = {n['id']: n for n in walk(tree) if 'id' in n}
    access = {}
    for record in walk(tree):
        if record.get('kind') != 'CXXRecordDecl':
            continue
        current = 'private' if record.get('tagUsed') == 'class' else 'public'
        for member in record.get('inner', []):
            if member.get('kind') == 'AccessSpecDecl':
                current = member['access']
            elif member.get('kind') in FUNCTIONS:
                access[member['id']] = current
    results, seen = [], set()
    for node in walk(tree):
        if node.get('kind') not in FUNCTIONS:
            continue
        body = next((c for c in node.get('inner', []) if c.get('kind') in ('CompoundStmt', 'CXXTryStmt')), None)
        if body is None:
            continue
        attributes = [c for c in node.get('inner', []) if c.get('kind') == 'AnnotateAttr']
        if not attributes:
            # Header inline helpers may not emit code; emitted bodies are checked
            # against COFF ownership later. Main-file bodies must be annotated.
            loc = location(node.get('loc', {}))
            if not loc.get('includedFrom') and Path(loc.get('file', source)) == source:
                raise ValueError(f'{source}: function {node.get("name")} has no RVA identity')
            continue
        matches = []
        for attr in attributes:
            loc = location(attr['range']['begin'])
            path = Path(loc.get('file', source)).resolve()
            text = path.read_text()
            annotation = re.match(r'RVA\s*\(\s*(0[xX][\da-fA-F]+|\d+)\s*,\s*(0[xX][\da-fA-F]+|\d+)\s*\)', text[loc['offset']:])
            if annotation:
                matches.append((path, text, int(annotation[1], 0), int(annotation[2], 0)))
        if len(matches) != 1:
            raise ValueError(f'{source}: {node.get("name")}: expected one literal RVA annotation on the definition')
        path, text, rva, size = matches[0]
        begin, end = location(node['range']['begin']), location(node['range']['end'])
        if Path(end.get('file', path)).resolve() != path:
            raise ValueError('cross-file macro-generated definitions are not supported')
        snippet = text[begin['offset']:end['offset'] + end.get('tokLen', 1)]
        symbol = node.get('mangledName')
        # HoMM2 source_symbols documents Clang's destructor-definition alias.
        # VC4 probes confirm the user body is ??1, not the ??_D vbase helper.
        if node['kind'] == 'CXXDestructorDecl' and symbol and symbol.startswith('??_D'):
            declaration = node
            while declaration.get('previousDecl') in nodes:
                declaration = nodes[declaration['previousDecl']]
            visibility = access.get(declaration['id'], 'public')
            letter = {'public': ('Q', 'U'), 'protected': ('I', 'M'), 'private': ('A', 'E')}[visibility][bool(declaration.get('virtual'))]
            if not symbol.endswith('@@QAEXXZ'):
                raise ValueError('unsupported destructor mangling; calibrate against the period compiler')
            symbol = '??1' + symbol[4:-len('@@QAEXXZ')] + '@@' + letter + 'AE@XZ'
        if not symbol or (rva, symbol) in seen:
            raise ValueError(f'{source}: duplicate or unnameable source definition')
        seen.add((rva, symbol))
        casts = [n for n in walk(body) if n.get('kind') == 'CStyleCastExpr']
        if casts:
            raise ValueError(f'{path}: {node["name"]}: C-style cast; use an evidenced named conversion')
        results.append(Claim(rva, size, symbol, source=str(path), src_hash=source_hash(snippet)))
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
    return results


def command(_args):
    from homm1.build import units
    config, entries = units()
    print(json.dumps([dict(asdict(c), unit=u['unit']) for u in entries for c in definitions(REPO / u['source'], config['build']['compiler'], config['flags'][u['flags']])], indent=2))
