from pathlib import Path
import os
import shutil
import signal
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_tower_animation_refresh(tmp_path, compiler):
    if shutil.which(compiler) is None:
        pytest.skip(f"{compiler} unavailable")
    root = Path(__file__).resolve().parents[1]
    executable = tmp_path / "tower_animation_refresh"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-fstrict-aliasing", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", "-I", root / "include",
        root / "src/overlays/sc02_0005/8013240c.c",
        root / "tests/tower_animation_refresh_probe.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
    if os.name != "posix":
        return
    import resource

    for direction in ("+", "-"):
        result = subprocess.run(
            [executable, direction], cwd=tmp_path,
            preexec_fn=lambda: resource.setrlimit(resource.RLIMIT_CORE, (0, 0)),
        )
        assert result.returncode == -signal.SIGABRT
