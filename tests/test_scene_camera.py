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
class SceneCameraTests(unittest.TestCase):
    def test_camera_smoothing_and_transform(self):
        with tempfile.TemporaryDirectory() as work:
            probe = Path(work) / "scene-camera"
            subprocess.run(
                [*CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                 "-I", ROOT / "include", "-I", SCENE, ROOT / "tests/scene_camera_probe.c",
                 SCENE / "8017d2b8.c", ROOT / "src/main/80012abc.c",
                 ROOT / "src/main/80012c6c.c", "-o", probe],
                check=True,
            )
            subprocess.run([probe], check=True)


if __name__ == "__main__":
    unittest.main()
