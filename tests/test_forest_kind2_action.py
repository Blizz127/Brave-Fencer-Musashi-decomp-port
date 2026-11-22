"""Run the forest action chain with synthetic actor, table and external services."""
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
ACTOR = ROOT / "src" / "overlays" / "sc01_0077"


@pytest.mark.parametrize(
    ("compiler", "flags"),
    [
        ("gcc", ["-O2"]),
        ("clang", ["-O3"]),
        ("gcc", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
    ],
    ids=["gcc", "clang", "ubsan"],
)
def test_forest_kind2_action(tmp_path, compiler, flags):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} is unavailable")
    probe = tmp_path / "forest-kind2-action"
    entries = ("8015ad3c", "8015ad78", "8015adb0", "80161104", "80161208", "801612b8", "8016130c")
    subprocess.run([
        compiler, "-std=c11", *flags, "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        ROOT / "tests" / "forest_kind2_action_probe.c", *(ACTOR / f"{pc}.c" for pc in entries),
        "-o", probe,
    ], check=True)
    subprocess.run([probe], check=True)
