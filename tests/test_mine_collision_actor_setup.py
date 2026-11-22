"""Exercise the mine constructor with generated actor records and mutable model keys."""
import ctypes as ct
import shutil
import struct
import subprocess
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[1]
OVERLAY = ROOT / "src/overlays/sc02_0039"
U32 = ct.c_uint32
PTR = ct.c_void_p
ALLOCATE = ct.CFUNCTYPE(U32, PTR)
REMOVE = ct.CFUNCTYPE(None, PTR, PTR)
CONFIGURE = ct.CFUNCTYPE(None, PTR, U32, ct.c_int)
SPAWN = ct.CFUNCTYPE(None, PTR, ct.c_int16, ct.c_int16, PTR)
PREPARE = ct.CFUNCTYPE(None, PTR, U32)
LINK = ct.CFUNCTYPE(None, PTR, U32, U32)
LOOKUP = ct.CFUNCTYPE(PTR, PTR, U32)


class Services(ct.Structure):
    _fields_ = [
        ("context", PTR), ("box_address", U32), ("allocate", ALLOCATE),
        ("remove_actor", REMOVE), ("configure", CONFIGURE), ("spawn", SPAWN),
        ("prepare_child", PREPARE), ("link_child", LINK), ("actor", LOOKUP),
        ("model_flags", LOOKUP),
    ]


@pytest.fixture(scope="module", params=["gcc", "clang"])
def setup_actor(request, tmp_path_factory):
    compiler = shutil.which(request.param)
    if not compiler:
        pytest.skip(f"{request.param} unavailable")
    library = tmp_path_factory.mktemp("mine-setup") / "setup.so"
    subprocess.run([compiler, "-std=c11", "-O2", "-flto", "-Wall", "-Wextra", "-Werror",
                    "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(OVERLAY / "8017de84.c"), "-o", str(library)], check=True)
    routine = ct.CDLL(str(library)).mine_setup_actor
    routine.argtypes = [PTR, ct.POINTER(Services)]
    routine.restype = ct.c_int
    return routine


class Fixture:
    def __init__(self, variant):
        self.actor = (ct.c_uint8 * 268)(*((i * 29 + 7) % 256 for i in range(268)))
        self.parent = (ct.c_uint8 * 268)()
        self.model = {key: (ct.c_uint8 * 4)() for key in [17, 23, 41]}
        self.events = []
        self.allocation = 17
        struct.pack_into("<H", self.actor, 2, 65535)
        struct.pack_into("<H", self.actor, 0x70, 0x620 | variant)
        struct.pack_into("<I", self.actor, 0x64, 53)
        struct.pack_into("<I", self.parent, 0x20, 67)
        self.original = bytes(self.actor)
        self.services = Services(
            None, 0x81234568, ALLOCATE(self.allocate), REMOVE(self.remove),
            CONFIGURE(self.configure), SPAWN(self.spawn), PREPARE(self.prepare),
            LINK(self.link), LOOKUP(self.lookup_parent), LOOKUP(self.lookup_flags),
        )

    def allocate(self, _):
        self.events.append(("allocate",))
        return self.allocation

    def remove(self, _, actor):
        self.events.append(("remove", actor))

    def configure(self, _, model, which):
        self.events.append(("configure", model, which))
        struct.pack_into("<I", self.actor, 0x20, 23)

    def spawn(self, _, kind, mode, actor):
        self.events.append(("spawn", kind, mode, actor))
        struct.pack_into("<I", self.actor, 0x20, 41)
        struct.pack_into("<H", self.actor, 0x70, 0xabc7)

    def prepare(self, _, model):
        self.events.append(("prepare", model))
        struct.pack_into("<I", self.actor, 0x20, 23)
        struct.pack_into("<I", self.actor, 0x64, 71)

    def link(self, _, model, parent_model):
        self.events.append(("link", model, parent_model))
        struct.pack_into("<I", self.actor, 0x20, 41)
        struct.pack_into("<H", self.actor, 0x70, 0xabc7)

    def lookup_parent(self, _, key):
        self.events.append(("parent", key))
        return ct.addressof(self.parent) if key == 71 else None

    def lookup_flags(self, _, key):
        self.events.append(("flags", key))
        return ct.addressof(self.model[key]) if key in self.model else None

    def run(self, setup_actor):
        return setup_actor(self.actor, ct.byref(self.services))


@pytest.mark.parametrize("variant", range(4))
def test_model_setup_and_complete_actor_bytes(setup_actor, variant):
    f = Fixture(variant)
    assert f.run(setup_actor) == 1
    expected = bytearray(f.original)
    struct.pack_into("<I", expected, 0x20, 41)
    struct.pack_into("<H", expected, 0x70, 0xabc7)
    struct.pack_into("<H", expected, 0xfe, variant)
    struct.pack_into("<H", expected, 0x100, 0xbc0)
    struct.pack_into("<H", expected, 2, 0)
    struct.pack_into("<H", expected, 0x34, 0)
    if variant < 2:
        assert f.events == [("allocate",), ("configure", 17, variant), ("flags", 23),
                            ("spawn", 0x199, 2 + variant, ct.addressof(f.actor)), ("flags", 41)]
    else:
        assert f.events == [("allocate",), ("prepare", 17), ("parent", 71),
                            ("link", 23, 67), ("flags", 41)]
        struct.pack_into("<I", expected, 0x64, 71)
        struct.pack_into("<I", expected, 0x58, 0xc1234568)
        struct.pack_into("<H", expected, 0x5c, 0x4400)
        for offset, value in ((6, 0), (10, 144), (14, 0)):
            struct.pack_into("<H", expected, offset, value)
        expected[0x75] = 4
    assert bytes(f.actor) == expected
    assert {key: int.from_bytes(value, "little") for key, value in f.model.items()} == {
        17: 0, 23: 0x8040 if variant < 2 else 0, 41: 0x8000}


def test_allocation_failure_calls_only_removal(setup_actor):
    f = Fixture(3)
    f.allocation = 0
    f.services.prepare_child = PREPARE()
    assert f.run(setup_actor) == 1
    expected = bytearray(f.original)
    struct.pack_into("<I", expected, 0x20, 0)
    assert bytes(f.actor) == expected
    assert f.events == [("allocate",), ("remove", ct.addressof(f.actor))]


@pytest.mark.parametrize("address", [0, 0x80000001, 0xc1234568])
def test_child_requires_untagged_aligned_box(setup_actor, address):
    f = Fixture(2)
    f.services.box_address = address
    assert f.run(setup_actor) == 0
    assert f.events == [("allocate",)]
    assert bytes(f.actor)[0x58:0x5e] == f.original[0x58:0x5e]


def test_missing_model_lookup_stops_before_spawn(setup_actor):
    f = Fixture(0)
    f.services.model_flags = LOOKUP()
    assert f.run(setup_actor) == 0
    assert f.events == [("allocate",), ("configure", 17, 0)]
    assert bytes(f.actor)[2:4] == f.original[2:4]
