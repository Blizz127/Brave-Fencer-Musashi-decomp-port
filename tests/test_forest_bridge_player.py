from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_bridge_player_scene_callback(tmp_path):
    output = tmp_path / "bridge-player"
    scene = ROOT / "src" / "overlays" / "sc01_0077"
    resident = ROOT / "src" / "main"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        ROOT / "tests" / "forest_bridge_player_probe.c",
        scene / "8017e868.c", scene / "8017e9c0.c",
        resident / "80012b04.c", resident / "80012b58.c", "-o", output,
    ], check=True)
    subprocess.run([output], check=True)
