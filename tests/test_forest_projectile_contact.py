"""Check the forest projectile offset and attack-power calls."""
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src" / "overlays" / "sc01_0077"


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_forest_projectile_contact(compiler, tmp_path):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")
    binary = tmp_path / "forest-projectile-contact"
    sources = [SCENE / name for name in ("80163408.c", "8016345c.c")]
    subprocess.run([
        cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all",
        "-I", ROOT / "include", "-I", SCENE,
        *sources, ROOT / "tests" / "forest_projectile_contact_probe.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
