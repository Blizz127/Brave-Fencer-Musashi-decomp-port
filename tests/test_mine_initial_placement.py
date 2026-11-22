"""Exercise initial actor placement, live list mutation and returned initializer status."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
ROOM = ROOT / "src/overlays/sc02_0039"


def test_initial_placement_lists_and_pool(tmp_path):
    program = tmp_path / "mine-placements"
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-I", str(ROOT / "include"), "-I", str(ROOM), "-I", str(ROOM.parent / "main_0012"),
                    str(ROOM / "8012c750.c"), str(ROOM / "8013e4b4.c"),
                    str(ROOT / "tests/mine_initial_placement_probe.c"),
                    "-o", str(program)], check=True)
    subprocess.run([str(program)], check=True)
