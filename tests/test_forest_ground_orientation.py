"""Exercise the orientation update with checked objects and synthetic service results."""

from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize(
    "compiler,flags",
    [
        ("gcc", ["-O2", "-flto", "-Werror=lto-type-mismatch"]),
        ("clang", ["-O2"]),
        ("gcc", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
    ],
    ids=["gcc", "clang", "ubsan"],
)
def test_forest_ground_orientation(tmp_path, compiler, flags):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} is unavailable")
    executable = tmp_path / "forest-ground-orientation"
    subprocess.run(
        [
            compiler, "-std=c11", *flags, "-Wall", "-Wextra", "-Werror",
            "-I", str(ROOT / "include"),
            Path(__file__).with_name("forest_ground_orientation_probe.c"),
            ROOT / "src" / "overlays" / "main_0012" / "80162968.c",
            "-o", executable,
        ],
        check=True,
    )
    subprocess.run([executable], check=True)
