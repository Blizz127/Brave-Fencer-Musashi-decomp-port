from pathlib import Path
import subprocess
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.skipif(sys.platform != "linux" or sys.byteorder != "little",
                    reason="requires little-endian ELF data aliases")
@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_room_resource_survives_reset_word_alias(compiler, tmp_path):
    overlay = ROOT / "src/overlays/main_0010"
    exe = tmp_path / "room_descriptor_alias"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
        "-fno-pie", "-no-pie", "-I", ROOT / "include",
        ROOT / "tests/room_descriptor_alias_probe.c",
        overlay / "800d17b8.c", overlay / "800d185c.c",
        "-Wl,--defsym,D_80114EA8=test_source+4",
        "-Wl,--defsym,D_800AE6B4=D_800AE6A8+12",
        "-Wl,--defsym,D_800AE6BC=D_800AE6A8+20", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
