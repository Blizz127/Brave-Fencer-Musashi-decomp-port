"""Exercise the mine display's reset, initialization, sprite building and callback ordering."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/overlays/sc02_0039"


def test_mine_hud_build_callbacks(tmp_path):
    exe = tmp_path / "mine-hud-build"
    sources = [SOURCE / name for name in ("8016e8f0.c", "8016e9ec.c", "8016ec0c.c")]
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", SOURCE, ROOT / "tests/mine_hud_glyph_probe.c",
        *sources, "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
