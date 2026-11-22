"""Check forest actor collision across the slot pool and both contact passes."""
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("cc,flags", [
    ("cc", ["-O2"]), ("clang", ["-O2"]),
    ("cc", ["-O2", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
], ids=["gcc", "clang", "ubsan"])
def test_actor_collision(tmp_path, cc, flags):
    if not shutil.which(cc):
        pytest.skip(f"{cc} unavailable")
    overlay = ROOT / "src/overlays/sc01_0077"
    output = tmp_path / "actor-collision"
    subprocess.run([
        cc, "-std=c11", "-UNDEBUG", "-Wall", "-Wextra", "-Werror", *flags,
        "-I", ROOT / "include", ROOT / "tests/forest_actor_collision_probe.c",
        overlay / "8014e98c.c", overlay / "8014ea4c.c", "-o", output,
    ], check=True)
    subprocess.run([output], check=True)
