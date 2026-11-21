"""Exercise the forest box classifier with controlled face and normalizer calls."""
from pathlib import Path
import subprocess


def test_forest_linked_box(tmp_path):
    root = Path(__file__).resolve().parents[1]
    exe = tmp_path / "forest-box"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", root / "include",
        root / "tests/forest_linked_box_probe.c", root / "src/overlays/sc01_0077/80135eb0.c",
        "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)


def test_forest_box_with_real_faces(tmp_path):
    root = Path(__file__).resolve().parents[1]
    overlay = root / "src/overlays/sc01_0077"
    exe = tmp_path / "forest-faces"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", root / "include",
        root / "tests/forest_box_faces_probe.c", overlay / "80135eb0.c", overlay / "80136334.c",
        overlay / "801365b8.c", overlay / "80136824.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
