"""Check pool scanning and the installed parent emitter's subtype gate."""
import os
from pathlib import Path
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def test_mine_emitter_scan(tmp_path):
    overlay = ROOT / "src/overlays/sc02_0039"
    probe = ROOT / "tests/mine_emitter_scan_probe.c"
    options = ["-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
               "-I", ROOT / "include", "-I", overlay]
    for name, extra in (("scan", []), ("parent_and_scan", [
        "-DBFM_TEST_EMITTER_CALLER=1", overlay / "8017e640.c"
    ])):
        executable = tmp_path / name
        subprocess.run([*shlex.split(os.environ.get("CC", "cc")), *options,
                        probe, overlay / "8017e794.c", *extra, "-o", executable],
                       check=True)
        subprocess.run([executable], check=True)
