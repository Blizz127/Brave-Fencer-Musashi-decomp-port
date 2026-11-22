from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_core_contact_and_damage(tmp_path, compiler):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} unavailable")
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0005"
    probe = tmp_path / "core-contact"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", "-I", root / "include",
        "-I", scene, *(scene / source for source in ("80184358.c", "80185a04.c", "80185a88.c", "801869e0.c")),
        root / "src/overlays/sc02_0035/8012ad44.c", root / "tests/steam_knight_core_probe.c",
        "-o", probe,
    ], check=True)
    subprocess.run([probe], check=True)
