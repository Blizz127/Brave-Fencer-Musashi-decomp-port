from pathlib import Path
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_forest_placement_activation(compiler, tmp_path):
    root = Path(__file__).resolve().parents[1]
    binary = tmp_path / "forest-placement"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-Wno-int-to-pointer-cast", "-fno-pie", "-no-pie",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", "-I", root / "include",
        root / "tests/forest_placement_activation_probe.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
