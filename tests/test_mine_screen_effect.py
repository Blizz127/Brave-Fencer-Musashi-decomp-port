import ctypes as C
from pathlib import Path
import struct
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_mine_screen_effect(compiler, tmp_path):
    repo = Path(__file__).resolve().parents[1]
    scene = repo / "src/overlays/sc02_0039"
    library = tmp_path / "screen.so"
    options = ["-Werror=lto-type-mismatch"] if compiler == "gcc" else []
    subprocess.run([
        compiler, "-std=c11", "-O2", "-flto", "-fPIC", "-shared",
        "-Wall", "-Wextra", "-Werror", "-fsanitize=undefined", "-fsanitize-undefined-trap-on-error",
        *options, "-I", repo / "include", "-I", scene, scene / "8013a530.c",
        repo / "tests/mine_screen_effect_probe.c", "-o", library,
    ], check=True)
    lib = C.CDLL(str(library))
    U8=C.c_ubyte
    class Trace(C.Structure):
     _fields_=[('rows',C.c_uint32),('calls',(C.c_uint32*10)*3)]
    call=lib.native_probe_mapping
    call.argtypes=[C.POINTER(U8),C.POINTER(U8),C.POINTER(Trace),C.c_uint32,C.c_uint32,C.c_uint32];call.restype=C.c_uint32
    def padded(raw):
     arr=(U8*(len(raw)+18))(*([0xa5]*9+list(raw)+[0x5a]*9))
     return arr,C.cast(C.byref(arr,9),C.POINTER(U8))
    key=0x801fe000
    for address,base,length,success in [(key,key,56,True),(0,0,56,False),(key,0x1234,56,False),(key,key,55,False),(key,key,0,False),(key+1,key+1,56,False)]:
     work=bytearray(range(16));struct.pack_into('<I',work,4,address);struct.pack_into('<hh',work,12,-1234,2)
     effect=bytearray(56);struct.pack_into('<H',effect,24,2);struct.pack_into('<hh',effect,40,154,42);struct.pack_into('<hh',effect,52,138,26)
     wb,wp=padded(work);eb,ep=padded(effect);t=Trace()
     assert call(wp,ep,C.byref(t),0,base,length)==1
     assert bytes(wb[:9])==bytes([0xa5]*9) and bytes(wb[-9:])==bytes([0x5a]*9)
     assert bytes(eb[:9])==bytes([0xa5]*9) and bytes(eb[-9:])==bytes([0x5a]*9)
     expected=bytearray(work)
     if success:
      struct.pack_into('<hhh',expected,8,154,42,-210)
      assert list(t.calls[0])[:5]==[0x8013aa24,key,0x2000,0x2000,42]
     assert bytes(wb[9:-9])==expected and bytes(eb[9:-9])==effect and t.rows==success
     last=(C.addressof(wb)+9,C.addressof(eb)+9)
    t=Trace();assert call(None,None,C.byref(t),0,key,56)==0 and t.rows==0
