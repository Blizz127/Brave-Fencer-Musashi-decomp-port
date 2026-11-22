from pathlib import Path
import re
import subprocess
import sys

import pytest


@pytest.mark.skipif(sys.platform != "linux", reason="fixture maps guest addresses with Linux mmap")
@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_disc_stream_step(compiler, tmp_path):
    root = Path(__file__).resolve().parents[1]
    source = root / "src/main/80019af8.c"
    symbols = set(re.findall(r"\bD_(800[A-F0-9]{5})\b", source.read_text()))
    assert symbols
    assert all(0x800A0000 <= int(s, 16) < 0x800B0000 for s in symbols)
    bindings = [f"-Wl,--defsym,D_{s}=0x{s}" for s in sorted(symbols)]
    program = tmp_path / "stream_step"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fPIC", "-no-pie", "-fno-strict-aliasing", "-fsanitize=undefined",
        "-fno-sanitize-recover=all", "-Wl,--no-relax", *bindings,
        "-Wl,--defsym,func_8001A338=0x8001A338", source,
        root / "tests/game_stream_probe.c", "-o", program,
    ], check=True)
    subprocess.run([program], check=True, timeout=10)
