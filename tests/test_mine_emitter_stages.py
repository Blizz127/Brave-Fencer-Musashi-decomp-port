"""Check the mine parent and child stage callbacks on synthetic actor records."""
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_mine_emitter_stages(tmp_path):
    overlay = ROOT / "src/overlays/sc02_0039"
    program = tmp_path / "emitter_stages"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")),
        "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
        "-I", ROOT / "include", "-I", overlay,
        ROOT / "tests/mine_emitter_stages_probe.c",
        overlay / "8017e758.c", overlay / "8017ee9c.c", "-o", program,
    ], check=True)
    subprocess.run([program], check=True)
