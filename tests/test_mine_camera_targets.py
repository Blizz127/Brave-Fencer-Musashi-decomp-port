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
class MineCameraTargetTests(unittest.TestCase):
    def test_room_bounds_and_phase_controls(self):
        with tempfile.TemporaryDirectory() as folder:
            overlays = ROOT / "src/overlays"
            exe = Path(folder) / "camera-targets"
            subprocess.run([
                *CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-I", ROOT / "include", "-I", overlays,
                ROOT / "tests/mine_camera_targets_probe.c",
                overlays / "sc02_0035/80148800.c", overlays / "sc03_0109/8017d3c0.c",
                overlays / "sc03_0113/8017f15c.c", "-o", exe,
            ], check=True)
            subprocess.run([exe], check=True)


if __name__ == "__main__":
    unittest.main()
