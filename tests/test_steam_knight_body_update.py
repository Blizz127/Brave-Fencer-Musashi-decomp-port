"""Compile and exercise the Steam Knight body update with synthetic actors."""

from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc02_0005"
SOURCE = SCENE / "80183218.c"


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_body_update(tmp_path, compiler):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")

    executable = tmp_path / "body-update-probe"
    subprocess.run([
        cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all",
        "-I", ROOT / "include", "-I", SCENE, SOURCE,
        ROOT / "tests/steam_knight_body_update_probe.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)


def test_body_update_freestanding_mips(tmp_path):
    clang = shutil.which("clang")
    if clang is None:
        pytest.skip("clang unavailable")
    targets = subprocess.check_output([clang, "--print-targets"], text=True)
    if "mipsel" not in targets.split():
        pytest.skip("clang lacks the little-endian MIPS target")

    subprocess.run([
        clang, "--target=mipsel-linux-gnu", "-mabi=32", "-std=c89",
        "-pedantic-errors", "-ffreestanding", "-nostdinc", "-O2",
        "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        "-c", SOURCE, "-o", tmp_path / "body-update.o",
    ], check=True)
