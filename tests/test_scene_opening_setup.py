import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc01_0077"


@unittest.skipUnless(sys.platform.startswith("linux") and shutil.which("cc"),
                     "the host test uses ELF linker symbols for the camera storage")
class OpeningSetupTests(unittest.TestCase):
    def test_camera_registration_and_event_counter(self):
        sources = [SCENE / (address + ".c") for address in
                   ("8017de84", "8017dec0", "8016f398", "8017d900", "80174e9c",
                    "8012a018", "8012a048")]
        # The named globals share one underlying camera structure in the scene.
        addresses = (0x80126940, 0x80126942, 0x80126944, 0x80126948,
                     0x80126950, 0x80126954, 0x8012695C, 0x80126968, 0x8012696A,
                     0x8012696C, 0x80126976, 0x80126978, 0x8012697A, 0x8012697C,
                     0x80126980, 0x80126B5E, 0x80126B62, 0x80126B66)
        symbols = [f"-Wl,--defsym,D_{a:08X}=camera_storage+{a - addresses[0]}"
                   for a in addresses]
        with tempfile.TemporaryDirectory() as folder:
            exe = Path(folder) / "scene-setup"
            subprocess.run(
                [os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
                 "-O2", "-fno-pie", "-no-pie", "-I", ROOT / "include",
                 *sources, ROOT / "src/main/80029124.c", ROOT / "src/main/800118ac.c",
                 ROOT / "tests/scene_opening_setup_probe.c", *symbols, "-o", exe],
                check=True,
            )
            subprocess.run([exe], check=True)
