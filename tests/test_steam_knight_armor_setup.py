from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_steam_knight_armor_setup(compiler, tmp_path):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} is unavailable")
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0005"
    binary = tmp_path / "armor_setup"
    subprocess.run(
        [
            cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=undefined", "-fno-sanitize-recover=undefined",
            "-I", root / "include", "-I", scene, scene / "80184ec0.c",
            root / "tests/steam_knight_armor_setup_probe.c", "-o", binary,
        ],
        check=True,
    )
    subprocess.run([binary], check=True)
