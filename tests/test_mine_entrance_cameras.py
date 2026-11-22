import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = (
    "sc01_0077/8012a018.c", "sc01_0077/8012a048.c", "sc02_0035/8012a094.c",
    "sc03_0109/8017d2e0.c", "sc03_0109/8017d384.c",
    "sc03_0113/8017f07c.c", "sc03_0113/8017f120.c",
)
CC = shlex.split(os.environ.get("CC") or "cc")


@unittest.skipUnless(shutil.which(CC[0]), "a host C compiler is required")
class MineEntranceCameraTests(unittest.TestCase):
    def test_distinct_entrances_and_selected_camera_modes(self):
        with tempfile.TemporaryDirectory() as folder:
            overlays = ROOT / "src/overlays"
            target = Path(folder) / "camera"
            subprocess.run([
                *CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-I", ROOT / "include", "-I", overlays,
                ROOT / "tests/mine_entrance_camera_probe.c",
                *(overlays / file for file in SOURCES), "-o", target,
            ], check=True)
            subprocess.run([target], check=True)


if __name__ == "__main__":
    unittest.main()
