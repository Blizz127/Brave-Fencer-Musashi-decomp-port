"""Exercise the signed object-distance return through both resident functions."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CC = shlex.split(os.environ.get("CC") or "cc")


@unittest.skipUnless(shutil.which(CC[0]), "a host C compiler is required")
class SceneObjectDistanceTests(unittest.TestCase):
    def test_range_check(self):
        room = ROOT / "src/overlays/main_0012"
        resident = ROOT / "src/main"
        with tempfile.TemporaryDirectory() as tmp:
            program = Path(tmp) / "object-distance"
            subprocess.run([
                *CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                "-I", ROOT / "include", "-I", room,
                resident / "80013328.c", resident / "80013350.c",
                room / "8014c278.c", ROOT / "tests/scene_object_distance_probe.c",
                "-o", program,
            ], check=True)
            subprocess.run([program], check=True)


if __name__ == "__main__":
    unittest.main()
