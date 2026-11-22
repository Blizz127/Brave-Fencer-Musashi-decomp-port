from pathlib import Path
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_mine_collision_actor_active(compiler, tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    sources = [scene / name for name in ("8017e25c.c", "8017e2f4.c", "8017e500.c")]
    binary = tmp_path / "mine-actor-active"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-flto", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", "-I", root / "include",
        "-I", scene, *sources, root / "tests/mine_collision_actor_active_probe.c",
        "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
