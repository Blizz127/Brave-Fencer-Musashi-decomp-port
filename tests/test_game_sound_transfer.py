from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("name", ["gcc", "clang"])
def test_game_sound_transfer(name, tmp_path):
    compiler = shutil.which(name)
    if compiler is None:
        pytest.skip(f"{name} is unavailable")
    source = Path(__file__).with_name("game_sound_transfer_probe.c")
    executable = tmp_path / "game_sound_transfer"
    subprocess.run(
        [compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
         str(source), "-o", str(executable)],
        check=True,
    )
    subprocess.run([str(executable)], check=True)
