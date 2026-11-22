"""Exercise the mine award actor lookup against its typed 96-slot pool."""
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]
ROOM = ROOT / "src/overlays/sc02_0039"


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_mine_award_actor_lookup(compiler, tmp_path):
    tool = shutil.which(compiler)
    if tool is None:
        pytest.skip(f"{compiler} unavailable")
    program = tmp_path / "lookup"
    subprocess.run([
        tool, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        "-I", ROOM, ROOT / "tests/mine_award_actor_lookup_probe.c", ROOM / "8014c118.c",
        "-o", program,
    ], check=True)
    subprocess.run([program], check=True)
