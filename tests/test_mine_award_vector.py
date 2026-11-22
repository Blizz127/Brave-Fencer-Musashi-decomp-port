"""Check the mine award movement routine's return value and ordered effects."""
import os
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[1]
ROOM = ROOT / "src/overlays/sc02_0039"


def test_vector_movement_and_planar_stop(tmp_path):
    executable = tmp_path / "mine-award-vector"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", ROOM, ROOM / "80171cc4.c",
        ROOT / "tests/mine_award_vector_probe.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
