from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_steam_knight_phase_entry(tmp_path, compiler):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")

    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0005"
    executable = tmp_path / "phase-entry"
    subprocess.run([
        cc, "-std=c11", "-O2", "-fstrict-aliasing", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all",
        "-I", root / "include", "-I", scene, scene / "80182998.c", scene / "8012ad44.c",
        root / "tests/steam_knight_phase_entry_probe.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
