from pathlib import Path
import subprocess
import sys

import pytest


@pytest.mark.skipif(sys.platform != "linux" or sys.byteorder != "little",
                    reason="uses ELF data symbols and fixed guest directory storage")
@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_resource_directory(compiler, tmp_path):
    program = tmp_path / "resource_directory"
    subprocess.run([
        compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fno-strict-aliasing", "-fno-pie", "-no-pie", "-fsanitize=undefined",
        "-fno-sanitize-recover=all", Path(__file__).with_name("resource_directory_probe.c"),
        "-Wl,--defsym,D_80063044=D_80062C24+1056",
        "-Wl,--defsym,D_80063074=D_80062C24+1104", "-o", program,
    ], check=True)
    subprocess.run([program], check=True)
