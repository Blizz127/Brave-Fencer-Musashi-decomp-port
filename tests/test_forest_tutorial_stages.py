from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_forest_tutorial_stages(tmp_path):
    scene = ROOT / "src" / "overlays" / "sc01_0077"
    output = tmp_path / "forest-tutorial-stages"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        "-I", scene, ROOT / "tests" / "forest_tutorial_stages_probe.c",
        scene / "8017e8a4.c", scene / "8017e924.c", scene / "8017e974.c",
        scene / "8017ea2c.c", "-o", output,
    ], check=True)
    subprocess.run([output], check=True)
