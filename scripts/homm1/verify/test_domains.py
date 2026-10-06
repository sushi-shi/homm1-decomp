"""include/Domains.h and H1/Ints.h: typed domains, arrays and flags are
strict-only; the retail view stays plain."""

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from homm1.core.paths import REPO

_SOURCE = """
#include <match.h>
#include <Domains.h>
H1_ENUM_BEGIN(Slot)
    SLOT_A = 0,
    SLOT_B = 1,
    SLOT_COUNT = 2
H1_ENUM_END(Slot)
H1_ENUM_STEPPED(Slot)
H1_ENUM_BEGIN(Other)
    OTHER_A = 0
H1_ENUM_END(Other)
H1_ENUM_ID_BEGIN(Control)
    CONTROL_OK = 1
H1_ENUM_ID_END(Control)
H1_ENUM_FLAGS_BEGIN(Bits, short)
    BIT_A = 1,
    BIT_B = 2
H1_ENUM_FLAGS_END(Bits)
#include <H1/Ints.h>
H1_ENUM_ARRAY(int, gTable, Slot, SLOT_COUNT) = {1, 2};
H1_ENUM_ARRAY2(short, gGrid, Slot, SLOT_COUNT, Slot, SLOT_COUNT);
struct Record {
    H1_ENUM_STORAGE(Slot, char) slot;
    H1_ENUM_STORAGE(Bits, short) bits;
    b8 flag;
};
void Use(int*);
int Read(Record& record) {
#if PROBE == 0
    Use(gTable);
    Use(gTable + SLOT_B);
    for (H1_ENUM_LOCAL(Slot, int) i = SLOT_A; i < SLOT_COUNT; i++)
        gTable[i] = 0;
    gGrid[SLOT_A][SLOT_B] = 3;
    record.bits = BIT_A | BIT_B;
    record.bits |= BIT_A;
    record.slot++;
    record.flag = true;
    b32 wide = record.flag;
    record.flag = wide;
    if (record.flag == false || !record.flag || wide == record.flag)
        return CONTROL_OK;
    return gTable[record.slot];
#elif PROBE == 1
    return gTable[1];
#elif PROBE == 2
    return gTable[OTHER_A];
#elif PROBE == 3
    return *(gTable + 1);
#elif PROBE == 4
    record.flag = 1;
    return 0;
#else
    return record.flag == 1;
#endif
}
"""


def _clang() -> str | None:
    return os.environ.get("HOMM1_CLANG") or shutil.which("clang++") or shutil.which("clang")


@unittest.skipUnless(_clang(), "clang is not on PATH")
class EnumArrayTest(unittest.TestCase):
    def _compile(self, std: str, probe: int, *extra: str) -> subprocess.CompletedProcess:
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "probe.cpp"
            source.write_text(_SOURCE)
            # H1/Ints.h opens the toolchain's <string>; an empty stand-in
            # keeps the probe independent of the VC6 headers.
            (Path(tmp) / "string").write_text("")
            return subprocess.run(
                [_clang(), "-x", "c++", f"-std={std}", "-fsyntax-only", "-fms-extensions",
                 f"-I{tmp}", f"-I{REPO / 'include'}", f"-DPROBE={probe}", *extra,
                 str(source)],
                capture_output=True, text=True)

    def test_strict_view_accepts_the_domain_and_its_storage(self):
        result = self._compile("c++20", 0)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_strict_view_rejects_other_indices(self):
        for probe in (1, 2, 3, 4, 5):
            result = self._compile("c++20", probe)
            self.assertNotEqual(result.returncode, 0, f"probe {probe} compiled")
            self.assertIn("deleted", result.stderr)

    def test_retail_view_is_a_plain_array(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "probe.cpp"
            source.write_text(_SOURCE)
            (Path(tmp) / "string").write_text("")
            result = subprocess.run(
                [_clang(), "-x", "c++", "-std=c++98", "-E", "-P", "-fms-extensions",
                 f"-I{tmp}", f"-I{REPO / 'include'}", "-DPROBE=1", str(source)],
                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("int gTable[SLOT_COUNT] = {1, 2};", result.stdout)
        self.assertIn("short gGrid[SLOT_COUNT][SLOT_COUNT];", result.stdout)
        self.assertNotIn("H1EnumArray", result.stdout)
        self.assertNotIn("H1Bool", result.stdout)
        self.assertIn("typedef i8 b8;", result.stdout)


if __name__ == "__main__":
    unittest.main()
