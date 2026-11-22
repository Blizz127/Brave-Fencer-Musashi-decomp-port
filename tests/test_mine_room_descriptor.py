"""Check the mine's descriptor lookup with independent, mutable table values."""
import ctypes
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module", params=["gcc", "clang"])
def lookup(tmp_path_factory, request):
    cc = shutil.which(request.param)
    if not cc:
        pytest.skip(f"{request.param} unavailable")
    temp = tmp_path_factory.mktemp("mine-descriptor")
    data = temp / "table.c"
    data.write_text('#include "psx_types.h"\nu32 D_8017F0B0[158];\n')
    lib = temp / "lookup.so"
    subprocess.run([cc, "-std=gnu11", "-O2", "-fPIC", "-shared", "-I", ROOT / "include",
                    ROOT / "src/overlays/sc02_0039/801457a4.c", data, "-o", lib], check=True)
    obj = ctypes.CDLL(str(lib))
    obj.func_801457A4.argtypes = [ctypes.c_int32]
    obj.func_801457A4.restype = ctypes.c_uint32
    return obj.func_801457A4, (ctypes.c_uint32 * 158).in_dll(obj, "D_8017F0B0")


def test_lookup_uses_fresh_table_and_rejects_unlisted_ids(lookup):
    get, table = lookup
    accepted = {0x1010, 0x1052, 0x1077, 0x1094, 0x2013}
    for first, last in [(0x2015, 0x201a), (0x2055, 0x205b), (0x207a, 0x2080), (0x2097, 0x209d)]:
        accepted.update(range(first, last + 1))
    for salt in [0, 157]:
        for i in range(len(table)):
            table[i] = (i + 1) * 104729 + salt
        for room in range(65536):
            index = room & 0xfff if room in accepted else 0
            assert get(room) == table[index]
        assert get(-(2**31)) == get(-1) == get(2**31 - 1) == table[0]
    for room in sorted(accepted):
        index = room & 0xfff
        table[index] ^= 0x80000000
        assert get(room) == table[index]
