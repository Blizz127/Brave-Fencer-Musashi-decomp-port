from pathlib import Path
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_attachment_slots(compiler, tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    opts = ["-Werror=lto-type-mismatch"] if compiler == "gcc" else []
    exe = tmp_path / "attachment-slots"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-flto", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", *opts,
        "-I", root / "include", "-I", scene, scene / "80129010.c",
        root / "tests/mine_attachment_slots_probe.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
