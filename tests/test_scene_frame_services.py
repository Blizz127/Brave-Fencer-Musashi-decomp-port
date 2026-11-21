"""Check regional frame service order with generated state."""
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
class SceneFrameServices(unittest.TestCase):
    def test_service_order(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / "scene-frame-probe"
            subprocess.run([
                *CC, "-std=c11", "-O2", "-Wall", "-Wextra",
                "-Werror", "-UNDEBUG", "-I", str(ROOT / "include"), "-I", str(ROOT / "src/main"),
                str(ROOT / "src/overlays/sc02_0039/801287b8.c"),
                str(ROOT / "tests/scene_frame_services_probe.c"), "-o", str(binary),
            ], check=True)
            subprocess.run([binary], check=True)


if __name__ == "__main__":
    unittest.main()
