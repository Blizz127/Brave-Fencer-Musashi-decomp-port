"""Compare the mine's three face routines and their box dispatcher on the host."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class MineBoxTests(unittest.TestCase):
    def test_project_and_publish_contacts(self):
        room = ROOT / "src/overlays/sc02_0039"
        with tempfile.TemporaryDirectory() as temp:
            exe = Path(temp) / "mine-boxes"
            subprocess.run([
                *shlex.split(os.environ.get("CC") or "cc"), "-std=c11", "-O2", "-Wall",
                "-Wextra", "-Werror", "-I", ROOT / "include", "-I", room,
                ROOT / "tests/mine_box_faces_probe.c", room / "80135eb0.c", room / "80136334.c",
                room / "801365b8.c", room / "80136824.c", "-o", exe,
            ], check=True)
            subprocess.run([exe], check=True)


if __name__ == "__main__":
    unittest.main()
