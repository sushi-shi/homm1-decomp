"""Target-specific correspondence provider using the existing retail TSV reader.

These are navigation records, never source claims or inferred HoMM1 extents.
Donor source verification reads a pinned Git object, not a dirty working file.
"""
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import tomllib

from homm1.core.inputs import REPO
from homm1.core.manifest import table

COLUMNS = ('rva', 'reference', 'symbol', 'donor_va', 'source', 'line', 'blob',
           'confidence', 'evidence', 'differences')


def load(root=REPO, image=None):
    config = tomllib.loads((root / 'config/references.toml').read_text())
    references = config['reference']
    base = config['target']['image_base']
    if image is not None and image.image_base != base:
        raise ValueError('correspondence target image base disagrees with retail')
    priorities = set()
    for key, ref in references.items():
        if (not re.fullmatch(r'[0-9a-f]{40}', ref['revision']) or not ref['name']
                or not ref['branch'] or type(ref['priority']) is not int
                or ref['priority'] < 0 or ref['priority'] in priorities):
            raise ValueError(f'invalid or ambiguous reference priority/pin: {key}')
        priorities.add(ref['priority'])
    if references.get('homm2_buka_21', {}).get('priority') != 0:
        raise ValueError('Buka 2.1 must remain the primary reference')
    located = {int(row['rva'], 0) for row in table('functions.tsv', ('rva', 'kind'), root / 'config/retail')}
    rows, seen = [], set()
    for raw in table('functions_correspondence.tsv', COLUMNS, root / 'config/retail'):
        row = dict(raw, rva=int(raw['rva'], 0), donor_va=int(raw['donor_va'], 0), line=int(raw['line']))
        key = (row['rva'], row['reference'])
        source = PurePosixPath(row['source'])
        evidence = PurePosixPath(row['evidence'].split('#', 1)[0])
        if (key in seen or row['rva'] not in located or row['reference'] not in references
                or row['line'] <= 0 or not 0 < row['donor_va'] < 2**32
                or not re.fullmatch(r'[0-9a-f]{40}', row['blob'])
                or row['confidence'] not in ('reviewed', 'hypothesis')
                or not row['symbol'].strip() or not row['differences'].strip()
                or not source.parts or source.is_absolute() or '..' in source.parts or source.parts[0] != 'src'
                or not evidence.parts or evidence.is_absolute() or '..' in evidence.parts or evidence.parts[0] != 'evidence'
                or not (root / evidence).is_file()):
            raise ValueError(f'invalid, duplicate or orphan correspondence: {raw}')
        seen.add(key)
        rows.append(dict(row, donor=references[row['reference']]))
    return config, sorted(rows, key=lambda row: (row['rva'], row['donor']['priority']))


def lookup(query=None, root=REPO):
    config, rows = load(root)
    if query:
        try:
            address = int(query, 0)
        except ValueError:
            exact = [row for row in rows if row['symbol'].casefold() == query.casefold()]
            found = exact or [row for row in rows if query.casefold() in row['symbol'].casefold()]
            addresses = {row['rva'] for row in found}
            rows = [row for row in rows if row['rva'] in addresses]
        else:
            if address >= config['target']['image_base']:
                address -= config['target']['image_base']
            # Exact starts only: correspondence never invents ownership of gaps.
            rows = [row for row in rows if row['rva'] == address]
    preferred = set()
    for row in rows:
        row['preferred'] = row['rva'] not in preferred
        preferred.add(row['rva'])
    return rows


def verify_checkout(rows, checkout):
    cache = {}
    for row in rows:
        key = (row['donor']['revision'], row['source'])
        if key not in cache:
            cache[key] = subprocess.check_output(['git', '-C', str(checkout), 'show', ':'.join(key)])
        blob = cache[key]
        digest = hashlib.sha1(b'blob ' + str(len(blob)).encode() + b'\0' + blob).hexdigest()
        if digest != row['blob']:
            raise ValueError(f'{row["source"]}: pinned donor blob differs')
        lines = blob.decode().splitlines()
        if row['line'] >= len(lines):
            raise ValueError('donor source line is outside the pinned blob')
        marker = re.fullmatch(r'\s*VA\(\s*(0x[0-9a-fA-F]+)\s*,\s*(?:0x[0-9a-fA-F]+|[0-9]+)\s*\)\s*', lines[row['line'] - 1])
        if not marker or int(marker[1], 0) != row['donor_va']:
            raise ValueError(f'{row["source"]}:{row["line"]}: donor VA annotation disagrees')
        if not re.search(r'(?<![\w:])' + re.escape(row['symbol']) + r'\s*\(', lines[row['line']]):
            raise ValueError(f'{row["source"]}:{row["line"]}: donor declaration disagrees')
    return len(rows)


def command(args):
    rows = lookup(args.query)
    if args.query and not rows:
        raise ValueError(f'no recorded correspondence for {args.query}')
    if args.checkout:
        verify_checkout(rows, args.checkout)
    if args.json:
        print(json.dumps(dict(scope='navigation evidence; not HoMM1 matching claims',
                              donor_verified=bool(args.checkout), correspondences=rows), indent=2))
    else:
        for row in rows:
            ref = row['donor']
            print(f'0x{row["rva"]:08X} {row["symbol"]} [{row["confidence"]}, '
                  f'{"preferred" if row["preferred"] else "supplementary"}]')
            print(f'  {ref["name"]} {ref["revision"]}: {row["source"]}:{row["line"]} '
                  f'VA 0x{row["donor_va"]:08X}')
            print(f'  Evidence: {row["evidence"]}\n  Differences: {row["differences"]}')
        print('Navigation evidence only; no HoMM1 extent or matching claim is inferred.')
        if args.checkout:
            print(f'Verified {len(rows)} records against pinned donor Git objects.')
