"""Opt-in clean-checkout acceptance test. Run inside nix develop .#build.

Uses only committed project files and locally verified retail/compiler inputs.
The /O2 experiment edits the temporary checkout, never the working campaign.
"""
import io
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--worktree', action='store_true', help='export current nonignored files to test changes before committing')
    args = parser.parse_args()
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    if args.worktree:
        names = subprocess.check_output(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=ROOT).decode().split('\0')
        stream = io.BytesIO()
        with tarfile.open(fileobj=stream, mode='w') as tar:
            for name in sorted(set(names) - {''}):
                if (ROOT / name).exists():
                    tar.add(ROOT / name, arcname=name, recursive=False)
        archive = stream.getvalue()
    else:
        archive = subprocess.check_output(['git', 'archive', revision], cwd=ROOT)
    log_path = ROOT / 'build/campaign-smoke.log'
    result = dict(revision=revision, worktree=args.worktree, archive_sha256=hashlib.sha256(archive).hexdigest())
    with tempfile.TemporaryDirectory(prefix='homm1-campaign-') as directory, log_path.open('w') as log:
        checkout = Path(directory)
        with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
            tar.extractall(checkout, filter='data')
        (checkout / 'build').mkdir()
        for name in ('orig', 'toolchains', 'wineprefix'):
            (checkout / 'build' / name).symlink_to(ROOT / 'build' / name, target_is_directory=True)
        environment = dict(os.environ, PYTHONPATH=str(checkout / 'scripts'))
        def run(*args, expected=0):
            print('smoke:', ' '.join(args), flush=True)
            process = subprocess.run([sys.executable, str(checkout / 'homm1'), *args], cwd=checkout,
                                     env=environment, capture_output=True, text=True, timeout=300)
            log.write('$ homm1 ' + ' '.join(args) + '\n' + process.stdout + process.stderr)
            log.flush()
            if (process.returncode == 0) != (expected == 0):
                raise RuntimeError(f'{args}: unexpected exit {process.returncode}\n{process.stdout[-2000:]}\n{process.stderr[-2000:]}')
            return process.stdout
        run('test')
        run('check')
        correspondence = json.loads(run('reference', '--json'))
        assert len(correspondence['correspondences']) == 6
        assert all(row['reference'] == 'homm2_buka_21' for row in correspondence['correspondences'])
        unclaimed = json.loads(run('sema', 'reference', 'KBTickCount'))
        assert unclaimed['correspondences'][0]['rva'] == 0x5DC9B
        result['offline_buka_correspondence'] = True
        run('build')
        run('verify', 'check', '--tier', 'full')
        report = checkout / 'build/match-report.json'
        functions = json.loads(report.read_text())['functions']
        assert len(functions) == 3 and all(f['exact'] for f in functions)
        result['unoptimized_exact'] = len(functions)
        blocks = run('sema', 'blocks', '0x4F640', '--diff', '--lite')
        assert '8 exact, 0 size-only' in blocks, 'VC4 debug markers broke block disassembly'
        run('sema', 'branches', '0x4F640', '--diff')
        callers = run('sema', 'callers', '0x4F640')
        assert '_AppAbout@16' in callers and '?ForcePollSound@@YAXXZ' in callers
        assert json.loads(run('status', 'queue', '--json')) == []
        result['buka_navigation'] = True
        # Gruntz graph contract: no-op runs leave every stage artifact alone;
        # changing a body must not re-delink its independent retail target.
        artifact_dirs = ('build/objdiff/base', 'build/objdiff/target', 'build/gen/claims',
                         'build/gen/bindings', 'build/gen/reports')
        def artifact_times():
            return {str(p.relative_to(checkout)): p.stat().st_mtime_ns
                    for directory in artifact_dirs for p in (checkout / directory).rglob('*')
                    if p.is_file() and '.tmp' not in p.parts}
        before = artifact_times()
        readme = checkout / 'README.md'
        readme_before = readme.read_bytes()
        run('build')
        assert artifact_times() == before, 'no-op build rewrote stage artifacts'
        assert readme.read_bytes() == readme_before
        result['no_op_graph'] = True
        source = checkout / 'src/SOURCE/AppAbout.cpp'
        original_source = source.read_text()
        source.write_text(original_source.replace('return 1;', 'return 1; /* graph control */'))
        ledger = checkout / 'config/match_baseline.tsv'
        ledger_before = ledger.read_bytes()
        run('build', '--unit', 'app_about')
        after = artifact_times()
        for name, timestamp in before.items():
            if 'kb_poll_sound' in name or '/target/' in name or '/bindings/' in name:
                assert after[name] == timestamp, f'unrelated stage rebuilt: {name}'
        assert ledger.read_bytes() == ledger_before and readme.read_bytes() == readme_before
        result['focused_body_edit_preserves_targets_and_other_unit'] = True
        # Token-equivalent edits preserve function reviews, but cannot silently
        # renew Buka's physical file review when banking a full checkpoint.
        report_before = report.read_bytes()
        run('build', expected=1)
        assert report.read_bytes() == report_before
        assert ledger.read_bytes() == ledger_before and readme.read_bytes() == readme_before
        result['stale_physical_review_blocks_publication'] = True
        source.write_text(original_source)
        run('build')
        config = checkout / 'config/units.toml'
        original = config.read_text()
        config.write_text(original.replace('"/Od"', '"/O2"'))
        run('compare', expected=1)
        result['stale_report_rejected'] = True
        run('build')
        run('verify', 'check', '--tier', 'full')
        functions = json.loads(report.read_text())['functions']
        result['optimized_nonexact'] = sum(not f['exact'] for f in functions)
        assert result['optimized_nonexact'] > 0
        assert len(json.loads(run('status', 'queue', '--json'))) == result['optimized_nonexact']
        assert readme.read_bytes() != readme_before, 'README did not reflect nonexact full build'
        run('sema', 'diff', '0x4F640')
        run('sema', 'frame', '0x4F640')
        baseline = checkout / 'config/match_baseline.tsv'
        previous = baseline.read_bytes()
        run('build', '--unit', 'kb_poll_sound')
        assert baseline.read_bytes() == previous
        result['unit_build_preserved_checkpoint'] = True
        config.write_text(original)
        run('build')
        run('verify', 'check', '--tier', 'full')
        assert all(f['exact'] for f in json.loads(report.read_text())['functions'])
        assert readme.read_bytes() == readme_before, 'restored exact state did not restore README'
        result['restored_exact'] = 3
    (ROOT / 'build/campaign-smoke.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
