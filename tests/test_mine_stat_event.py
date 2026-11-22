"""Exercise the mine stat event's selector, recovery services and ordered award calls."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/overlays/sc02_0039"


def test_mine_stat_event_callbacks(tmp_path):
    exe = tmp_path / "mine-stat-event"
    sources = [SOURCE / name for name in ("80173ef8.c", "80174158.c", "80174188.c", "801741a8.c")]
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", SOURCE, ROOT / "tests/mine_stat_event_probe.c",
        *sources, "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
