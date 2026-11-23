from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_shared_actor_callbacks(tmp_path, compiler):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc03_0029"
    sources = ["801466f0.c", "8014680c.c", "8015f89c.c",
               "8015fa78.c", "8015faac.c", "8015fe70.c"]
    probe = tmp_path / "twinpeak-action22"
    subprocess.run([
        cc, "-std=c11", "-O2", "-fstrict-aliasing", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", "-I", scene,
        *(scene / source for source in sources), root / "tests/twinpeak_action22_probe.c",
        "-o", probe,
    ], check=True)
    subprocess.run([probe], check=True)
