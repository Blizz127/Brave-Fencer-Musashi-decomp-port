import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class ResidentInventoryTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which(os.environ.get("CC", "cc")), "requires a C compiler")
    def test_search_acquire_and_remove(self):
        sources = [ROOT / "src/overlays/main_0010" / f"{address}.c" for address in
                   ("800d0ec4", "800d0f0c", "800d0f4c", "800d0f8c", "800d0fe0")]
        accessors = [ROOT / "src/main" / f"{address}.c" for address in
                     ("800291a0", "800291b4", "800291c8")]
        with tempfile.TemporaryDirectory() as output:
            exe = Path(output) / "inventory"
            subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
                            "-Werror", "-O2", "-I", ROOT / "include", *sources, *accessors,
                            ROOT / "tests/resident_inventory_probe.c", "-o", exe], check=True)
            subprocess.run([exe], check=True)
