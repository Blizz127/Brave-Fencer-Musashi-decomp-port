"""Run the checked image-call admission point with invented frame addresses."""
import os
from pathlib import Path
import shlex
import subprocess
import sys

import pytest


@pytest.mark.skipif(sys.platform != "linux", reason="uses ELF section garbage collection")
def test_cd_image_frame(tmp_path):
    root = Path(__file__).resolve().parents[1]
    port = root / "pc_port"
    program = tmp_path / "cd-image-frame"
    compiler = shlex.split(os.environ.get("CC", "cc"))
    subprocess.run([*compiler, "-std=gnu11", "-O1", "-UNDEBUG", "-ffunction-sections",
                    "-fdata-sections", "-I", port / "include", "-I", root / "include",
                    root / "tests/cd_image_frame_probe.c", port / "boot_memory.c",
                    "-Wl,--gc-sections", "-o", program], check=True)
    subprocess.run([program], check=True)
