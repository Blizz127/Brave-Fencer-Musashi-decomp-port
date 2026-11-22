from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
TOWER = ROOT / "src/overlays/sc02_0005"


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_dispose_actor_and_optional_model(tmp_path, compiler):
    driver = shutil.which(compiler)
    if not driver:
        pytest.skip(f"{compiler} unavailable")
    program = tmp_path / "dispose-probe"
    subprocess.run(
        [
            driver, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
            "-DMUSASHI_NATIVE_MEMSET_BINDING", "-I", ROOT / "include", "-I", TOWER,
            ROOT / "src/main/80016714.c", TOWER / "8012cae4.c",
            ROOT / "tests/steam_knight_actor_dispose_probe.c", "-o", program,
        ],
        check=True,
    )
    subprocess.run([program], check=True)
