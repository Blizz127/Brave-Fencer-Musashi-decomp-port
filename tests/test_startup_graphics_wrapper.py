from pathlib import Path
import subprocess

import pytest


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_graphics_startup_order_and_clear(compiler, tmp_path):
    root = Path(__file__).resolve().parents[1]
    main = root / "src/main"
    program = tmp_path / "startup_graphics_wrapper"
    flags = ["-Werror=lto-type-mismatch"] if compiler == "gcc" else []
    subprocess.run([
        compiler, "-std=c11", "-O2", "-flto", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=undefined", "-fno-sanitize-recover=all", *flags,
        "-DMUSASHI_NATIVE_MEMSET_BINDING", "-I", root / "include",
        main / "800141f0.c", main / "80016714.c", main / "800142c8.c",
        root / "tests/startup_graphics_wrapper_probe.c", "-o", program,
    ], check=True)
    subprocess.run([program], check=True)
