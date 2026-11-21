import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc01_0077"


@unittest.skipUnless(sys.platform.startswith("linux") and shutil.which("cc"),
                     "the fixture uses Linux non-PIE addresses for 32-bit guest pointers")
class SceneDirectionTests(unittest.TestCase):
    def test_timer_advances_the_state_after_cleanup(self):
        with tempfile.TemporaryDirectory() as folder:
            exe = Path(folder) / "scene-direction"
            subprocess.run(
                ["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                 "-fno-pie", "-no-pie", "-I", str(ROOT / "include"), "-I", str(SCENE),
                 str(ROOT / "tests/scene_direction_probe.c"), str(SCENE / "80171b4c.c"),
                 str(SCENE / "8017e170.c"), "-o", str(exe)],
                check=True,
            )
            subprocess.run([exe], check=True)
