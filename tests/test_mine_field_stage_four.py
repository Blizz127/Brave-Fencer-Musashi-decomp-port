"""Exercise the room-stage hook and the resident substate update in call order."""
import os
from pathlib import Path
import shlex
import subprocess


def test_mine_field_stage_four(tmp_path):
    root = Path(__file__).resolve().parents[1]
    executable = tmp_path / "mine_field_stage_four"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2", "-Wall", "-Wextra",
        "-Werror", "-UNDEBUG", root / "tests/mine_field_stage_four_probe.c",
        root / "src/overlays/sc02_0039/8012853c.c", root / "src/main/80011c10.c",
        "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
