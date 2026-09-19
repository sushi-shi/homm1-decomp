"""Campaign inspection: HoMM3-style semantic context and HoMM2 frame evidence."""
from dataclasses import asdict
import json

from homm1 import build, checkpoint, model
from homm1.core.disasm import instructions, code_instructions, frame
from homm1.core import manifest
from homm1.core.inputs import REPO


def measured(unit):
    errors = []
    for path in (REPO / f'build/unit-reports/{unit}.json', REPO / 'build/match-report.json'):
        if not path.exists():
            continue
        try:
            return checkpoint.fresh_report(path)
        except ValueError as exc:
            errors.append(str(exc))
    raise ValueError('; '.join(errors) or f'no measured report for {unit}; run homm1 build --unit {unit}')


def render(instruction, origin):
    return dict(address=f'0x{instruction.address:08X}', offset=instruction.address - origin,
                bytes=instruction.bytes.hex(), instruction=instruction.mnemonic + ' ' + instruction.op_str)


def command(args):
    image = build.image()
    claims, refs = model.resolve(image)
    try:
        rva = int(args.address, 0)
        if rva >= image.image_base:
            rva -= image.image_base
    except ValueError:
        matches = [c for c in claims if args.address in c.symbol]
        if len(matches) != 1:
            raise ValueError('name lookup must identify exactly one claimed function')
        rva = matches[0].rva
    owner = next((c for c in claims if c.rva <= rva < c.rva + c.size), None)
    if args.action == 'rva':
        kinds = manifest.check_retail(image)
        value = dict(rva=hex(rva), va=hex(image.image_base + rva),
                     located=kinds.get(rva, 'not catalogued'),
                     owner=asdict(owner) if owner else None,
                     references=[dict(function_rva=hex(start), **ref) for start, rows in refs.items()
                                 for ref in rows if ref['target_rva'] == rva])
    elif args.action == 'xref':
        value = dict(scope='reviewed references in admitted code; not a whole-image call graph',
                     incoming=[dict(function_rva=hex(start), **ref) for start, rows in refs.items()
                               for ref in rows if ref['target_rva'] == rva],
                     outgoing=refs.get(owner.rva if owner else rva, []))
    elif args.action == 'strings':
        section = image.section_of(rva)
        if not section:
            raise ValueError('address is not mapped')
        data = image.read(rva, min(args.size or 256, section.rva + section.size - rva))
        value = dict(rva=hex(rva), ascii=data.split(b'\0')[0].decode('ascii', errors='replace'))
    elif not owner:
        if args.action != 'disasm' or not args.size or args.side != 'retail':
            raise ValueError('unclaimed code has unknown extent; use sema disasm ADDRESS --size SIZE')
        value = [render(i, image.image_base + rva) for i in instructions(image.read(rva, args.size), image.image_base + rva)]
    elif args.action == 'source':
        from pathlib import Path
        value = dict(claim=asdict(owner), source=Path(owner.source).read_text())
    else:
        origin = image.image_base + owner.rva
        retail = image.read(owner.rva, owner.size)
        rows, _ = code_instructions(image, owner)
        target_rows = [dict(address=f'0x{i.address + image.image_base:08X}', offset=i.address - owner.rva,
                            bytes=i.bytes.hex(), instruction=i.mnemonic + ' ' + i.op_str) for i in rows]
        if args.action == 'disasm' and args.side == 'retail':
            value = target_rows
        else:
            report = measured(owner.unit)
            fn = next(f for f in report['functions'] if f['rva'] == owner.rva)
            code = bytes.fromhex(fn['resolved_hex'])
            compiled_rows = [render(i, origin) for i in instructions(code, origin)]
            if args.action == 'disasm':
                value = compiled_rows
            elif args.action == 'frame':
                value = dict(retail=frame(retail), compiled=frame(code),
                             note='Diagnostic only; no stack-slot predictor assumed for VC4')
            else:
                first = fn['differing_offsets'][0] if fn['differing_offsets'] else None
                def context(rows):
                    index = next((n for n, row in enumerate(rows) if first is not None and
                                  row['offset'] + len(row['bytes']) // 2 > first), len(rows) - 1)
                    return rows[max(0, index - 3):index + 4]
                value = dict(rva=hex(owner.rva), symbol=owner.symbol, exact=fn['exact'], score=fn['score'],
                             first_differing_offset=first, retail_size=owner.size, compiled_size=len(code),
                             retail=context(target_rows), compiled=context(compiled_rows),
                             expected_relocations=fn['expected_relocations'], actual_relocations=fn['actual_relocations'])
    print(json.dumps(value, indent=2))
