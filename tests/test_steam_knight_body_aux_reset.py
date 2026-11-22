from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_steam_knight_body_aux_reset(tmp_path, compiler):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")

    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0005"
    executable = tmp_path / "body-aux-reset"
    subprocess.run([
        cc, "-std=c11", "-O2", "-fstrict-aliasing", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all",
        "-I", root / "include", "-I", scene, scene / "80184c20.c", scene / "80186f58.c",
        root / "tests/steam_knight_body_aux_reset_probe.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
