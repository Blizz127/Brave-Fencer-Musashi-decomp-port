"""Check each forest caller and the sound dispatcher against one C function type."""
import os
from pathlib import Path
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
UNITS = (
    "src/main/8002d4c8.c",
    "src/overlays/main_0012/80149954.c",
    "src/overlays/sc01_0077/8017f460.c",
    "src/overlays/sc01_0077/8018047c.c",
    "src/overlays/sc01_0077/8018470c.c",
    "tests/forest_log_standing_probe.c",
    "tests/forest_log_fall_probe.c",
)


class ForestSoundAbiTest(unittest.TestCase):
    def check_units(self, compiler, sources):
        for source in sources:
            with self.subTest(source=source):
                # Check after inclusion so a missing declaration also fails.
                unit = f'#include "{source}"\n' + (
                    "_Static_assert(_Generic(&func_8002D4C8, "
                    "void (*)(unsigned short, unsigned short): 1, default: 0), "
                    '"forest sound command ABI");\n'
                )
                subprocess.run(
                    [*compiler, "-std=c11", "-fsyntax-only",
                     "-Iinclude", "-x", "c", "-"],
                    input=unit, cwd=ROOT, text=True, check=True,
                )

    def test_host_command_type(self):
        self.check_units([os.environ.get("CC", "cc")], UNITS)

    def test_shared_forest_caller_on_mips(self):
        clang = shutil.which("clang")
        if not clang or "mips" not in subprocess.check_output(
            [clang, "--print-targets"], text=True
        ):
            self.skipTest("Clang MIPS target unavailable")
        # The shared secondary-event source selects its direct sound call on MIPS.
        self.check_units(
            [clang, "--target=mipsel-none-elf", "-mabi=32", "-ffreestanding"],
            ["src/overlays/main_0012/80149954.c"],
        )


if __name__ == "__main__":
    unittest.main()
