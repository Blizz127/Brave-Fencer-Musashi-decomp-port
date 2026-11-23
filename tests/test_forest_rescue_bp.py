"""Exercise the mounted forest reward's maximum and current BP calls together."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc03_0006"


def test_forest_rescue_bp(tmp_path):
    binary = tmp_path / "forest-rescue-bp"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", SCENE, ROOT / "tests/forest_rescue_bp_probe.c",
        SCENE / "8017cccc.c", SCENE / "8014bcec.c", SCENE / "8014bd24.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
