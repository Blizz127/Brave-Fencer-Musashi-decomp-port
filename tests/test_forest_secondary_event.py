"""Check the secondary forest actor event with synthetic providers and the real slot allocator."""
from pathlib import Path
import shutil
import subprocess

import pytest


def test_forest_secondary_event(tmp_path):
    compiler = shutil.which("cc")
    if compiler is None:
        pytest.skip("a C compiler is required")
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/main_0012"
    executable = tmp_path / "forest_secondary_event"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", root / "include", Path(__file__).with_name("forest_secondary_event_probe.c"),
        scene / "80149954.c", scene / "80146750.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)


def test_forest_positional_sound(tmp_path):
    compiler = shutil.which("cc")
    if compiler is None:
        pytest.skip("a C compiler is required")
    root = Path(__file__).resolve().parents[1]
    executable = tmp_path / "forest_positional_sound"
    subprocess.run([
        compiler, "-std=c11", "-O3", "-fstrict-aliasing", "-Wall", "-Wextra", "-Werror",
        "-I", root / "include", Path(__file__).with_name("forest_positional_sound_probe.c"),
        root / "src/overlays/main_0012/8012e5cc.c", root / "src/main/8002d4c8.c",
        "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
