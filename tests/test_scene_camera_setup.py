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
class SceneCameraSetupTests(unittest.TestCase):
    def test_setup_and_dispatch_with_shared_camera(self):
        with tempfile.TemporaryDirectory() as work:
            build = Path(work)
            flags = [*CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                     "-I", ROOT / "include", "-I", SCENE]
            providers = [
                ("sc01_0077/8012a018.c", "func_8012A018", "camera_probe_register"),
                ("sc02_0035/8012a094.c", "func_8012A094", "camera_probe_sync"),
            ]
            objects = []
            for relative, symbol, wrapper in providers:
                obj = build / (symbol + ".o")
                subprocess.run([*flags, "-D" + symbol + "=" + wrapper, "-c",
                                ROOT / "src/overlays" / relative, "-o", obj], check=True)
                objects.append(obj)
            probe = build / "scene-camera-setup"
            subprocess.run([*flags, ROOT / "tests/scene_camera_setup_probe.c",
                            SCENE / "8017d01c.c", SCENE / "8017d0d4.c",
                            ROOT / "src/overlays/sc01_0077/8012a048.c", *objects,
                            "-o", probe], check=True)
            subprocess.run([probe], check=True)


if __name__ == "__main__":
    unittest.main()
