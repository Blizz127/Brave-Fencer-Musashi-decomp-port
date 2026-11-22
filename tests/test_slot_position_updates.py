"""Build and exercise the 20-slot position update with synthetic scene objects."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class SlotPositionTests(unittest.TestCase):
    def test_callback_order_and_position_copies(self):
        cc = shlex.split(os.environ.get("CC") or "cc")
        if not shutil.which(cc[0]):
            self.skipTest("a host C compiler is required")
        scene = ROOT / "src/overlays/sc02_0039"
        with tempfile.TemporaryDirectory() as temporary:
            program = Path(temporary) / "slot_positions"
            subprocess.run([
                *cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-I", ROOT / "include", "-I", scene, scene / "80162b1c.c",
                ROOT / "tests/slot_position_probe.c", "-o", program,
            ], check=True)
            result = subprocess.run([program])
            if result.returncode == 77:
                self.skipTest("host addresses must exceed the 32-bit guest range")
            result.check_returncode()


if __name__ == "__main__":
    unittest.main()
