"""Always-on source checks, adapted from Gruntz's verify/board/casts doctrine.

Text checks include inactive code. Semantic checks are supplied by analysis;
human review is never inferred from a byte score or a successful compiler run.
"""
from collections import Counter
import hashlib
import json
import re
import tomllib

from homm1.core.inputs import REPO


def blank(text):
    """Blank comments and literals without moving offsets or line numbers."""
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text, flags=re.S)


def fingerprint(text):
    from homm1.labels import source_hash
    return source_hash(text)


# Hard bans have no exception mechanism. Scoped debt is deliberately separate.
HARD = {
    'assembly': r'\b(?:__asm|_asm|asm)\b|\b__declspec\s*\(\s*naked\s*\)',
    'manual-vtable': r'\b(?:struct|class)\s+\w*Vtbl\b|->\s*vtbl\b|\b(?:m_vtbl|m_vptr|g_\w*Vtbl)\b',
    'fake-view': r'\b(?:struct|class)\s+\w*(?:FakeView|OffsetView|LayoutView)\b',
    'codegen-directive': r'^\s*#\s*pragma\s+(?:optimize|code_seg|function|intrinsic|inline_depth|inline_recursion)\b',
    'codegen-artifact': r'\b(?:FORCE_EMIT|FORCE_COMDAT|REGISTER_PADDING|STACK_PADDING)\b',
}
DEBT = {
    'provisional-name': r'\b(?:RetailService_[0-9A-Fa-f]+|(?:FUN|sub|g)_[0-9A-Fa-f]{6,}|Unknown\w*|MethodStub\w*)\b',
    'reinterpret-cast': r'\breinterpret_cast\s*<[^>]+>',
    'volatile': r'\bvolatile\b',
}


def source_files(root):
    return sorted(p for directory in ('src', 'include') for p in (root / directory).rglob('*')
                  if p.suffix in ('.h', '.hpp', '.cpp', '.cc', '.cxx', '.inl'))


def board(root=REPO):
    findings, sites = [], []
    for path in source_files(root):
        source = path.read_text()
        text = blank(source)
        relative = str(path.relative_to(root))
        for rules, hard in ((HARD, True), (DEBT, False)):
            for rule, pattern in rules.items():
                for match in re.finditer(pattern, text, re.M):
                    line = text.count('\n', 0, match.start()) + 1
                    site = dict(path=relative, line=line, rule=rule,
                                fingerprint=fingerprint(source.splitlines()[line - 1]))
                    if hard:
                        findings.append(f'{relative}:{line}: forbidden {rule}')
                    else:
                        sites.append(site)
        if path.suffix in ('.cpp', '.cc', '.cxx'):
            for line, code in enumerate(text.splitlines(), 1):
                if re.match(r'\s*extern\b.*;\s*$', code):
                    findings.append(f'{relative}:{line}: declaration belongs in a canonical header')
                if re.match(r'\s*(?:(?:typedef|enum)\b|H1_ENUM_BEGIN\s*\()', code):
                    findings.append(f'{relative}:{line}: shared type/domain belongs in a header')
        includes = re.findall(r'^\s*#\s*include\s*([<"][^>"\n]+[>"])', source, re.M)
        if len(includes) != len(set(includes)):
            findings.append(f'{relative}: duplicate include')
        if relative not in ('include/match.h', 'include/Domains.h') and re.search(
                r'^\s*#\s*(?:if|ifdef|ifndef|elif)\b[^\n]*(?:__clang__|_MSC_VER|__cplusplus|H1_RETAIL_COMPILER)', text, re.M):
            findings.append(f'{relative}: compiler-specific source behavior outside approved annotation/domain headers')
    ledger_path = root / 'config/cleanliness/debt.toml'
    ledger = tomllib.loads(ledger_path.read_text()).get('debt', []) if ledger_path.exists() else []
    keys = lambda row: (row['path'], row['rule'], row['fingerprint'])
    expected = Counter(keys(row) for row in ledger)
    actual = Counter(keys(row) for row in sites)
    for row in ledger:
        if row['rule'] not in DEBT or not row.get('evidence', '').strip():
            findings.append(f'invalid debt entry: {row}')
    for site in sites:
        key = keys(site)
        if expected[key]:
            expected[key] -= 1
        else:
            findings.append(f'{site["path"]}:{site["line"]}: unregistered {site["rule"]}')
    for key, count in expected.items():
        if count:
            findings.append(f'stale debt entry ({count}): {key}')
    return dict(findings=findings, debt=dict(Counter(s['rule'] for s in sites)),
                source_files=len(source_files(root)), debt_sites=sites,
                readability='not certified; human review is separate',
                semantic_checks='not measured by the text tier')


