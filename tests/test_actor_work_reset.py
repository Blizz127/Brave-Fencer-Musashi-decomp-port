import ctypes
import json
from pathlib import Path
import shutil
import subprocess
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_host_return_and_sparse_writes(tmp_path, compiler):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} unavailable")
    library = tmp_path / ("actor_reset.dll" if sys.platform == "win32" else "actor_reset.so")
    subprocess.run([
        compiler, "-std=c11", "-O2", "-shared", "-fPIC",
        "-I", str(ROOT / "include"), "-include", "actor_work_reset.h",
        str(ROOT / "src/main/8001c9d0.c"), "-o", str(library),
    ], check=True)
    reset = ctypes.CDLL(str(library)).func_8001C9D0
    reset.argtypes = [ctypes.c_void_p]
    reset.restype = ctypes.c_uint32
    # Four guard bytes on either side of the aligned 132-byte record.
    storage = (ctypes.c_uint32 * 35)()
    for seed in range(256):
        expected = bytearray((seed + 7 * i) % 256 for i in range(ctypes.sizeof(storage)))
        ctypes.memmove(storage, bytes(expected), len(expected))
        record = memoryview(expected)[4:-4]
        record[4:14] = bytes(10)
        record[16:22] = bytes(6)
        record[24:30] = (4096).to_bytes(2, "little") * 3
        record[32:36] = bytes(4)
        record[40:44] = (200).to_bytes(2, "little") * 2
        record[44:48] = bytes(4)
        record[128:132] = bytes(4)
        assert reset(ctypes.byref(storage, 4)) == 200
        assert bytes(storage) == expected


def test_atomic_registration_declines_separately_timed_stores(tmp_path):
    registry = json.loads((ROOT / "provenance/native-main.json").read_text())
    row = next(item for item in registry["matches"] if item["name"] == "func_8001C9D0")
    assert lane.build_one(row, tmp_path, {}, {}, "clang")["rule"] == "timing"
