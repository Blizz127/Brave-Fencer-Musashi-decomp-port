import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
CC = os.environ.get("CC", "cc")


@unittest.skipUnless(shutil.which(CC), "requires a C compiler")
class MenuCounterTest(unittest.TestCase):
    def run_probe(self, *defines):
        scene = ROOT / "src/overlays/main_0012"
        with tempfile.TemporaryDirectory() as directory:
            exe = Path(directory) / "menu-counter"
            subprocess.run(
                [CC, "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2", *defines,
                 "-I", ROOT / "include", scene / "8017c24c.c", scene / "8017c348.c",
                 ROOT / "tests/menu_counter_probe.c", "-o", exe],
                check=True,
            )
            subprocess.run([exe], check=True)

    def test_full_increment_and_wrapped_store(self):
        self.run_probe()

    def test_neighboring_halfwords(self):
        # With this declaration the two callbacks still share a single counter
        # and the fixture can place a halfword immediately on either side.
        self.run_probe("-DD_80115112=(*menu_counter)")
