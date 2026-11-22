from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_steam_knight_state_two(tmp_path, compiler):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")

    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0005"
    sources = ["801838a8.c", "80182810.c", "801828d8.c", "8018791c.c", "80182918.c"]
    executable = tmp_path / "state-two"
    subprocess.run([
        cc, "-std=c11", "-O2", "-fstrict-aliasing", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all",
        "-I", root / "include", "-I", scene, *(scene / name for name in sources),
        root / "tests/steam_knight_state_two_probe.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
