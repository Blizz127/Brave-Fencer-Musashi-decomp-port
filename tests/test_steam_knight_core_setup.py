from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_steam_knight_core_model_setup(tmp_path, compiler):
    if shutil.which(compiler) is None:
        pytest.skip(f"{compiler} unavailable")
    root = Path(__file__).resolve().parents[1]
    room = root / "src/overlays/sc02_0005"
    executable = tmp_path / "steam_knight_core_setup"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fstrict-aliasing", "-fsanitize=undefined", "-fno-sanitize-recover=all",
        "-I", root / "include", "-I", room, room / "80183fcc.c", room / "801840e8.c",
        root / "tests/steam_knight_core_setup_probe.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
