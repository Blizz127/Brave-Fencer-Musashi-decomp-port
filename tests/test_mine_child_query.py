"""Check the mine child query's mutable shape and its caller's feedback path."""
import os
from pathlib import Path
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def test_mine_child_query(tmp_path):
    overlay = ROOT / "src/overlays/sc02_0039"
    compiler = shlex.split(os.environ.get("CC", "cc"))
    options = ["-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
               "-I", ROOT / "include", "-I", overlay]
    probe = ROOT / "tests/mine_child_query_probe.c"
    query = overlay / "8017ef5c.c"
    for name, extra in (("query", []), ("query_and_caller", [
        "-DBFM_TEST_QUERY_CALLER=1", overlay / "8017eed8.c"
    ])):
        program = tmp_path / name
        subprocess.run([*compiler, *options, probe, query, *extra, "-o", program], check=True)
        subprocess.run([program], check=True)
