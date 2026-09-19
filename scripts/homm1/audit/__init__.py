"""Buka 2.1 and Gruntz source audits; see docs/tooling-ports.md for adaptations."""


def command(args):
    from homm1.audit import casts, readability, compiler_artifacts
    if args.audit == 'compiler-artifacts':
        return compiler_artifacts.main(args.arguments)
    if args.audit == 'casts':
        if '--portable' not in args.arguments and '--help' not in args.arguments:
            from homm1 import analysis, build
            config, entries = build.units()
            analysis.compilation_databases(config, entries)
        return casts.main(args.arguments)
    return readability.main(args.arguments)
