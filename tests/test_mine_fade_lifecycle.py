from pathlib import Path
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_fade_state_transitions(compiler, tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    opts = ["-Werror=lto-type-mismatch"] if compiler == "gcc" else []
    exe = tmp_path / "mine-fade-lifecycle"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-flto", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", *opts,
        "-I", root / "include", "-I", scene, scene / "8013a448.c", scene / "8013a4c4.c", scene / "8013a380.c",
        scene / "8013a530.c", root / "tests/mine_fade_lifecycle_probe.c",
        "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
