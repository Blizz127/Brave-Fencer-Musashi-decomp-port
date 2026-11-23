"""Run the compiled PAC sector regression with manufactured data."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_compiled_pac_sector_decode(tmp_path):
    port = ROOT / "pc_port"
    executable = tmp_path / "pac-sector-decode"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-I", str(port), "-I", str(port / "include"),
        str(ROOT / "tests/pac_sector_decode_probe.c"), str(port / "stream_decode.c"),
        str(port / "stream_timing.c"), str(port / "stream_call.c"),
        str(port / "boot_memory.c"), "-lcrypto", "-o", str(executable),
    ], check=True, timeout=60)
    subprocess.run([str(executable)], check=True, timeout=10)
