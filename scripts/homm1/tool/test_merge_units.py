import unittest

from homm1.tool.merge_units import merge

HEAD = "# manifest\n\n[flags]\nc = [\"/Ox\"]\n"


def unit(name, flags="c"):
    return f'[[unit]]\nunit = "{name}"\nsource = "src/{name}.c"\nflags = "{flags}"\n'


def manifest(*blocks, head=HEAD):
    return head + "".join("\n" + b for b in blocks)


class MergeUnits(unittest.TestCase):
    def test_both_sides_append(self):
        base = manifest(unit("a"))
        ours = manifest(unit("a"), unit("b"))
        theirs = manifest(unit("a"), unit("c"))
        self.assertEqual(merge(base, ours, theirs),
                         manifest(unit("a"), unit("b"), unit("c")))

    def test_deletion_against_unchanged(self):
        base = manifest(unit("a"), unit("b"))
        ours = manifest(unit("a"), unit("b"), unit("c"))
        theirs = manifest(unit("a"))
        self.assertEqual(merge(base, ours, theirs), manifest(unit("a"), unit("c")))

    def test_one_side_edit_wins(self):
        base = manifest(unit("a"))
        theirs = manifest(unit("a", "cpp"))
        self.assertEqual(merge(base, base, theirs), theirs)

    def test_header_edit_merges(self):
        base = manifest(unit("a"))
        ours = manifest(unit("a"), head=HEAD.replace("/Ox", "/O2"))
        theirs = manifest(unit("a"), unit("b"))
        self.assertEqual(merge(base, ours, theirs),
                         manifest(unit("a"), unit("b"), head=HEAD.replace("/Ox", "/O2")))

    def test_conflicting_edits_refuse(self):
        base = manifest(unit("a"))
        with self.assertRaises(ValueError):
            merge(base, manifest(unit("a", "cpp")), manifest(unit("a", "cpp-noeh")))

    def test_round_trip_real_manifest(self):
        from homm1.core.paths import CONFIG
        text = (CONFIG / "units.toml").read_text()
        self.assertEqual(merge(text, text, text), text)


if __name__ == "__main__":
    unittest.main()
