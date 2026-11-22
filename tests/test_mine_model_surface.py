"""Compare mine surface queries and the full object-to-terrain contact path."""
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
class MineModelSurfaceTests(unittest.TestCase):
    def test_grid_and_box_contacts(self):
        room = ROOT / "src/overlays/sc02_0039"
        shared = ROOT / "src/overlays/main_0012"
        model = room / "80135d20.c"
        providers = [
            shared / name for name in (
                "8014fe60.c", "8014ffdc.c", "80150150.c",
                "80150170.c", "801502ec.c", "80150460.c",
            )
        ]
        cases = {
            "mine_model_surface_probe.c": [model],
            "mine_model_contact_probe.c": [
                *providers, room / "80134510.c", room / "801345f8.c", model,
            ],
        }
        with tempfile.TemporaryDirectory() as directory:
            for source, bodies in cases.items():
                with self.subTest(source=source):
                    executable = Path(directory) / Path(source).stem
                    subprocess.run([
                        *CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-I", ROOT / "include", "-I", room, "-I", shared,
                        ROOT / "tests" / source, *bodies, "-o", executable,
                    ], check=True)
                    subprocess.run([executable], check=True)


if __name__ == "__main__":
    unittest.main()
