"""Link the mine pickup action to the original-matching 96-object scanner."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class ActorPickupTests(unittest.TestCase):
    def test_contact_and_effect_order(self):
        cc = shlex.split(os.environ.get("CC") or "cc")
        if not shutil.which(cc[0]):
            self.skipTest("a host C compiler is required")
        overlays = ROOT / "src/overlays"
        with tempfile.TemporaryDirectory() as tmp:
            program = Path(tmp) / "pickup"
            subprocess.run([
                *cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-I", ROOT / "include", "-I", overlays / "sc02_0039",
                overlays / "main_0012/801505fc.c", overlays / "sc02_0039/801506a4.c",
                ROOT / "tests/actor_pickup_probe.c", "-o", program,
            ], check=True)
            result = subprocess.run([program])
            if result.returncode == 77:
                self.skipTest("host objects must be above the 32-bit guest range")
            result.check_returncode()


if __name__ == "__main__":
    unittest.main()
