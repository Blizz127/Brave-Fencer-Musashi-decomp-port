"""Exercise all five collision model selections using instrumented external services."""
from pathlib import Path
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.skipif(sys.platform != "linux", reason="maps a fixed guest-address fixture on Linux")
def test_contact_model_selection(tmp_path):
    sources = ROOT / "src/overlays/sc01_0077"
    binary = tmp_path / "contact"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2", "-I", ROOT / "include",
        ROOT / "tests/forest_contact_selection_probe.c", sources / "80135480.c",
        sources / "80135888.c", "-o", binary,
    ], check=True)
    result = subprocess.run([binary])
    if result.returncode == 77:
        pytest.skip("guest address unavailable")
    result.check_returncode()
