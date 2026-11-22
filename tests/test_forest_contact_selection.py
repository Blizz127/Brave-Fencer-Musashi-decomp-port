"""Exercise forest contact selection with synthetic guest handles and caller-owned models."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_contact_model_selection(tmp_path):
    sources = ROOT / "src/overlays/sc01_0077"
    binary = tmp_path / "contact"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2", "-I", ROOT / "include",
        ROOT / "tests/forest_contact_selection_probe.c", sources / "80135480.c",
        sources / "80135888.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
