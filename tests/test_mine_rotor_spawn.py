"""Check the four ordered mine spawn requests with generated room-table values."""
import ctypes as ct
from pathlib import Path
import shutil
import struct
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]
ROOM = ROOT / "src/overlays/sc02_0039"
POINTER = ct.c_void_p
READ = ct.CFUNCTYPE(ct.c_int, POINTER, ct.c_int16, ct.c_uint, ct.POINTER(ct.c_uint16))
SPAWN = ct.CFUNCTYPE(ct.c_int32, POINTER, ct.c_int16, ct.c_int16, POINTER)


class Services(ct.Structure):
    _fields_ = [("context", POINTER), ("read", READ), ("spawn", SPAWN)]


@pytest.fixture(scope="module", params=["gcc", "clang"])
def issue(request, tmp_path_factory):
    compiler = shutil.which(request.param)
    if not compiler:
        pytest.skip(f"{request.param} unavailable")
    lib = tmp_path_factory.mktemp("mine-rotor") / "rotor.so"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-fPIC", "-shared",
        "-I", str(ROOT / "include"), str(ROOM / "8017de00.c"), "-o", str(lib),
    ], check=True)
    func = ct.CDLL(str(lib)).mine_rotor_spawn_children
    func.argtypes = [POINTER, ct.POINTER(Services)]
    func.restype = ct.c_int
    return func


@pytest.mark.parametrize("stop", range(5))
def test_reloads_row_and_preserves_issued_prefix(issue, stop):
    for seed in range(24):
        origin = bytes((seed * 19 + n * 23) & 255 for n in range(268))
        actor = ct.create_string_buffer(origin, len(origin))
        address = ct.addressof(actor)
        events = []
        rows = [((seed + i * 7) % 23) - 11 for i in range(5)]
        struct.pack_into("<h", actor, 0x70, rows[0])

        def read(_, row, column, output):
            events.append(("read", row, column))
            if column >= stop:
                return 0
            output[0] = ((row * 3329) ^ (seed * 241 + column * 563)) & 65535
            return 1

        def spawn(_, kind, option, parent):
            call = len(events) // 2
            events.append(("spawn", kind, option, parent))
            struct.pack_into("<h", actor, 0x70, rows[call + 1])
            return -seed if call % 2 else 0

        services = Services(None, READ(read), SPAWN(spawn))
        assert issue(address, ct.byref(services)) == int(stop == 4)
        expected = []
        for i in range(stop):
            row = rows[i]
            expected.append(("read", row, i))
            word = (((row * 3329) ^ (seed * 241 + i * 563)) + 1024 * i) & 65535
            option = word - 65536 if word >= 32768 else word
            expected.append(("spawn", 0x199, option, address))
        if stop < 4:
            expected.append(("read", rows[stop], stop))
        assert events == expected
        remaining = bytearray(origin)
        struct.pack_into("<h", remaining, 0x70, rows[stop])
        assert bytes(actor) == remaining


def test_missing_provider_leaves_actor_untouched(issue):
    actor = ct.create_string_buffer(bytes(range(67)) * 4, 268)
    original = bytes(actor)
    touched = []
    def record(*_):
        touched.append(True)
        return 0
    for services in (Services(None, READ(), SPAWN(record)),
                     Services(None, READ(record), SPAWN())):
        assert not issue(ct.addressof(actor), ct.byref(services))
    assert not issue(ct.addressof(actor), None)
    complete = Services(None, READ(record), SPAWN(record))
    assert not issue(None, ct.byref(complete))
    assert not touched
    assert bytes(actor) == original
