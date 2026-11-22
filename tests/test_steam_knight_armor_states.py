from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_steam_knight_armor_states(tmp_path, compiler):
    if shutil.which(compiler) is None:
        pytest.skip(f"{compiler} unavailable")
    root = Path(__file__).resolve().parents[1]
    room = root / "src/overlays/sc02_0005"
    exe = tmp_path / "armor_states"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fstrict-aliasing", "-fsanitize=undefined", "-fno-sanitize-recover=all",
        "-I", root / "include", "-I", room, room / "8018874c.c", room / "801887cc.c",
        root / "tests/steam_knight_armor_states_probe.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
