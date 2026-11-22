"""Exercise the forest kind-four selector and movement action with synthetic callbacks."""
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
ACTOR = ROOT / "src" / "overlays" / "sc01_0077"


@pytest.mark.parametrize("compiler,flags", [
    ("gcc", ["-O2"]),
    ("clang", ["-O2"]),
    ("gcc", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
], ids=["gcc", "clang", "ubsan"])
def test_kind4_action(tmp_path, compiler, flags):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} is unavailable")
    executable = tmp_path / "forest-kind4-action"
    subprocess.run([
        compiler, "-std=c11", *flags, "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        Path(__file__).with_name("forest_kind4_action_probe.c"),
        ACTOR / "8015bff4.c", ACTOR / "8015c128.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
