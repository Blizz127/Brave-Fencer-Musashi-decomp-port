"""Check the forest impact effect lifecycle and state table."""
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src" / "overlays" / "sc01_0077"


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_forest_impact_effect(compiler, tmp_path):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")
    binary = tmp_path / "forest-impact-effect"
    sources = [SCENE / name for name in ("80168aa8.c", "80168ae4.c", "80168b70.c")]
    subprocess.run([
        cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all",
        "-I", ROOT / "include", "-I", SCENE,
        *sources, ROOT / "tests" / "forest_impact_effect_probe.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
