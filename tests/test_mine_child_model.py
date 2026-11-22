"""Exercise the mine child's model record lookup and conditional feedback."""
import os
from pathlib import Path
import shlex
import subprocess


def test_mine_child_model(tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    program = tmp_path / "child-model"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2", "-Wall", "-Wextra",
        "-Werror", "-UNDEBUG", "-I", root / "include", "-I", scene,
        root / "tests/mine_child_model_probe.c", scene / "8017eed8.c", "-o", program,
    ], check=True)
    subprocess.run([program], check=True)
