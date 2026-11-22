from pathlib import Path
import shutil
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_actor_template_initialization(tmp_path, compiler):
    if shutil.which(compiler) is None:
        pytest.skip(f"{compiler} unavailable")
    root = Path(__file__).resolve().parents[1]
    executable = tmp_path / "actor_template_init"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-fstrict-aliasing", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", "-I", root / "include",
        root / "src/overlays/sc02_0005/8012c354.c",
        root / "tests/actor_template_init_probe.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
