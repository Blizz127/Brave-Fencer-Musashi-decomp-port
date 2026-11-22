"""Link the actor predicates, game flag lookup and event allocator with a synthetic game pool."""
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src" / "overlays" / "sc01_0077"

@pytest.mark.parametrize(("cc", "flags"), [
    ("gcc", ["-O2", "-flto", "-Werror=lto-type-mismatch"]),
    ("clang", ["-O3", "-flto"]),
    ("gcc", ["-O1", "-fsanitize=undefined", "-fno-sanitize-recover=all"]),
], ids=["gcc", "clang", "ubsan"])
def test_forest_actor_event(tmp_path, cc, flags):
    if not shutil.which(cc):
        pytest.skip(f"{cc} is unavailable")
    sources = [SCENE / filename for filename in
               ("801498e0.c", "80149aa8.c", "80149b54.c", "8014bef8.c")]
    executable = tmp_path / "event"
    subprocess.run([
        cc, "-std=c11", *flags, "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "include"),
        Path(__file__).with_name("forest_actor_event_probe.c"), *sources,
        ROOT / "src" / "main" / "80029178.c",
        ROOT / "src" / "overlays" / "main_0012" / "80146750.c",
        "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
