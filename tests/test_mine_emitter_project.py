"""Check the emitter projection service boundaries and its caller's use of the results."""
import os
from pathlib import Path
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def test_mine_emitter_project(tmp_path):
    overlay = ROOT / "src/overlays/sc02_0039"
    probe = ROOT / "tests/mine_emitter_project_probe.c"
    options = ["-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
               "-I", ROOT / "include", "-I", overlay]
    for name, extra in (("projection", []), ("scan_and_projection", [
        "-DBFM_TEST_EMITTER_SCAN=1", overlay / "8017e794.c"
    ])):
        executable = tmp_path / name
        subprocess.run([*shlex.split(os.environ.get("CC", "cc")), *options,
                        probe, overlay / "8017e910.c", *extra, "-o", executable],
                       check=True)
        subprocess.run([executable], check=True)
