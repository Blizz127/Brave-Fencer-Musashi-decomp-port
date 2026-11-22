"""Exercise the regional value/duration script from generated state."""
from pathlib import Path
import os
import shlex
import subprocess


def test_mine_script(tmp_path):
    root = Path(__file__).resolve().parents[1]
    room = root / "src/overlays/sc02_0039"
    program = tmp_path / "mine-script"
    subprocess.run([
        *shlex.split(os.environ.get("CC") or "cc"), "-std=c11", "-O2", "-Wall", "-Wextra",
        "-Werror", "-I", root / "include", "-I", room, room / "8013ca14.c",
        root / "tests/mine_step_script_probe.c", "-o", program,
    ], check=True)
    subprocess.run([program], check=True)
