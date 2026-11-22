from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parent.parent


def test_forest_ability_release(tmp_path):
    binary = tmp_path / "ability-release"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-I", str(ROOT / "include"), str(ROOT / "tests/forest_ability_release_probe.c"),
        str(ROOT / "src/overlays/sc01_0077/80165ac8.c"), "-o", str(binary)
    ], check=True)
    subprocess.run([binary], check=True)
