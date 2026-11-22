from pathlib import Path
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_mine_focus_step(compiler, tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    extra = ["-Werror=lto-type-mismatch"] if compiler == "gcc" else []
    output = tmp_path / "mine-focus-step"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-flto", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", *extra,
        "-I", root / "include", "-I", scene, scene / "8015094c.c",
        root / "src/overlays/main_0012/801508f8.c",
        root / "tests/mine_focus_step_probe.c", "-o", output,
    ], check=True)
    subprocess.run([output], check=True)
