"""Check the two mine actor movement leaves and their complete input state."""
import os
from pathlib import Path
import shlex
import subprocess


def test_mine_actor_motion(tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    exe = tmp_path / "mine_actor_motion"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-Wall", "-Wextra",
        "-Werror", "-O2", "-UNDEBUG", "-I", root / "include", "-I", scene,
        root / "tests/mine_actor_motion_probe.c", scene / "801473ec.c", scene / "80147460.c",
        "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
