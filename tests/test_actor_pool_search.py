from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_tower_actor_pool_search(tmp_path, compiler):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} unavailable")
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0005"
    probe = tmp_path / "actor-pool-search"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", "-I", root / "include",
        "-I", scene, scene / "8012c658.c", root / "tests/actor_pool_search_probe.c",
        "-o", probe,
    ], check=True)
    subprocess.run([probe], check=True)
