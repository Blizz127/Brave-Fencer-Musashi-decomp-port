"""Run the MAIN/0010 decimal packing and glyph-pair writing controls."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_main0010_number_format_and_write(tmp_path):
    source = ROOT / "src/overlays/main_0010"
    binary = tmp_path / "main0010-number-probe"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", ROOT / "tests/main0010_number_probe.c",
        source / "800d2ca8.c", source / "800d2d10.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
