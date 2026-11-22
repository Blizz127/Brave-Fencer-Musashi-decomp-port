"""Compile and run the room rotor against synthetic actors and host services."""
from pathlib import Path
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
OVERLAY = ROOT / "src/overlays/sc02_0039"


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_rotor_active(tmp_path, compiler):
    binary = tmp_path / "rotor"
    subprocess.run(
        [compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
         "-fsanitize=undefined", "-fno-sanitize-recover=all",
         "-I", str(ROOT / "include"), "-I", str(OVERLAY),
         str(OVERLAY / "8017dc7c.c"), str(ROOT / "tests/mine_rotor_active_probe.c"),
         "-o", str(binary)],
        check=True,
    )
    subprocess.run([binary], check=True)
