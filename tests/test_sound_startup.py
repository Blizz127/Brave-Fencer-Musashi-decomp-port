from pathlib import Path
import re
import subprocess
import sys

import pytest


@pytest.mark.skipif(sys.platform != "linux", reason="fixture uses ELF linker symbol assignments")
@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_sound_startup_state_and_calls(compiler, tmp_path):
    root = Path(__file__).resolve().parents[1]
    source = root / "src/main/8002c8f4.c"
    names = sorted(set(re.findall(r"\bD_(800[A-F0-9]{5})\b", source.read_text())))
    assert names
    bindings = []
    for address in names:
        offset = int(address, 16) - 0x800a0000
        assert 0 <= offset < 0x27d30
        bindings.append(f"-Wl,--defsym,D_{address}=sound_test_state+{offset}")
    program = tmp_path / "sound_startup"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fno-strict-aliasing", "-fno-pie", "-no-pie", "-fsanitize=undefined",
        "-fno-sanitize-recover=all", root / "tests/sound_startup_probe.c",
        *bindings, "-o", program,
    ], check=True)
    subprocess.run([program], check=True)
