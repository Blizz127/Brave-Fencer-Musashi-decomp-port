import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class RoomStreamSelectorTests(unittest.TestCase):
    def test_room_selection_and_request(self):
        with tempfile.TemporaryDirectory() as temporary:
            exe = Path(temporary) / "room-stream-selector"
            compiler = shlex.split(os.environ.get("CC") or "cc")
            subprocess.run([
                *compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                "-fsanitize=undefined", "-fno-sanitize-recover=all",
                ROOT / "tests/room_stream_selector_probe.c", "-o", exe,
            ], check=True)
            subprocess.run([exe], check=True)


if __name__ == "__main__":
    unittest.main()
