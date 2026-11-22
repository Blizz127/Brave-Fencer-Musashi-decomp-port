from pathlib import Path
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_mine_dialog_band(compiler, tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    extra = ["-Werror=lto-type-mismatch"] if compiler == "gcc" else []
    output = tmp_path / "mine-dialog-band"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-flto", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", *extra,
        "-I", root / "include", "-I", scene, scene / "801299c8.c",
        root / "tests/mine_dialog_band_probe.c", "-o", output,
    ], check=True)
    subprocess.run([output], check=True)
