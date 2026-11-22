"""Forest player state, phase and kind dispatch using synthetic guest callback tables."""
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / "src" / "overlays" / "sc01_0077"


@pytest.mark.parametrize(
    ("compiler", "flags"),
    [
        ("gcc", ["-O2"]),
        ("clang", ["-O3"]),
        ("gcc", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
    ],
    ids=["gcc", "clang", "ubsan"],
)
def test_forest_player_state(tmp_path, compiler, flags):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} is unavailable")
    probe = tmp_path / "forest-player-state"
    subprocess.run(
        [
            compiler, "-std=c11", *flags, "-Wall", "-Wextra", "-Werror",
            "-I", str(ROOT / "include"),
            str(Path(__file__).with_name("forest_player_state_probe.c")),
            *(str(SOURCES / name) for name in ("80146360.c", "80159968.c", "80159a20.c")),
            "-o", str(probe),
        ],
        check=True,
    )
    subprocess.run([probe], check=True)
