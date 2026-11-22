"""Run the mine slot initializer with synthetic, directly addressable room records."""
import platform
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.skipif(platform.system() != "Linux" or platform.machine() not in ("x86_64", "i686"),
                    reason="the probe needs a fixed, low-address Linux executable")
@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_slot_setup_and_removal(tmp_path, compiler):
    cc = shutil.which(compiler)
    if not cc:
        pytest.skip(f"{compiler} unavailable")
    output = tmp_path / "slot"
    subprocess.run([
        cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-fno-strict-aliasing",
        "-fno-pie", "-no-pie", "-DMUSASHI_NATIVE_MEMSET_BINDING", "-I", ROOT / "include",
        ROOT / "src/overlays/sc02_0039/8012c890.c", ROOT / "src/main/80016714.c",
        ROOT / "tests/mine_actor_slot_probe.c", "-o", output,
    ], check=True)
    subprocess.run([output], check=True)
