"""Link and exercise the three mine actor prepasses on synthetic records."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class ActorPrepassTests(unittest.TestCase):
    def test_timer_and_per_actor_dispatch(self):
        cc = shlex.split(os.environ.get("CC") or "cc")
        if not shutil.which(cc[0]):
            self.skipTest("a host C compiler is required")
        scene = ROOT / "src/overlays/sc02_0039"
        with tempfile.TemporaryDirectory() as tmp:
            program = Path(tmp) / "actor_prepasses"
            subprocess.run([
                *cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-I", ROOT / "include", "-I", scene, scene / "8017869c.c",
                scene / "801787d4.c", scene / "8012a908.c",
                ROOT / "tests/actor_prepass_probe.c", "-o", program,
            ], check=True)
            result = subprocess.run([program])
            if result.returncode == 77:
                self.skipTest("the actor pool must be above the 32-bit guest range")
            result.check_returncode()


if __name__ == "__main__":
    unittest.main()
