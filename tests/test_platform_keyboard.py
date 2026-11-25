"""Exercise configured SDL keyboard bindings using synthetic host events."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]


def test_configured_sdl_keyboard(tmp_path):
    config = shutil.which("sdl2-config")
    if config is None:
        pytest.skip("SDL2 development files are required")
    flags = shlex.split(subprocess.check_output([config, "--cflags", "--libs"], text=True))
    exe = tmp_path / "platform_keyboard"
    subprocess.run([
        "cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-DBFM_PLAT_WITH_SDL_INPUT",
        "-I", str(ROOT / "pc_port/platform"), str(ROOT / "tests/platform_keyboard_probe.c"),
        str(ROOT / "pc_port/platform/bfm_plat_input.c"),
        str(ROOT / "pc_port/platform/backends/input_sdl.c"), "-o", str(exe), *flags,
    ], check=True)
    subprocess.run([str(exe)], check=True, timeout=15,
                   env={**os.environ, "SDL_VIDEODRIVER": "dummy"})
