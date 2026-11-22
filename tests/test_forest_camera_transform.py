from pathlib import Path
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_forest_camera_transform(compiler, tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc01_0077"
    binary = tmp_path / "forest-camera-transform"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-flto", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all",
        "-I", scene, scene / "8012f14c.c",
        root / "tests/forest_camera_transform_probe.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
