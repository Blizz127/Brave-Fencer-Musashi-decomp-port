from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_body_dispatch_and_tracked_objects(tmp_path, compiler):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} unavailable")
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0005"
    probe = tmp_path / "body-services"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-fsanitize=undefined",
        "-fno-sanitize-recover=all", "-I", root / "include", "-I", scene,
        *(scene / source for source in ("801831a4.c", "80183200.c", "801858c4.c")),
        root / "tests/steam_knight_body_services_probe.c", "-o", probe,
    ], check=True)
    subprocess.run([probe], check=True)
