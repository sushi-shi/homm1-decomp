"""Dead-code annotations cover both C++ and reviewed fixed MASM sources."""

import tempfile
import unittest
from pathlib import Path

from homm1.verify import dead_code


class DeadCodeMarkerTests(unittest.TestCase):
    def test_masm_marker_resolves_through_fixed_claim(self):
        with tempfile.TemporaryDirectory() as td:
            source = Path(td) / "BITS.asm"
            source.write_text(
                "; @dead-code\n"
                "; Zero-ref: no effective incoming retail reference.\n"
                "BitClear PROC C\n"
                "    ret\n"
                "BitClear ENDP\n"
            )
            claims = {source.resolve(): {"BitClear": 0x7BB16}}
            marked, sites, problems = dead_code.source_markers(
                files=[source], asm_claims=claims)

        self.assertEqual(problems, [])
        self.assertEqual(set(marked), {0x7BB16})
        self.assertEqual(sites[0x7BB16][1], 3)


if __name__ == "__main__":
    unittest.main()
