"""Check the mine actor integration, callback mutations and terrain flags."""
import os
from pathlib import Path
import shlex
import subprocess


def test_mine_actor_step(tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    executable = tmp_path / "mine-actor-step"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2",
        "-Wall", "-Wextra", "-Werror", "-UNDEBUG", "-I", root / "include", "-I", scene,
        root / "tests/mine_actor_step_probe.c", scene / "8012cc88.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
