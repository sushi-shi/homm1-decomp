"""Opt-in clean-checkout acceptance test. Run inside nix develop .#build.

Uses only committed project files and locally verified retail/compiler inputs.
The /O2 experiment edits the temporary checkout, never the working campaign.
"""
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    archive = subprocess.check_output(['git', 'archive', revision], cwd=ROOT)
    log_path = ROOT / 'build/campaign-smoke.log'
    result = dict(revision=revision)
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
        run('test')
        run('check')
        run('build')
        run('verify', 'check', '--tier', 'full')
        report = checkout / 'build/match-report.json'
        functions = json.loads(report.read_text())['functions']
        assert len(functions) == 3 and all(f['exact'] for f in functions)
        result['unoptimized_exact'] = len(functions)
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
        result['restored_exact'] = 3
    (ROOT / 'build/campaign-smoke.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
