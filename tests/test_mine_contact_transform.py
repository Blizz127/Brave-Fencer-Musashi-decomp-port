"""Exercise the room-specific contact transform with synthetic matrix services."""
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
class MineTransformTests(unittest.TestCase):
    def test_rotate_and_translate(self):
        region = ROOT / "src/overlays/sc02_0039"
        with tempfile.TemporaryDirectory() as output:
            program = Path(output) / "mine-transform"
            subprocess.run([
                *CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                "-I", ROOT / "include", "-I", region, region / "80136a94.c",
                ROOT / "tests/mine_contact_transform_probe.c", "-o", program,
            ], check=True)
            subprocess.run([program], check=True)


if __name__ == "__main__":
    unittest.main()
