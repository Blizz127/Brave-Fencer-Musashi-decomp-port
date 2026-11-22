"""Exercise the two scene floor scanners and their map-query wrappers."""
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
class SceneFloorProviderTests(unittest.TestCase):
    def test_probe_selection_and_pointer_forwarding(self):
        overlay = ROOT / "src/overlays/main_0012"
        sources = [overlay / f"{address}.c" for address in
                   ("8014ffdc", "80150150", "801502ec", "80150460")]
        with tempfile.TemporaryDirectory() as tmp:
            program = Path(tmp) / "floor-providers"
            subprocess.run([
                *CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-I", ROOT / "include", "-I", overlay, *sources,
                ROOT / "tests/scene_floor_provider_probe.c", "-o", program,
            ], check=True)
            result = subprocess.run([program])
            if result.returncode == 77:
                self.skipTest("host allocated the probe inside the 32-bit address range")
            result.check_returncode()


if __name__ == "__main__":
    unittest.main()
