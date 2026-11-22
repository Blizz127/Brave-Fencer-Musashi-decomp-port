"""Check the byte-sized award phase and mode across full actor records."""
import ctypes
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src" / "overlays" / "sc02_0039"


def test_award_actor_fields(tmp_path):
    library = tmp_path / "actor-fields.so"
    services = ["80171a1c", "80171a34", "80147060", "8014706c"]
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra",
        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
        *(str(SCENE / f"{service}.c") for service in services),
        "-o", str(library),
    ], check=True)
    functions = ctypes.CDLL(str(library))
    actor_pointer = ctypes.POINTER(ctypes.c_ubyte)
    advance = functions.func_80171A1C
    select = functions.func_80171A34
    mode_one = functions.func_80147060
    mode_two = functions.func_8014706C
    advance.argtypes = mode_one.argtypes = mode_two.argtypes = [actor_pointer]
    select.argtypes = [actor_pointer, ctypes.c_int32]
    advance.restype = select.restype = mode_one.restype = mode_two.restype = None

    for phase in range(256):
        original = bytearray((i * 39 + phase) & 255 for i in range(0x244))
        original[0x214] = phase
        actor = (ctypes.c_ubyte * len(original)).from_buffer_copy(original)

        advance(actor)
        original[0x214:0x217] = bytes(((phase + 1) & 255, 0, 0))
        assert bytes(actor) == original

        for value in (phase - 512, phase + 1024):
            actor[0x215], actor[0x216] = 173, 91
            select(actor, value)
            original[0x214:0x217] = bytes((value & 255, 0, 0))
            assert bytes(actor) == original

        for mode, set_mode in ((1, mode_one), (2, mode_two)):
            set_mode(actor)
            original[0x4D] = mode
            assert bytes(actor) == original
