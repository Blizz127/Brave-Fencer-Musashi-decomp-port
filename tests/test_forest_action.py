"""Check forest transition and animation state decisions without a disc."""
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize(
    ("compiler", "flags"),
    [
        ("gcc", ["-O2"]),
        ("clang", ["-O3"]),
        ("gcc", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
    ],
    ids=["gcc", "clang", "ubsan"],
)
def test_forest_action(tmp_path, compiler, flags):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} is unavailable")
    executable = tmp_path / "forest-action"
    subprocess.run(
        [
            compiler, "-std=c11", *flags, "-Wall", "-Wextra", "-Werror",
            "-I", str(ROOT / "include"), str(Path(__file__).with_name("forest_action_probe.c")),
            str(ROOT / "src/overlays/sc01_0077/80159c84.c"), "-o", str(executable),
        ],
        check=True,
    )
    subprocess.run([executable], check=True)
