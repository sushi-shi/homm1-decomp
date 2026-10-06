"""Every wine process the tooling starts runs in one time zone."""
import os
import unittest
from unittest import mock

from homm1.tool import wine


class WineEnvTests(unittest.TestCase):
    def test_pins_the_zone_over_the_host_one(self):
        with mock.patch.dict(os.environ, {"TZ": "Europe/Warsaw", "KEEP": "1"}):
            env = wine.wine_env()
        self.assertEqual(env["TZ"], wine.WINE_ZONE)
        self.assertEqual(env["KEEP"], "1")

    def test_pins_the_zone_of_a_given_environment(self):
        env = wine.wine_env({"TZ": "America/New_York", "FAKETIME": "x"})
        self.assertEqual(env, {"TZ": wine.WINE_ZONE, "FAKETIME": "x"})

    def test_run_gives_the_tool_the_zone(self):
        seen = {}

        def popen(argv, **kwargs):
            seen.update(kwargs["env"])
            raise FileNotFoundError(argv[0])
        with mock.patch.object(wine, "ensure_wineserver"), \
                mock.patch.object(wine.subprocess, "Popen", side_effect=popen), \
                mock.patch.dict(os.environ, {"TZ": "Europe/Warsaw"}):
            with self.assertRaises(wine.ToolError):
                wine.run(["wine", "cl.exe"])
        self.assertEqual(seen["TZ"], wine.WINE_ZONE)


if __name__ == "__main__":
    unittest.main()
