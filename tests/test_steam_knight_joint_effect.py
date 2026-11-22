from pathlib import Path
import subprocess
import tempfile
import pytest

ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc02_0005"


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_steam_knight_joint_effect(compiler):
    with tempfile.TemporaryDirectory() as output:
        program = Path(output) / "joint_effect"
        subprocess.run([
            compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=undefined", "-fno-sanitize-recover=all",
            "-I", str(ROOT / "include"), "-I", str(SCENE),
            str(SCENE / "80186f9c.c"),
            str(Path(__file__).with_name("steam_knight_joint_effect_probe.c")),
            "-o", str(program),
        ], check=True)
        subprocess.run([str(program)], check=True)
