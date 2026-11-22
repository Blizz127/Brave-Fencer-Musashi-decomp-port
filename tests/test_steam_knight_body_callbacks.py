"""Exercise the Steam Knight body callbacks and check their 32-bit MIPS build."""

from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc02_0005"
SOURCES = (SCENE / "80186adc.c", SCENE / "8018415c.c")


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_body_callbacks(tmp_path, compiler):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")

    executable = tmp_path / "body-callbacks-probe"
    subprocess.run([
        cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all",
        "-I", ROOT / "include", "-I", SCENE, *SOURCES,
        ROOT / "tests/steam_knight_body_callbacks_probe.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)


@pytest.fixture(scope="module")
def mips_objects(tmp_path_factory):
    clang = shutil.which("clang")
    if clang is None:
        pytest.skip("clang unavailable")
    targets = subprocess.check_output([clang, "--print-targets"], text=True)
    if "mipsel" not in targets.split():
        pytest.skip("clang lacks the little-endian MIPS target")

    output = tmp_path_factory.mktemp("body-callbacks-mips")
    for source in SOURCES:
        subprocess.run([
            clang, "--target=mipsel-linux-gnu", "-mabi=32", "-std=c89",
            "-pedantic-errors", "-ffreestanding", "-nostdinc", "-O2",
            "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
            "-c", source, "-o", output / (source.stem + ".o"),
        ], check=True)
    return output


def test_body_callbacks_freestanding_mips(mips_objects):
    for source in SOURCES:
        assert (mips_objects / (source.stem + ".o")).stat().st_size > 0


def test_mips_restore_references_both_records(mips_objects):
    obj = mips_objects / "80186adc.o"
    for program in ("llvm-nm", "nm"):
        reader = shutil.which(program)
        if reader is None:
            continue
        result = subprocess.run([reader, "-u", obj], text=True, capture_output=True)
        if result.returncode == 0:
            symbols = {line.split()[-1] for line in result.stdout.splitlines() if line.strip()}
            assert {"D_801961B8", "D_801961E0"} <= symbols
            return
    pytest.skip("no nm reader for the MIPS object")
