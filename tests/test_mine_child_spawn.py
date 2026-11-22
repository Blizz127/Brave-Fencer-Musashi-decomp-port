"""Compile the mine child-emitter source against synthetic callback cases."""
import os
import shlex
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def test_mine_child_spawn(tmp_path):
    overlay = ROOT / "src/overlays/sc02_0039"
    exe = tmp_path / "mine_child_spawn"
    compiler = shlex.split(os.environ.get("CC", "cc"))
    subprocess.run([
        *compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
        "-I", str(ROOT / "include"), "-I", str(overlay),
        str(ROOT / "tests/mine_child_spawn_probe.c"), str(overlay / "8017e640.c"),
        "-o", str(exe)
    ], check=True)
    subprocess.run([exe], check=True)
