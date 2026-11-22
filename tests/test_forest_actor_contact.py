"""Exercise the two forest actor searches with synthetic contact records."""
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize(
    "compiler,flags",
    [("cc", ["-O2"]), ("clang", ["-O2"]),
     ("cc", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"])],
    ids=["cc", "clang", "ubsan"],
)
def test_forest_actor_contact(tmp_path, compiler, flags):
    if shutil.which(compiler) is None:
        pytest.skip(f"{compiler} unavailable")
    source = ROOT / "src/overlays/sc01_0077"
    binary = tmp_path / "actor-contact"
    subprocess.run([
        compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", *flags, "-I", ROOT / "include",
        ROOT / "tests/forest_actor_contact_probe.c", source / "8014e5b4.c",
        source / "8014e83c.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
