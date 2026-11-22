"""Check that the per-frame service forwards the entire object pointer."""
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
class SceneTargetPointerTests(unittest.TestCase):
    def test_first_nearby_target(self):
        room = ROOT / "src/overlays/main_0012"
        with tempfile.TemporaryDirectory() as tmp:
            program = Path(tmp) / "target-pointer"
            subprocess.run([
                *CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-I", ROOT / "include", "-I", room, room / "801508b4.c",
                room / "8014a2e4.c", room / "8014c278.c",
                ROOT / "tests/scene_target_pointer_probe.c", "-o", program,
            ], check=True)
            result = subprocess.run([program])
            if result.returncode == 77:
                self.skipTest("host did not allocate the object pool above the 32-bit range")
            result.check_returncode()


if __name__ == "__main__":
    unittest.main()
