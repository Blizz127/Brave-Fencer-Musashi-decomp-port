"""Drive normal native/CPU selection using code compiled from the matched C."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.skipif(sys.platform != "linux", reason="uses the Linux candidate toolchain and guest window")
def test_wrapped_angle_phases(tmp_path, monkeypatch):
    toolchains = Path(os.environ.get("PSYQ_TOOLCHAIN_ROOT", ROOT / "tools/psyq"))
    maspsx = ROOT / "tools/maspsx/maspsx.py"
    if not (toolchains / "gcc-2.7.2-psx/cc1").is_file() or not maspsx.is_file():
        pytest.skip("requires the candidate toolchain and maspsx")
    if not shutil.which("mips-linux-gnu-as"):
        pytest.skip("requires cross-binutils for MIPS")
    image = tmp_path / "angle.bin"
    subprocess.run([sys.executable, ROOT / "tools/build_candidate.py",
                    ROOT / "src/main/80012ab0.c", "--include", ROOT / "include",
                    "--symbol", "func_80012AB0", "--output", image], check=True)
    if not all(shutil.which(name) for name in ("clang", "ar", "nm", "cc")):
        pytest.skip("requires host lane and C compilers")
    selected = tmp_path / "selected"
    selected.write_text("80012a60\n80012ab0\n")
    out = tmp_path / "gen"
    generate = [sys.executable, ROOT / "tools/native_lane_gen.py", "--out", out,
                "--registry", ROOT / "provenance/native-main.json", "--only", selected,
                "--own-code-only", "--jobs", "1"]
    subprocess.run(generate, check=True)
    report = json.loads((out / "report.json").read_text())
    assert report["admitted"] == 2
    assert report["call_stubs"] == {"direct": 0, "interpreter_thunk": 1}
    selected.write_text("80012ab0\n")
    subprocess.run(generate, check=True)
    probe = tmp_path / "probe"
    subprocess.run(["cc", "-std=c99", "-O2", "-ffunction-sections", "-fdata-sections",
                    "-Wl,--gc-sections", "-I", ROOT / "pc_port", "-I", ROOT / "pc_port/include",
                    "-I", ROOT / "include",
                    ROOT / "tests/wrapped_angle_probe.c", ROOT / "pc_port/boot_memory.c",
                    ROOT / "pc_port/platform/linux.c", out / "native_lane_table.c",
                    out / "libmusashi_native_lane.a", "-lcrypto", "-o", probe], check=True)
    monkeypatch.delenv("MUSASHI_CODE_IMAGE", raising=False)
    monkeypatch.setenv("MUSASHI_NATIVE_LANE", "1")
    result = subprocess.run([probe, image], timeout=10)
    if result.returncode == 77:
        pytest.skip("fixed guest window unavailable")
    result.check_returncode()
