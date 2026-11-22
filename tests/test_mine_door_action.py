"""Check action selection, transition masking and consumption after door callbacks."""
import os
from pathlib import Path
import shlex
import subprocess

def test_mine_door_action(tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    executable = tmp_path / "door-action"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2", "-Wall", "-Wextra",
        "-Werror", "-UNDEBUG", "-I", root / "include", "-I", scene,
        root / "tests/mine_door_action_probe.c", scene / "80150b28.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
