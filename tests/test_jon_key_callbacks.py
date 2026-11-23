"""Check the order of the two actor services after Jon's key message."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc03_0054"


def test_jon_key_callbacks(tmp_path):
    binary = tmp_path / "jon-key"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", SCENE, ROOT / "tests/jon_key_probe.c",
        SCENE / "801f0048.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
