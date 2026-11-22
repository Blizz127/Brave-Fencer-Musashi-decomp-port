"""Check the forest attack-strength branches using synthetic attack records."""
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("compiler,flags", [
    ("gcc", ["-O2"]),
    ("clang", ["-O2"]),
    ("gcc", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
], ids=["gcc", "clang", "ubsan"])
def test_forest_attack_strength(tmp_path, compiler, flags):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} is unavailable")
    executable = tmp_path / "forest-attack-strength"
    subprocess.run([
        compiler, "-std=c11", *flags, "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", Path(__file__).with_name("forest_attack_strength_probe.c"),
        ROOT / "src/overlays/sc01_0077/8015616c.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
