from pathlib import Path
import subprocess
import sys

import pytest


@pytest.mark.skipif(sys.platform != "linux", reason="probe uses a non-PIE Linux executable for 32-bit addresses")
@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_game_transfer(compiler, tmp_path):
    program = tmp_path / "game_transfer"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fno-pie", "-no-pie", "-fno-strict-aliasing", "-fsanitize=undefined",
        "-fno-sanitize-recover=all", Path(__file__).with_name("game_transfer_probe.c"),
        "-o", program,
    ], check=True)
    subprocess.run([program], check=True, timeout=10)
