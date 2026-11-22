"""Build and exercise the 96-actor position update with synthetic objects and callbacks."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class ActorPositionTests(unittest.TestCase):
    def test_actor_updates_and_refusals(self):
        cc = shlex.split(os.environ.get("CC") or "cc")
        if not shutil.which(cc[0]):
            self.skipTest("a host C compiler is required")
        scene = ROOT / "src/overlays/sc02_0039"
        with tempfile.TemporaryDirectory() as temporary:
            program = Path(temporary) / "actor_positions"
            subprocess.run([
                *cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-I", ROOT / "include", "-I", scene, scene / "8017849c.c",
                ROOT / "tests/actor_position_probe.c", "-o", program,
            ], check=True)
            result = subprocess.run([program])
            if result.returncode == 77:
                self.skipTest("host addresses must exceed the 32-bit guest range")
            result.check_returncode()


if __name__ == "__main__":
    unittest.main()
