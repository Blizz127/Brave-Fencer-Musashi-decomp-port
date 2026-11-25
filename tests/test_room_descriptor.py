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
class RoomDescriptorTests(unittest.TestCase):
    def test_layout_and_saved_room_round_trip(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "room-descriptor"
            subprocess.run([
                *CC, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-I", ROOT / "include", ROOT / "tests/room_descriptor_probe.c",
                ROOT / "src/main/800296f8.c", ROOT / "src/main/800298bc.c",
                "-o", output,
            ], check=True)
            subprocess.run([output], check=True)


if __name__ == "__main__":
    unittest.main()
