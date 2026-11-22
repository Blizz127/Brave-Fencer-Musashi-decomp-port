from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_steam_knight_action_choice(tmp_path, compiler):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")

    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0005"
    sources = ["80185e80.c", "801860f4.c", "80186144.c", "8018ac48.c", "8018ad20.c"]
    executable = tmp_path / "action-choice"
    subprocess.run([
        cc, "-std=c11", "-O2", "-fstrict-aliasing", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", "-I", root / "include",
        "-I", scene, *(scene / name for name in sources),
        root / "tests/steam_knight_action_choice_probe.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
