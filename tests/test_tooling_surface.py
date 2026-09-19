"""Donor command surfaces must remain reachable after target adaptation."""
from contextlib import redirect_stdout
import importlib
import io
import unittest
from unittest.mock import patch

from homm1 import cli


class ToolingSurfaceTests(unittest.TestCase):
    def test_all_ported_tool_entrypoints_are_public(self):
        from homm1.core.paths import REPO
        import ast
        available = set()
        for path in (REPO / "scripts/homm1/tool").glob("*.py"):
            if any(isinstance(n, ast.FunctionDef) and n.name == "main"
                   for n in ast.parse(path.read_text()).body):
                available.add(path.stem)
        self.assertEqual(set(cli.TOOLS), available)

    def test_ghidra_dispatch_preserves_arguments(self):
        module = importlib.import_module("homm1.ghidra")
        with patch.object(module, "main", return_value=7) as main:
            self.assertEqual(cli.main(["ghidra", "status"]), 7)
        main.assert_called_once_with(["status"])

    def test_linker_and_resource_compiler_help_are_reachable(self):
        # Tool mains use sys.argv; the public dispatcher must preserve their help.
        import sys
        for name in ("link", "rc", "ml"):
            with self.subTest(name=name), patch.object(sys, "argv", ["homm1"]), \
                    redirect_stdout(io.StringIO()), self.assertRaises(SystemExit) as exit:
                cli.main(["tool", name, "--help"])
            self.assertEqual(exit.exception.code, 0)
