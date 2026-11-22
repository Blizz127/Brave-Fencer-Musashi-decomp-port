"""Check the mine child initializer and its installed two-stage dispatcher."""
import os
from pathlib import Path
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def test_mine_child_setup(tmp_path):
    overlay = ROOT / "src/overlays/sc02_0039"
    probe = ROOT / "tests/mine_child_setup_probe.c"
    options = ["-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
               "-I", ROOT / "include", "-I", overlay]
    for name, extra in (("setup", []), ("installed", [
        "-DBFM_TEST_CHILD_SELECTOR=1", overlay / "8017ee9c.c"
    ])):
        executable = tmp_path / name
        subprocess.run([*shlex.split(os.environ.get("CC", "cc")), *options,
                        probe, overlay / "8017ea20.c", *extra,
                        "-o", executable], check=True)
        subprocess.run([executable], check=True)
