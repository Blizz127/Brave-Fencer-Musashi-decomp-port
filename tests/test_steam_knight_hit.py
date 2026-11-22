from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc02_0005"


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_attachment_hit(compiler, tmp_path):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} is unavailable")

    output = tmp_path / "steam_knight_hit"
    subprocess.run(
        [
            cc,
            "-std=c11",
            "-O2",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=undefined",
            "-fno-sanitize-recover=undefined",
            "-I", str(ROOT / "include"),
            "-I", str(SCENE),
            str(SCENE / "801853ec.c"),
            str(ROOT / "tests/steam_knight_hit_probe.c"),
            "-o", str(output),
        ],
        check=True,
    )
    subprocess.run([output], check=True)
