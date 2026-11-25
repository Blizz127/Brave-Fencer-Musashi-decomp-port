"""The libgs TMD primitive admission windows match the licensed EXE."""
import hashlib
import re
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "extracted/disc/files/SLUS_007.26"


def test_windows_match_licensed_exe():
    if not EXE.exists():
        pytest.skip("pinned licensed EXE unavailable")
    generated = subprocess.run([sys.executable, str(ROOT / "tools/gen_libgs_prim_windows.py"), str(EXE)],
                               check=True, capture_output=True, text=True).stdout
    source = (ROOT / "pc_port/mips_formatter.c").read_text()
    assert generated in source
    spans = re.findall(r"\{0x([0-9a-f]{8})u, 0x([0-9a-f]{8})u, kLibgsPrim", source)
    assert spans == [("8004a27c", "8004cfec"), ("8004d6bc", "800509c0")]
    for lo, hi in spans:
        table = re.search(r"kLibgsPrim%s\[\] = \{([^}]*)\}" % lo.upper(), source).group(1)
        assert len(re.findall(r"0x[0-9a-f]{16}ull", table)) == -(-(int(hi, 16) - int(lo, 16)) // 256)


@pytest.mark.parametrize("address", (0x8004B710, 0x8004B8DC, 0x8004CD38, 0x8004CF18))
def test_changed_byte_invalidates_only_its_primitive_window(address):
    from runpy import run_path

    if not EXE.exists():
        pytest.skip("pinned licensed EXE unavailable")
    image = EXE.read_bytes()
    image_digest = hashlib.sha256(image).hexdigest()
    assert image_digest == "66371c3a7517e9eabd7cb6cf0c5abffe7296bd4bac29c85b8bf7bb9db349714a"

    windows = run_path(str(ROOT / "tools/gen_libgs_prim_windows.py"))["windows"]

    def digests(data):
        return {(start, index): digest
                for start, _, values in windows(data)
                for index, digest in enumerate(values)}

    text_base = int.from_bytes(image[24:28], "little")
    modified = bytearray(image)
    modified[2048 + address - text_base] ^= 1
    before, after = digests(image), digests(modified)
    assert before.keys() == after.keys()
    changed = {key for key in before if before[key] != after[key]}
    first_span = 0x8004A27C
    assert changed == {(first_span, (address - first_span) // 256)}
