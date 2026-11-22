import shutil
import subprocess
from pathlib import Path

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_game_track_rows(compiler, tmp_path):
    if shutil.which(compiler) is None:
        pytest.skip(f"{compiler} is unavailable")
    root = Path(__file__).resolve().parents[1]
    binary = tmp_path / "game-track-rows"
    subprocess.run(
        [
            compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
            "-I", root / "include",
            root / "src/main/80030730.c", root / "src/main/80030a14.c",
            root / "pc_port/game_track_rows.c", root / "tests/game_track_rows_probe.c",
            "-o", binary,
        ],
        check=True,
    )
    subprocess.run([binary], check=True)
