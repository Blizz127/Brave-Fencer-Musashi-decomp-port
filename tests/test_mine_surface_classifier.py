"""Check mine surface modes and the arguments passed to each matrix service."""
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
class MineSurfaceClassifierTests(unittest.TestCase):
    def test_modes_and_scale_failures(self):
        region = ROOT / "src/overlays/sc02_0039"
        with tempfile.TemporaryDirectory() as tmp:
            program = Path(tmp) / "classifier"
            subprocess.run([
                *CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-I", ROOT / "include", "-I", region, region / "80135480.c",
                ROOT / "tests/mine_surface_classifier_probe.c", "-o", program,
            ], check=True)
            result = subprocess.run([program])
            if result.returncode == 77:
                self.skipTest("host placed the model or output inside the 32-bit address range")
            result.check_returncode()


if __name__ == "__main__":
    unittest.main()
