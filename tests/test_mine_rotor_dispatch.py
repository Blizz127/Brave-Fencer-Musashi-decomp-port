"""Check state selection and the order of the rotor's post-action synchronization."""
import ctypes as ct
from pathlib import Path
import shutil
import struct
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]
ROOM = ROOT / "src/overlays/sc02_0039"
PTR = ct.c_void_p
ACTION = ct.CFUNCTYPE(ct.c_int, PTR, PTR)


class Actions(ct.Structure):
    _fields_ = [("context", PTR), ("initial", ACTION), ("update", ACTION), ("sync", ACTION)]


@pytest.fixture(scope="module", params=["gcc", "clang"])
def dispatch(request, tmp_path_factory):
    cc = shutil.which(request.param)
    if not cc:
        pytest.skip(f"{request.param} unavailable")
    program = tmp_path_factory.mktemp("rotor-dispatch") / "rotor.so"
    subprocess.run([
        cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-shared", "-fPIC",
        "-I", str(ROOT / "include"), str(ROOM / "8017ddb0.c"), "-o", str(program),
    ], check=True)
    func = ct.CDLL(str(program)).mine_rotor_dispatch
    func.argtypes = [PTR, ct.POINTER(Actions)]
    func.restype = ct.c_int
    return func


def test_unknown_state_has_no_effect(dispatch):
    events = []
    def seen(*_):
        events.append(True)
        return 1
    ops = Actions(None, ACTION(seen), ACTION(seen), ACTION(seen))
    record = ct.create_string_buffer(bytes((i * 11) & 255 for i in range(268)), 268)
    for state in range(2, 65536):
        struct.pack_into("<H", record, 2, state)
        before = bytes(record)
        assert not dispatch(ct.addressof(record), ct.byref(ops))
        assert bytes(record) == before
    assert not events


@pytest.mark.parametrize("state", [0, 1])
@pytest.mark.parametrize("case", ["ok", "missing action", "action refuses", "missing sync", "sync refuses"])
def test_post_observes_finished_action(dispatch, state, case):
    record = ct.create_string_buffer(bytes((i * 19 + 13) & 255 for i in range(268)), 268)
    struct.pack_into("<H", record, 2, state)
    original = bytearray(bytes(record))
    address = ct.addressof(record)
    events = []

    def action(index, context, ptr):
        events.append((index, context, ptr, bytes(record)))
        struct.pack_into("<HI", record, 2, 35 + index, 97 + index)
        return int(case != "action refuses")

    def sync(context, ptr):
        events.append(("post", context, ptr, bytes(record)))
        struct.pack_into("<I", record, 0x20, 157)
        return int(case != "sync refuses")

    ops = Actions(651, ACTION(lambda c, p: action(0, c, p)), ACTION(lambda c, p: action(1, c, p)),
                  ACTION() if case == "missing sync" else ACTION(sync))
    if case == "missing action":
        setattr(ops, "update" if state else "initial", ACTION())
    assert dispatch(address, ct.byref(ops)) == int(case == "ok")
    expected = []
    if case != "missing action":
        expected.append((state, 651, address, bytes(original)))
        struct.pack_into("<HI", original, 2, 35 + state, 97 + state)
        if case not in ("action refuses", "missing sync"):
            expected.append(("post", 651, address, bytes(original)))
            struct.pack_into("<I", original, 0x20, 157)
    assert events == expected
    assert bytes(record) == original
    assert not dispatch(None, ct.byref(ops))
    assert not dispatch(address, None)
