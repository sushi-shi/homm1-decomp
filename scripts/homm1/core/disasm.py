"""Instruction boundaries and operands shared by validation and diagnostics."""


def instructions(code, address):
    try:
        from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    except ImportError:
        raise ValueError('Capstone is required; enter nix develop') from None
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    result = list(decoder.disasm(bytes(code), address))
    if sum(i.size for i in result) != len(code):
        raise ValueError(f'undecodable code at {address + sum(i.size for i in result):#x}; review code/table boundaries')
    return result


def code_instructions(image, claim, retail=None):
    from homm1.core import manifest
    path = (retail or manifest.CONFIG / 'retail') / 'code_data.tsv'
    rows = manifest.table('code_data.tsv', ('function_rva', 'rva', 'size', 'kind', 'provenance'), retail) if path.exists() else []
    tables = sorted((int(r['rva'], 0), int(r['size'], 0), r) for r in rows if int(r['function_rva'], 0) == claim.rva)
    cursor, decoded, fields = claim.rva, [], set()
    for start, size, row in tables:
        if start < cursor or size <= 0 or size % 4 or start + size > claim.rva + claim.size or row['kind'] not in ('jump_table', 'eh_table') or not row['provenance']:
            raise ValueError('invalid or overlapping embedded code-data extent')
        decoded.extend(instructions(image.read(cursor, start - cursor), cursor))
        fields.update(range(start, start + size, 4))
        cursor = start + size
    decoded.extend(instructions(image.read(cursor, claim.rva + claim.size - cursor), cursor))
    return decoded, fields


def frame(code):
    rows = instructions(code, 0)
    saved, locals_, allocation = [], [], 0
    for i in rows:
        if i.mnemonic == 'sub' and i.op_str.startswith('esp, '):
            allocation = int(i.op_str.split(', ')[1], 0)
            break
    for i in rows:
        if i.mnemonic == 'push' and i.op_str in ('ebx', 'esi', 'edi'):
            saved.append(i.op_str)
        for operand in i.operands:
            if operand.type == 3 and i.reg_name(operand.mem.base) == 'ebp' and operand.mem.disp < 0:
                locals_.append(operand.mem.disp)
    return dict(frame_pointer=len(rows) > 1 and rows[0].mnemonic == 'push' and rows[0].op_str == 'ebp',
                allocation=allocation, saved_registers=saved, local_accesses=locals_)
