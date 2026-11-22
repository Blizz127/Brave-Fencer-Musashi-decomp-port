"""Exercise the mine camera frame with synthetic high-address camera records."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class MineCameraFrameTests(unittest.TestCase):
    def test_camera_frame_and_hook_refusal(self):
        cc = shlex.split(os.environ.get("CC") or "cc")
        if not shutil.which(cc[0]):
            self.skipTest("a host C compiler is required")
        overlays = ROOT / "src/overlays"
        with tempfile.TemporaryDirectory() as tmp:
            program = Path(tmp) / "camera_frame"
            subprocess.run([
                *cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-I", ROOT / "include", "-I", overlays / "sc02_0039",
                overlays / "sc02_0039/80129cf8.c", ROOT / "tests/mine_camera_frame_probe.c",
                "-o", program,
            ], check=True)
            result = subprocess.run([program])
            if result.returncode == 77:
                self.skipTest("camera and matrix storage must exceed the 32-bit guest range")
            result.check_returncode()


if __name__ == "__main__":
    unittest.main()
