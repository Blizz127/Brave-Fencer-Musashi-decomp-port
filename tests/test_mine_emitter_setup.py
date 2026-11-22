"""Check the original mine parent initialization with synthetic model service outcomes."""
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_mine_emitter_setup(tmp_path):
    overlay = ROOT / "src/overlays/sc02_0039"
    program = tmp_path / "emitter_setup"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")),
        "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
        "-I", ROOT / "include", "-I", overlay,
        ROOT / "tests/mine_emitter_setup_probe.c", overlay / "8017f050.c",
        "-o", program,
    ], check=True)
    subprocess.run([program], check=True)
