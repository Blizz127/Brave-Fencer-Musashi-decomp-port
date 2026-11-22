"""Compare script-ring wrap, operand widths, and the live request reference."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_mine_script_queue_words_and_operand(tmp_path):
    overlay = ROOT / "src/overlays/sc02_0039"
    probe = tmp_path / "queue-probe"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", overlay, ROOT / "tests/mine_script_queue_probe.c",
        overlay / "8017a094.c", overlay / "80179b74.c", "-o", probe,
    ], check=True, timeout=45)
    subprocess.run([probe], check=True, timeout=10)
