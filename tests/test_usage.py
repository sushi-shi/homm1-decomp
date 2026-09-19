"""Usage coverage and failure isolation at real command boundaries."""
import ast
from concurrent.futures import ThreadPoolExecutor
from contextlib import redirect_stderr, redirect_stdout
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from homm1.core import paths, usage


class UsageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.build_patch = patch.object(paths, "BUILD", self.root / "build")
        self.build_patch.start()
        self.addCleanup(self.build_patch.stop)

    def rows(self):
        return [json.loads(line) for line in
                (self.root / "build/homm1_usage.jsonl").read_text().splitlines()]

    def test_success_failure_interrupt_and_argument_fidelity(self):
        @usage.logged
        def command(argv):
            if argv[0] == "exit":
                raise SystemExit(2)
            if argv[0] == "crash":
                raise ValueError("broken")
            if argv[0] == "interrupt":
                raise KeyboardInterrupt()
            return 7

        args = ["space and 'quotes'", "newline\nhere"]
        self.assertEqual(command(args), 7)
        for arg, exception in [("exit", SystemExit), ("crash", ValueError),
                               ("interrupt", KeyboardInterrupt)]:
            with self.assertRaises(exception):
                command([arg])
        rows = self.rows()
        self.assertEqual(rows[0]["argv"], args)
        self.assertEqual([r["exit_code"] for r in rows if r["event"] == "finish"],
                         [7, 2, 1, 130])
        for start, finish in zip(rows[::2], rows[1::2]):
            self.assertEqual(start["id"], finish["id"])
            self.assertGreaterEqual(finish["duration_s"], 0)

    def test_unwritable_log_preserves_result_and_warns(self):
        self.root.joinpath("build").write_text("not a directory")
        with patch.object(usage, "_warned", False), redirect_stderr(io.StringIO()) as err:
            self.assertEqual(usage.logged(lambda argv: 9)([]), 9)
        self.assertEqual(err.getvalue().count("[usage]"), 1)

    def test_batch_keeps_each_query_and_parent_relationship(self):
        from homm1.cli import main
        with patch("sys.stdin", io.StringIO("--help\nunknown-view\n")), \
                redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            self.assertEqual(main(["sema", "-"]), 2)
        starts = [r for r in self.rows() if r["event"] == "start"]
        self.assertEqual([r["argv"] for r in starts],
                         [["sema", "-"], ["-"], ["--help"], ["unknown-view"]])
        self.assertEqual(starts[2]["parent_id"], starts[1]["id"])
        self.assertEqual(starts[3]["parent_id"], starts[1]["id"])

    def test_subprocess_launch_records_no_environment(self):
        @usage.logged
        def command(argv):
            return subprocess.run([sys.executable, "-c", "pass"],
                                  env={"PRIVATE_TEST_TOKEN": "do-not-record"}).returncode
        self.assertEqual(command([]), 0)
        rows = self.rows()
        launch = next(r for r in rows if r["event"] == "subprocess")
        self.assertEqual(launch["parent_id"], rows[0]["id"])
        self.assertNotIn("do-not-record", json.dumps(rows))

    def test_direct_modules_and_concurrent_processes(self):
        self.root.joinpath("flake.nix").touch()
        import os
        env = {**os.environ, "PYTHONPATH": str(paths.REPO / "scripts")}

        def run(_):
            return subprocess.run([sys.executable, "-m", "homm1.sema.strings", "--help"],
                                  cwd=self.root, env=env, capture_output=True, text=True)

        with ThreadPoolExecutor(max_workers=4) as pool:
            results = list(pool.map(run, range(8)))
        self.assertTrue(all(r.returncode == 0 for r in results), results)
        rows = self.rows()
        starts = [r for r in rows if r["event"] == "start"]
        ends = [r for r in rows if r["event"] == "finish"]
        self.assertEqual(len(starts), 8)
        self.assertEqual({r["id"] for r in starts}, {r["id"] for r in ends})
        self.assertTrue(all(r["module"] == "homm1.sema.strings" for r in starts))

    def test_every_main_is_instrumented(self):
        missing = []
        for path in (paths.REPO / "scripts/homm1").rglob("*.py"):
            tree = ast.parse(path.read_text())
            for node in tree.body:
                if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)) \
                        and node.name in ("main", "cli_main"):
                    if not any(isinstance(d, ast.Name) and d.id == "logged"
                               for d in node.decorator_list):
                        missing.append(str(path.relative_to(paths.REPO)))
        self.assertEqual(missing, [], "New entry points must retain usage logging")
