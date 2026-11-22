"""Exercise the room ring's two intensity routines with synthetic actor and ring records."""
import os
from pathlib import Path
import shlex
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("replace_child", [False, True], ids=["linked", "wrapper-entry"])
def test_room_33a_intensity(tmp_path, replace_child):
    overlay = ROOT / "src/overlays/sc03_0116"
    exe = tmp_path / "room_33a_intensity"
    sources = [overlay / "80181844.c"]
    options = []
    if replace_child:
        options.append("-DROOM_33A_WRAPPER_TEST")
    else:
        sources.append(overlay / "80181864.c")
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2",
        "-Wall", "-Wextra", "-Werror", "-UNDEBUG", *options, "-I", ROOT / "include",
        "-I", overlay, ROOT / "tests/room_33a_intensity_probe.c", *sources, "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
