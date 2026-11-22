from pathlib import Path
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_frame_callbacks(compiler, tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    opts = ["-Werror=lto-type-mismatch"] if compiler == "gcc" else []
    exe = tmp_path / "frame-callbacks"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-flto", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", *opts,
        "-I", root / "include", "-I", scene, scene / "8013bc7c.c", scene / "8013bcdc.c", scene / "801287b8.c",
        root / "tests/mine_frame_callback_probe.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
