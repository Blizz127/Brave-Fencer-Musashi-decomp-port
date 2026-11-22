from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
@pytest.mark.parametrize(("probe", "sources"), [
    ("steam_knight_particle_probe.c", ["8018b16c.c", "80187664.c"]),
    ("steam_knight_particle_wrapper_probe.c", ["80187664.c"]),
])
def test_steam_knight_particles(tmp_path, compiler, probe, sources):
    if shutil.which(compiler) is None:
        pytest.skip(f"{compiler} unavailable")
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0005"
    executable = tmp_path / "particle-probe"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all",
        "-I", root / "include", "-I", scene,
        *(scene / name for name in sources),
        root / "tests" / probe, "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
