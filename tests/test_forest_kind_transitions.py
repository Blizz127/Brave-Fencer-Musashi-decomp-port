"""Verify the three forest player transitions with synthetic game state."""
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize(
    "compiler,flags",
    [
        ("gcc", ["-O2"]),
        ("clang", ["-O2"]),
        ("gcc", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
    ],
    ids=["gcc", "clang", "ubsan"],
)
def test_forest_kind_transitions(tmp_path, compiler, flags):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} is unavailable")
    source = ROOT / "src/overlays/main_0012"
    executable = tmp_path / "forest-kind-transitions"
    subprocess.run(
        [
            compiler, "-std=c11", *flags, "-Wall", "-Wextra", "-Werror",
            "-I", str(ROOT / "include"), str(Path(__file__).with_name("forest_kind_transitions_probe.c")),
            str(source / "8015bdd0.c"), str(source / "8015bf48.c"), str(source / "8015c0c4.c"),
            "-o", str(executable),
        ],
        check=True,
    )
    subprocess.run([executable], check=True)
