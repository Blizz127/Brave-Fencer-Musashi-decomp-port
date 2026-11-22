"""Check the forest controller's two callback tables and grounded position update."""
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src" / "overlays" / "sc01_0077"


@pytest.mark.parametrize(
    "compiler,flags",
    [
        ("gcc", ["-O2", "-flto", "-Werror=lto-type-mismatch"]),
        ("clang", ["-O2"]),
        ("gcc", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
    ],
    ids=["gcc", "clang", "ubsan"],
)
def test_forest_ground_attachment(tmp_path, compiler, flags):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} is unavailable")
    executable = tmp_path / "ground-attachment"
    sources = [SCENE / "801627e8.c", SCENE / "80162834.c", SCENE.parent / "main_0012" / "801628f4.c"]
    subprocess.run(
        [
            compiler, "-std=c11", *flags, "-Wall", "-Wextra", "-Werror",
            "-I", str(ROOT / "include"), Path(__file__).with_name("forest_ground_attachment_probe.c"),
            *sources, "-o", executable,
        ],
        check=True,
    )
    subprocess.run([executable], check=True)
