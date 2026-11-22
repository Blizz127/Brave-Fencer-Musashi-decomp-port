from pathlib import Path
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_actor_links(compiler, tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    opts = ["-Werror=lto-type-mismatch"] if compiler == "gcc" else []
    exe = tmp_path / "actor-links"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-flto", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", *opts,
        "-I", root / "include", "-I", scene, scene / "80165ca0.c",
        root / "tests/mine_actor_links_probe.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
