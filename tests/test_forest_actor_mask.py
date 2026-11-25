"""Check the callable types at each Giant Forest player mask call site."""
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src/overlays/sc01_0077"


def test_player_mask_declarations(tmp_path):
    # A separate-object link can accept an incompatible return or pointer type.
    # Compile each real caller and its callee as one C translation unit.
    pairs = (
        ("80159c84", "80172590"),
        ("8015b950", "801725a4"),
        ("8015a3c8", "80172608"),
        ("8017e8a4", "80172658"), ("8017e974", "80172664"),
    )
    units = []
    for caller, callee in pairs:
        unit = tmp_path / f"pair_{caller}.c"
        unit.write_text(f'#include "{caller}.c"\n#include "{callee}.c"\n')
        units.append(unit)
    subprocess.run([
        "cc", "-std=c11", "-Werror", "-fsyntax-only", "-I", ROOT / "include",
        "-I", SCENE, *units,
    ], check=True)
