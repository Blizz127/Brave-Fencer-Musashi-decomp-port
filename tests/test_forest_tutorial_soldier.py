"""Check the forest tutorial soldier request and its countdown."""
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src" / "overlays" / "sc01_0077"


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_forest_tutorial_soldier(compiler, tmp_path):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")
    binary = tmp_path / "forest-tutorial-soldier"
    sources = [SCENE / name for name in ("8017e1d4.c", "8017e460.c", "8017f240.c")]
    subprocess.run([
        cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all",
        "-I", ROOT / "include", "-I", SCENE,
        *sources, ROOT / "tests" / "forest_tutorial_soldier_probe.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
