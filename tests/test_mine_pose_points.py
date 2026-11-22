"""Read the currently selected native model and preserve both output pads."""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
ROOM = ROOT / "src/overlays/sc02_0039"


def test_local_points_follow_the_fixed_player_model(tmp_path):
    binary = tmp_path / "pose-points"
    sources = [ROOM / "801484b0.c", ROOM / "801485b8.c", ROOT / "tests/mine_pose_points_probe.c"]
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-I", str(ROOT / "include"), "-I", str(ROOM), *sources,
                    "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
