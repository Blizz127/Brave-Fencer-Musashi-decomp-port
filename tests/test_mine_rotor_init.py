"""Exercise both model allocations and the four ordered room-child requests."""
import ctypes as ct
from pathlib import Path
import shutil
import struct
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]
ROOM = ROOT / "src/overlays/sc02_0039"
PTR = ct.c_void_p
ALLOC = ct.CFUNCTYPE(ct.c_uint32, PTR)
REMOVE = ct.CFUNCTYPE(None, PTR, PTR)
CONFIGURE = ct.CFUNCTYPE(None, PTR, ct.c_uint32, ct.c_int)
RESOLVE = ct.CFUNCTYPE(PTR, PTR, ct.c_uint32)
READ = ct.CFUNCTYPE(ct.c_int, PTR, ct.c_int16, ct.c_uint, ct.POINTER(ct.c_uint16))
SPAWN = ct.CFUNCTYPE(ct.c_int32, PTR, ct.c_int16, ct.c_int16, PTR)


class ChildServices(ct.Structure):
    _fields_ = [("context", PTR), ("read", READ), ("spawn", SPAWN)]


class InitServices(ct.Structure):
    _fields_ = [
        ("context", PTR), ("alloc", ALLOC), ("remove", REMOVE), ("configure", CONFIGURE),
        ("resolve", RESOLVE), ("children", ChildServices)
    ]


@pytest.fixture(scope="module", params=["gcc", "clang"])
def initialize(request, tmp_path_factory):
    cc = shutil.which(request.param)
    if not cc:
        pytest.skip(f"{request.param} unavailable")
    path = tmp_path_factory.mktemp("rotor-init") / "model.so"
    subprocess.run([
        cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-fPIC", "-shared",
        "-I", str(ROOT / "include"), str(ROOM / "8017e428.c"), str(ROOM / "8017de00.c"),
        "-o", str(path),
    ], check=True)
    f = ct.CDLL(str(path)).mine_rotor_initialize
    f.argtypes = [PTR, ct.POINTER(InitServices)]
    f.restype = ct.c_int
    return f


def execute(initialize, first, second, missing, stop, seed):
    actor = ct.create_string_buffer(bytes((seed * 11 + i * 7) & 255 for i in range(268)), 268)
    model = {key: ct.create_string_buffer(bytes([key + seed % 16]) * 44, 44) for key in [1, 2, 3]}
    initial_models = {key: bytearray(bytes(value)) for key, value in model.items()}
    struct.pack_into("<H", actor, 2, 65535)
    struct.pack_into("<h", actor, 0x70, seed - 9)
    struct.pack_into("<I", actor, 0x20, 471)
    original = bytearray(bytes(actor))
    events = []
    children = []
    allocations = iter((first, second))

    def allocate(_):
        value = next(allocations)
        events.append(("allocate", value))
        return value

    def remove(_, p):
        events.append(("remove", p))
        struct.pack_into("<H", actor, 0x34, seed + 37)

    def configure(_, key, resource):
        events.append(("configure", key, resource))
        if resource == 0:
            struct.pack_into("<I", actor, 0x20, 3)
        else:
            for offset, value in [(6, seed + 17), (10, 65535 - seed), (14, seed)]:
                struct.pack_into("<H", actor, offset, value)

    def resolve(_, key):
        events.append(("lookup", key))
        return None if missing == key else ct.addressof(model[key])

    def read(_, row, col, out):
        if col >= stop:
            return 0
        out[0] = (row * 3127 + col * 179) & 65535
        return 1

    def spawn(_, kind, option, parent):
        children.append((kind, option, parent))
        return -27

    services = InitServices(
        None, ALLOC(allocate), REMOVE(remove), CONFIGURE(configure), RESOLVE(resolve),
        ChildServices(None, READ(read), SPAWN(spawn))
    )
    success = initialize(ct.byref(actor), ct.byref(services))
    expected_actor = original.copy()
    expected_models = initial_models
    struct.pack_into("<I", expected_actor, 0x20, first)
    expected_events = [("allocate", first)]
    expected_children = []
    if not first:
        expected_events.append(("remove", ct.addressof(actor)))
        struct.pack_into("<H", expected_actor, 0x34, seed + 37)
        expected_result = 1
    else:
        expected_events.extend([("configure", first, 0), ("lookup", 3)])
        struct.pack_into("<I", expected_actor, 0x20, 3)
        expected_result = 0
        if missing != 3:
            expected_events.extend([("lookup", 3), ("allocate", second)])
            flags = struct.unpack_from("<I", expected_models[3], 4)[0] | 0x8040
            struct.pack_into("<I", expected_models[3], 4, flags)
            struct.pack_into("<I", expected_models[3], 0x28, 0x01800180)
            if second:
                expected_events.extend([("configure", second, 1), ("lookup", second)])
                xyz = [seed + 17, 65535 - seed, seed]
                for j, coordinate in enumerate(xyz):
                    struct.pack_into("<H", expected_actor, 6 + j * 4, coordinate)
                if second != missing:
                    struct.pack_into("<HHH", expected_models[second], 8,
                                     xyz[0], (xyz[1] + 192) & 65535, (xyz[2] - 176) & 65535)
            if second == 0 or second != missing:
                for col in range(stop):
                    value = ((seed - 9) * 3127 + col * 179 + col * 1024) & 65535
                    packed = value - 65536 if value >= 32768 else value
                    expected_children.append((0x199, packed, ct.addressof(actor)))
                if stop == 4:
                    expected_result = 1
                    struct.pack_into("<H", expected_actor, 2, 0)
                    struct.pack_into("<H", expected_actor, 0x34, 0)

    assert success == expected_result
    assert events == expected_events
    assert children == expected_children
    assert bytes(actor) == expected_actor
    for key, record in model.items():
        assert bytes(record) == expected_models[key]


@pytest.mark.parametrize("first,second,missing,stop", [
    (0, 0, None, 4), (1, 0, None, 4), (1, 2, None, 4), (1, 3, None, 4),
    (1, 2, 3, 4), (1, 2, 2, 4), (1, 2, None, 0), (1, 2, None, 3),
])
def test_physical_model_setup(initialize, first, second, missing, stop):
    for seed in range(19):
        execute(initialize, first, second, missing, stop, seed)
