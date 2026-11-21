"""Compile and exercise the production SDL controller sampler without disc data."""
from pathlib import Path
import shlex
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]


def test_two_virtual_sdl_controllers(tmp_path):
    config = shutil.which("sdl2-config")
    if config is None:
        pytest.skip("SDL2 development files are required")
    flags = shlex.split(subprocess.check_output([config, "--cflags", "--libs"], text=True))
    exe = tmp_path / "host_gamepad"
    subprocess.run([
        "cc", "-std=c99", "-Wall", "-Wextra", "-Werror",
        "-I", str(ROOT / "pc_port/include"), str(ROOT / "tests/host_gamepad_probe.c"),
        str(ROOT / "pc_port/host_gamepad.c"), "-o", str(exe), *flags,
    ], check=True)
    result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=15)
    if result.returncode == 77:
        pytest.skip(result.stdout.strip())
    assert result.returncode == 0, result.stderr
