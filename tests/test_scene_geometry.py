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
class SceneGeometryTests(unittest.TestCase):
    def compile_and_run(self, probe, sources):
        with tempfile.TemporaryDirectory() as folder:
            exe = Path(folder) / "scene-test"
            subprocess.run(
                [*CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                 "-I", ROOT / "include", "-I", SCENE, ROOT / "tests" / probe,
                 *(SCENE / name for name in sources), "-o", exe],
                check=True,
            )
            subprocess.run([exe], check=True)

    def test_bounds_and_packed_coordinates(self):
        self.compile_and_run("scene_geometry_probe.c", ["8017d5f4.c", "8017d6ac.c"])

    def test_facing_region_dispatch(self):
        self.compile_and_run("scene_facing_probe.c", ["8017d484.c", "8017d5f4.c", "8017d6ac.c"])


if __name__ == "__main__":
    unittest.main()
