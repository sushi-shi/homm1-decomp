"""Empty-string identity needs a real reference, not merely zero-filled storage."""
import struct
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from homm1.delink.data_manifest import fp_pool_rows


def empty_literal_object():
    """Real i386 COFF: _Use pushes $SG1; adjacent $SG2 is unreferenced."""
    code = b'\x68\0\0\0\0\xc3'
    raw = 20 + 2 * 40
    rel = raw + len(code)
    symbols = rel + 10
    header = struct.pack('<HHIIIHH', 0x14c, 2, 0, symbols, 3, 0, 0)
    text = struct.pack('<8sIIIIIIHHI', b'.text', 0, 0, len(code), raw,
                       rel, 0, 1, 0, 0x60500020)
    bss = struct.pack('<8sIIIIIIHHI', b'.bss', 0, 0, 5, 0,
                      0, 0, 0, 0, 0xc0300080)
    relocation = struct.pack('<IIH', 1, 1, 6)
    table = b''.join(struct.pack('<8sIhHBB', name, value, section, kind, storage, 0)
                     for name, value, section, kind, storage in [
                         (b'_Use', 0, 1, 0x20, 2),
                         (b'$SG1', 0, 2, 0, 3),
                         (b'$SG2', 4, 2, 0, 3)])
    return header + text + bss + code + relocation + table + struct.pack('<I', 4)


def two_function_object(prefix=b'', *, static_next=False):
    """Two real functions; each uses its own distinct empty BSS literal."""
    first = prefix + b'\x68\0\0\0\0\xc3'
    second = b'\x68\0\0\0\0\xc3'
    code = first + second
    raw = 20 + 2 * 40
    rel = raw + len(code)
    symbols = rel + 20
    header = struct.pack('<HHIIIHH', 0x14c, 2, 0, symbols, 4, 0, 0)
    text = struct.pack('<8sIIIIIIHHI', b'.text', 0, 0, len(code), raw,
                       rel, 0, 2, 0, 0x60500020)
    bss = struct.pack('<8sIIIIIIHHI', b'.bss', 0, 0, 5, 0,
                      0, 0, 0, 0, 0xc0300080)
    relocations = struct.pack('<IIH', len(prefix) + 1, 2, 6)
    relocations += struct.pack('<IIH', len(first) + 1, 3, 6)
    table = b''.join(struct.pack('<8sIhHBB', name, value, section, kind, storage, 0)
                     for name, value, section, kind, storage in [
                         (b'_Use', 0, 1, 0x20, 2),
                         (b'_Next', len(first), 1, 0x20, 3 if static_next else 2),
                         (b'$SG1', 0, 2, 0, 3), (b'$SG2', 4, 2, 0, 3)])
    return header + text + bss + code + relocations + table + struct.pack('<I', 4)


class BssLiteralTests(unittest.TestCase):
    def run_case(self, *, referenced=True, payload=b'\0', storage='data-loader-zero-tail'):
        target = 0x3000
        code = b'\x68' + struct.pack('<I', 0x400000 + target) + b'\xc3'
        image = SimpleNamespace(
            data=code, image_base=0x400000,
            reloc_sites=[0x1001] if referenced else [],
            off=lambda rva: rva - 0x1000 if 0x1000 <= rva < 0x1006 else None,
            pe=SimpleNamespace(read=lambda rva, size:
                               payload if rva == target and size == 1 else None))
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'TEST.obj').write_bytes(empty_literal_object())
            with patch('homm1.delink.data_manifest.retail', return_value=image), \
                    patch('homm1.delink.data_manifest._classify', return_value=storage), \
                    patch('homm1.delink.data_manifest._referrer_maps',
                          return_value=({}, {'_Use': (0x1000, len(code))}, {})):
                return fp_pool_rows(SimpleNamespace(data=[]), root, literal='sg')

    def different_extents(self, candidate, retail_targets):
        code = b''.join(b'\x68' + struct.pack('<I', 0x400000 + target)
                        for target in retail_targets) + b'\xc3'
        image = SimpleNamespace(data=code, image_base=0x400000,
                                reloc_sites=[0x1001 + 5 * i for i in range(len(retail_targets))],
                                off=lambda rva: rva - 0x1000 if 0x1000 <= rva < 0x1000 + len(code) else None,
                                pe=SimpleNamespace(read=lambda rva, size: b'\0' if rva in retail_targets and size == 1 else None))
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'TEST.obj').write_bytes(candidate)
            with patch('homm1.delink.data_manifest.retail', return_value=image), \
                    patch('homm1.delink.data_manifest._classify', return_value='data-loader-zero-tail'), \
                    patch('homm1.delink.data_manifest._referrer_maps',
                          return_value=({}, {'_Use': (0x1000, len(code))}, {})):
                return fp_pool_rows(SimpleNamespace(data=[]), root, literal='sg')

    def test_longer_candidate_uses_its_own_function_boundary(self):
        rows, _ = self.different_extents(two_function_object(b'\x90' * 6), [0x3000])
        self.assertEqual([(r['member'], r['rva']) for r in rows], [('$SG1', 0x3000)])

    def test_shorter_candidate_cannot_borrow_next_function_references(self):
        for static in (False, True):
            with self.subTest(static=static):
                rows, _ = self.different_extents(two_function_object(static_next=static),
                                                 [0x3000, 0x3004])
                self.assertEqual(rows, [])

    def test_relocated_empty_string_is_one_byte_in_bss(self):
        rows, withheld = self.run_case()
        self.assertEqual([(r['member'], r['rva'], r['size'], r['storage']) for r in rows],
                         [('$SG1', 0x3000, 1, 'bss')])
        self.assertTrue(any(member == '$SG2' for _, member, _ in withheld))

    def test_adjacent_zero_bytes_do_not_establish_an_address(self):
        rows, withheld = self.run_case(referenced=False)
        self.assertEqual(rows, [])
        self.assertEqual({member for _, member, _ in withheld}, {'$SG1', '$SG2'})

    def test_nonzero_or_unmapped_payload_is_refused(self):
        for payload in (b'x', None):
            with self.subTest(payload=payload):
                rows, _ = self.run_case(payload=payload)
                self.assertEqual(rows, [])

    def test_coff_bss_resolves_file_alignment_slack(self):
        rows, _ = self.run_case(storage='data-unprovable-tail')
        self.assertEqual([(r['rva'], r['storage']) for r in rows], [(0x3000, 'bss')])

    def test_initialized_storage_is_not_silently_retyped_as_bss(self):
        rows, _ = self.run_case(storage='data-initialized')
        self.assertEqual(rows, [])


if __name__ == '__main__':
    unittest.main()