def check(root=REPO):
    report = board(root)
    if report['findings']:
        raise ValueError('cleanliness failed:\n' + '\n'.join(report['findings']))
    return report


def check_reviews(claims, root=REPO):
    path = root / 'config/cleanliness/reviews.toml'
    rows = tomllib.loads(path.read_text()).get('review', []) if path.exists() else []
    owned = {c.rva: c for c in claims if c.parent is None}
    seen = set()
    for row in rows:
        rva = int(row['rva'], 0)
        if rva in seen or rva not in owned or row['src_hash'] != owned[rva].src_hash:
            raise ValueError(f'stale or duplicate source review for {rva:#x}; re-review changed source')
        if not row.get('reviewer') or not row.get('evidence'):
            raise ValueError('source review requires reviewer and evidence')
        seen.add(rva)
    return dict(reviewed=sorted(seen), pending=sorted(owned.keys() - seen))


def check_incomplete_types(tree, root=REPO):
    """A method-only class declaration is not permission to invent its layout."""
    from homm1.labels import walk
    path = root / 'config/cleanliness/types.toml'
    rows = tomllib.loads(path.read_text()).get('incomplete_type', []) if path.exists() else []
    for row in rows:
        name = row['name']
        if not re.fullmatch(r'[A-Za-z_]\w*', name) or not row.get('evidence'):
            raise ValueError('invalid incomplete-type debt')
        for node in walk(tree):
            kind = node.get('kind')
            typ = node.get('argType', node.get('type', {}))
            typ = typ.get('desugaredQualType', typ.get('qualType', ''))
            mentions = re.search(r'\b' + re.escape(name) + r'\b', typ)
            by_value = mentions and '*' not in typ and '&' not in typ
            if by_value and kind in ('VarDecl', 'ParmVarDecl', 'FieldDecl', 'CXXConstructExpr',
                                     'CXXTemporaryObjectExpr', 'UnaryExprOrTypeTraitExpr', 'ArraySubscriptExpr'):
                raise ValueError(f'{name}: {kind} requires an unrecovered class layout')
            if mentions and kind in ('CXXNewExpr', 'CXXDeleteExpr', 'BinaryOperator', 'UnaryOperator'):
                raise ValueError(f'{name}: allocation/pointer arithmetic requires a recovered layout')
            if kind == 'CXXDeleteExpr' and any(re.search(r'\b' + re.escape(name) + r'\b', n.get('type', {}).get('qualType', '')) for n in walk(node)):
                raise ValueError(f'{name}: deletion requires a recovered layout/destructor')


def command(args):
    result = board()
    if args.action == 'check' and args.tier != 'fast':
        from homm1 import analysis, build
        config, entries = build.units()
        claims = build.validate_claims(build.image())
        result['readability'] = check_reviews(claims)
        result['semantic_checks'] = analysis.check(config, entries)
        if args.tier == 'full':
            from homm1.checkpoint import fresh_report
            report = fresh_report()
            if not report.get('complete'):
                raise ValueError('full verification requires a complete build report')
            result['binary_checks'] = dict(functions=len(report['functions']),
                                            exact=sum(f['exact'] for f in report['functions']),
                                            status='validated against original retail')
    print(json.dumps(result, indent=2))
    return bool(result['findings'])
