import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc02_0039"
CC = shlex.split(os.environ.get("CC") or "cc")


@unittest.skipUnless(shutil.which(CC[0]), "a host C compiler is required")
class SceneCameraSmoothingTests(unittest.TestCase):
    def test_fully_linked_smoothing(self):
        with tempfile.TemporaryDirectory() as work:
            probe = Path(work) / "camera-chain"
            subprocess.run(
                [*CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                 "-I", ROOT / "include", "-I", SCENE, ROOT / "tests/scene_camera_smoothing_probe.c",
                 SCENE / "8017d2b8.c",
                 *(ROOT / "src/main" / name for name in (
                     "80012abc.c", "80012b04.c", "80012b58.c",
                     "80012c6c.c", "80012cb8.c", "80012d0c.c",
                 )), "-o", probe],
                check=True,
            )
            subprocess.run([probe], check=True)


if __name__ == "__main__":
    unittest.main()
