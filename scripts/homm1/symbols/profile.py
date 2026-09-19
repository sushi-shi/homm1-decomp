"""HoMM1 inputs for the copied Buka symbol scanners."""
from pathlib import Path

from homm1.clang_options import ClangMode


def _clang_args(repo: Path, source: Path, *, mode: ClangMode) -> list[str]:
    # Production analysis comes from the same manifest/profile as compilation.
    # Standalone donor fixtures have no native compiler installation or manifest.
    if (repo / 'config/units.toml').is_file():
        from homm1 import analysis
        from homm1.core.manifest import load
        config = load(repo / 'config/units.toml')
        unit = next((u for u in config['unit'] if (repo / u['source']).resolve() == source.resolve()), None)
        if unit is None:
            raise ValueError(f'{source}: source is not enrolled in config/units.toml')
        return analysis.arguments(source, config['build']['compiler'], mode == ClangMode.STRICT,
                                  config['flags'][unit['flags']])[1:-1]
    args = ['-x', 'c++', mode.driver_flag, '--target=i386-pc-windows-msvc',
            '-fms-extensions', '-fms-compatibility-version=10.00',
            '-I', str(repo / 'include')]
    vendor = repo / 'vendor'
    if vendor.is_dir():
        for include in sorted(p for p in vendor.iterdir() if p.is_dir()):
            args.extend(('-I', str(include)))
    return args


def import_specs():
    # Buka's vendor ordinal table is target evidence, not generic tooling.
    # HoMM1 has no reviewed ordinal aliases yet; unknown ordinals fail in donor.
    return ()


def source_unit(path: Path, source_root: Path, repo: Path) -> str:
    """Preserve configured fragment names instead of assuming a source-path TU."""
    manifest = repo / 'config/units.toml'
    if manifest.is_file():
        from homm1.core.manifest import load
        matches = [u['unit'] for u in load(manifest)['unit']
                   if (repo / u['source']).resolve() == path.resolve()]
        if len(matches) > 1:
            raise ValueError(f'{path}: multiple configured owners')
        if matches:
            return matches[0]
    return path.relative_to(source_root.resolve()).with_suffix('').as_posix()
