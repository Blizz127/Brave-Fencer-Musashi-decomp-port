"""Exercise Tower model allocation, record lookup and release together."""
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src" / "overlays" / "sc02_0005"


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_tower_resource_lifecycle(compiler, tmp_path):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")
    binary = tmp_path / "resource-pool"
    units = ["8012913c.c", "801291c0.c", "801292c8.c", "8012c1b8.c"]
    subprocess.run(
        [
            cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-fstrict-aliasing",
            "-fsanitize=undefined", "-fno-sanitize-recover=all",
            "-I", ROOT / "include", "-I", SCENE,
            *[SCENE / unit for unit in units],
            ROOT / "tests" / "steam_knight_resource_pool_probe.c", "-o", binary,
        ],
        check=True,
    )
    subprocess.run([binary], check=True)
