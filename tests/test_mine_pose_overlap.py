"""Read each signed sample after earlier writes to the same backing storage."""
import ctypes
import itertools
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
ROOM = ROOT / "src/overlays/sc02_0039"


def test_local_point_inputs_and_outputs_may_overlap(tmp_path):
    binary = tmp_path / "pose-extract.so"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-shared", "-fPIC",
        "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "include"),
        str(ROOM / "801485b8.c"), "-o", str(binary),
    ], check=True)
    extract = ctypes.CDLL(str(binary)).func_801485B8
    extract.argtypes = [ctypes.c_void_p] * 3
    extract.restype = None

    seed = bytes((57 * i + 131) % 256 for i in range(24))
    for source, first, second in itertools.product(range(5, 9), repeat=3):
        expected = bytearray(seed)
        for index in range(6):
            sample = expected[source + index]
            output = (first if index < 3 else second) + 2 * (index % 3)
            expected[output:output + 2] = bytes((sample, 255 if sample >= 128 else 0))
        storage = (ctypes.c_ubyte * len(seed)).from_buffer_copy(seed)
        address = ctypes.addressof(storage)
        extract(address + source, address + first, address + second)
        assert bytes(storage) == expected, (source, first, second)
