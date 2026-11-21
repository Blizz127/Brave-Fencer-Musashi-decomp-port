"""Check forwarding of the three original draw arguments through both branches."""
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
class ModelDrawDispatch(unittest.TestCase):
    def test_forward_arguments(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / "model-draw-probe"
            subprocess.run([
                *CC, "-std=c11", "-O2", "-Wall", "-Wextra",
                "-Werror", "-UNDEBUG", "-I", str(ROOT / "include"), "-I", str(ROOT / "src/main"),
                str(ROOT / "src/main/800243ec.c"),
                str(ROOT / "tests/model_draw_dispatch_probe.c"), "-o", str(binary),
            ], check=True)
            subprocess.run([binary], check=True)


if __name__ == "__main__":
    unittest.main()
