from pathlib import Path
import subprocess
import tempfile
import pytest

ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc02_0005"


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_steam_knight_joint_dispatch(compiler):
    with tempfile.TemporaryDirectory() as output:
        program = Path(output) / "joint_dispatch"
        subprocess.run([
            compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=undefined", "-fno-sanitize-recover=all",
            "-I", str(ROOT / "include"), "-I", str(SCENE),
            str(SCENE / "80189af4.c"), str(SCENE / "8018b300.c"),
            str(Path(__file__).with_name("steam_knight_joint_dispatch_probe.c")),
            "-o", str(program),
        ], check=True)
        subprocess.run([str(program)], check=True)
