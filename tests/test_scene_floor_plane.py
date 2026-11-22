"""Compile and execute the mine member's two plane-service routines."""
import os
from pathlib import Path
import shlex
import shutil
import struct
import subprocess
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]
REGION = ROOT / "src" / "overlays" / "sc02_0039"


def test_scene_floor_plane(tmp_path):
    compiler = shlex.split(os.environ.get("CC") or "cc")
    if not shutil.which(compiler[0]):
        pytest.skip("a host C compiler is required")
    profiles = [("portable", [])]
    if sys.platform.startswith("linux") and struct.calcsize("P") == 8:
        profiles.append(("high-address", ["-fPIE", "-pie", "-DBFM_TEST_HIGH_ADDRESSES"]))
    for name, flags in profiles:
        binary = tmp_path / name
        subprocess.run([
            *compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
            *flags, "-I", str(ROOT / "include"), "-I", str(REGION),
            str(ROOT / "tests" / "scene_floor_plane_probe.c"),
            str(REGION / "801347a0.c"), str(REGION / "80134a28.c"),
            "-o", str(binary),
        ], check=True)
        subprocess.run([binary], check=True)
