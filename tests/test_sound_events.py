"""Exercise three linked game sound services with generated records."""
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


SOURCES = ("8002f248.c", "800331d4.c", "8002dc68.c")


def test_sound_events(tmp_path):
    compiler = shlex.split(os.environ.get("CC") or "cc")
    if not shutil.which(compiler[0]):
        pytest.skip("a host C compiler is required")
    executable = tmp_path / "sound-events"
    subprocess.run([
        *compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
        "-I", str(ROOT / "include"), str(ROOT / "tests" / "sound_event_probe.c"),
        *(str(ROOT / "src" / "main" / source) for source in SOURCES),
        "-o", str(executable),
    ], check=True)
    subprocess.run([executable], check=True)


def test_timed_sound_services_require_instruction_boundaries(tmp_path):
    rows = json.loads((ROOT / "provenance/sound-events.json").read_text())["matches"]
    services = {0x800331D4, 0x8002F248, 0x8002DC68}
    checked = set()
    for row in rows:
        if row["vram"] in services:
            selection = lane.build_one(row, tmp_path, {}, {}, "clang")
            assert not selection["admitted"] and selection["rule"] == "timing"
            checked.add(row["vram"])
    assert checked == services
