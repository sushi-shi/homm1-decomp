"""Measured compiler contracts, independent of game-wide flag attribution."""
import json

from homm1 import analysis, toolchain, labels
from homm1.core.compiler import compile_source
from homm1.core.coff import CoffObject
from homm1.core.inputs import REPO
from homm1.core.matching import confirm_object, function_payload


def run(args):
    source = REPO / 'tests/fixtures/compiler.cpp'
    reports = []
    for compiler in args.ids:
        toolchain.verify(compiler)
        analysis.run(source, compiler)
        analysis.run(source, compiler, strict=True)
        flags = ['/nologo', '/c', '/Od', '/GX', '/Z7']
        output = REPO / f'build/probes/{compiler}/contracts.obj'
        compile_source(source, output, flags, compiler)
        obj = CoffObject(output.read_bytes())
        claims = labels.definitions(source, compiler, flags)
        confirm_object(obj, claims)
        plain_path = output.with_name('contracts-no-debug.obj')
        compile_source(source, plain_path, [flag for flag in flags if flag != '/Z7'], compiler)
        plain = CoffObject(plain_path.read_bytes())
        confirm_object(plain, claims)
        for claim in claims:
            if function_payload(obj, claim.symbol, claims)[1] != function_payload(plain, claim.symbol, claims)[1]:
                raise ValueError(f'{compiler}: /Z7 changes code for {claim.symbol}; debug extents cannot be used')
        defined = {s.name for s in obj.symbols.values() if s.section > 0}
        required = {'_probe_stdcall@8', '_probe_cdecl', '??0Probe@@QAE@XZ',
                    '??1Probe@@QAE@XZ', '?value@Probe@@UAEHH@Z', '?twice@Probe@@SAHH@Z',
                    '?overload@Probe@@QAEHH@Z', '?overload@Probe@@QAEHF@Z'}
        if not required <= defined:
            raise ValueError(f'{compiler}: ABI probe missing symbols: {sorted(required - defined)}')
        reports.append(dict(compiler=compiler, flags=flags, source_sha256=toolchain.digest(source),
                            object_sha256=toolchain.digest(output), symbols=sorted(defined),
                            source_bindings_confirmed=len(claims), debug_code_identical=True,
                            note='Compiler fixtures only; not a game-wide profile attribution'))
    destination = REPO / 'build/probes/contracts.json'
    destination.write_text(json.dumps(reports, indent=2) + '\n')
    print(f'Compiler/enum/layout contracts passed for {", ".join(args.ids)}')
