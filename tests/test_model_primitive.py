"""Exercise the model primitive dispatcher with generated 32-bit guest records."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CC = shlex.split(os.environ.get("CC") or "cc")


@unittest.skipUnless(sys.platform.startswith("linux") and sys.byteorder == "little"
                     and shutil.which(CC[0]), "requires a little-endian Linux C host")
class ModelPrimitiveTests(unittest.TestCase):
    def test_primitive_groups(self):
        with tempfile.TemporaryDirectory() as temporary:
            binary = Path(temporary) / "model-primitive"
            subprocess.run([
                *CC, "-O2", "-std=c11", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-fno-pie", "-no-pie", "-I", ROOT / "include", "-I", ROOT / "src/main",
                ROOT / "src/main/80026d64.c", ROOT / "tests/model_primitive_probe.c",
                "-o", binary,
            ], check=True)
            result = subprocess.run([binary], timeout=15)
            if result.returncode == 77:
                self.skipTest("test storage exceeds the guest's 32-bit addresses")
            result.check_returncode()


if __name__ == "__main__":
    unittest.main()
