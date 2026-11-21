"""Check channel release with typed host voice records and generated state."""
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
class SoundVoiceRelease(unittest.TestCase):
    def test_callback_and_slot_effects(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / "sound-voice-release"
            subprocess.run([
                *CC, "-std=c11", "-O2", "-Wall", "-Wextra",
                "-Werror", "-UNDEBUG", "-I", str(ROOT / "include"), "-I", str(ROOT / "src/main"),
                str(ROOT / "src/main/80030d80.c"),
                str(ROOT / "tests/sound_voice_release_probe.c"), "-o", str(binary),
            ], check=True)
            subprocess.run([binary], check=True)


if __name__ == "__main__":
    unittest.main()
