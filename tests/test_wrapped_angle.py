"""The matched caller uses the guest CPU for the timed angle difference."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


@pytest.fixture(scope="module")
def caller_table(tmp_path_factory):
    if not all(shutil.which(tool) for tool in ("clang", "nm")):
        pytest.skip("the generator requires clang and nm")
    out = tmp_path_factory.mktemp("angle")
    (out / "ir").mkdir()
    registry = json.loads((ROOT / "provenance/native-main.json").read_text())
    rows = {row["vram"]: row for row in registry["matches"] if row["region"] == "main"}
    difference = lane.build_one(rows[0x80012AB0], out, {}, {}, "clang")
    assert not difference["admitted"] and difference["rule"] == "timing"
    caller = lane.build_one(rows[0x80012A60], out, {}, {}, "clang")
    assert caller["admitted"], caller
    assert lane.emit_table([caller], out) == (0, 1)
    return out, caller["obj"]


def test_generated_caller_uses_guest_bridge(caller_table):
    out, _ = caller_table
    table = (out / "native_lane_table.c").read_text()
    assert "musashi_native_guest_call(0x80012AB0u, 2u, args, NULL)" in table
    assert "wrap_80012ab0" not in table


def test_mounted_angle_and_registered_caller(caller_table):
    if not sys.platform.startswith("linux"):
        pytest.skip("the native lane fixture uses the Linux guest RAM mapping")
    exe = ROOT / "extracted/disc/files/SLUS_007.26"
    if not exe.is_file():
        pytest.skip("the owner's US executable is required")
    out, caller = caller_table
    build = out / "build"
    subprocess.run(["cmake", "-S", ROOT, "-B", build, "-DMUSASHI_WITH_PSYCROSS=OFF"], check=True)
    subprocess.run(["cmake", "--build", build, "--target", "musashi_pc_smoke", "--parallel", "1"],
                   check=True)
    subprocess.run([build / "musashi_pc_smoke"], check=True)
    probe = out / "angle"
    subprocess.run([
        "cc", "-std=c11", "-O1", "-ffunction-sections", "-fdata-sections",
        "-Wl,--gc-sections", "-Iinclude", "-Ipc_port/include", "-Ipc_port",
        "-DMUSASHI_COMPILED_STREAM", "-DMUSASHI_LINKED_BIOS_EXCEPTION=1",
        "tests/mounted_angle_probe.c", out / "native_lane_table.c", caller,
        build / "libmusashi_pc_port.a", build / "libmusashi_shared.a", "-lcrypto", "-o", probe,
    ], cwd=ROOT, check=True)
    env = dict(os.environ, MUSASHI_CODE_IMAGE="", MUSASHI_NATIVE_LANE="1",
               MUSASHI_NATIVE_LANE_RANGE="", MUSASHI_NATIVE_LANE_DENY="")
    subprocess.run([probe, exe], cwd=out, env=env, check=True)
