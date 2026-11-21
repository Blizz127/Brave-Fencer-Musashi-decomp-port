"""Exercise the image, BIOS file operations and host storage with invented cards."""
import os
from pathlib import Path
import shlex
import subprocess
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]
CARD = ROOT / "pc_port" / "card"
PLATFORM = ROOT / "pc_port" / "platform"
FIXTURES = ROOT / "tests" / "card"


def compile_probe(tmp_path, name, *sources, platform_storage=False):
    cc = shlex.split(os.environ.get("CC", "cc"))
    binary = tmp_path / name
    args = [*cc, "-std=gnu11", "-O2", "-I", CARD, "-I", PLATFORM]
    if platform_storage:
        args.extend(["-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections"])
    subprocess.run([*args, *sources, "-o", binary], check=True)
    return binary


def test_image(tmp_path):
    binary = compile_probe(tmp_path, "image", FIXTURES / "image_probe.c", CARD / "card_image.c")
    subprocess.run([sys.executable, FIXTURES / "make_card_fixture.py"], cwd=tmp_path, check=True)
    subprocess.run([binary], cwd=tmp_path, check=True)


def test_file_operations(tmp_path):
    binary = compile_probe(tmp_path, "files", FIXTURES / "service_probe.c",
                           CARD / "card_service.c", CARD / "card_image.c")
    store = tmp_path / "card-store"
    store.mkdir()
    for phase in ("create", "reopen", "failure", "after-failure", "events"):
        subprocess.run([binary, phase], cwd=tmp_path, check=True)
        if phase == "create":
            saved = (store / "slot0.mcr").read_bytes()
        elif phase != "events":
            assert (store / "slot0.mcr").read_bytes() == saved


def test_file_vectors(tmp_path):
    binary = compile_probe(tmp_path, "vectors", FIXTURES / "vector_probe.c", CARD / "card_vector.c",
                           CARD / "card_service.c", CARD / "card_image.c")
    subprocess.run([binary], cwd=tmp_path, check=True)


@pytest.mark.skipif(sys.platform != "linux", reason="exercises POSIX fork and Linux process locks")
def test_storage_sessions(tmp_path):
    sources = [CARD / "card_host_storage.c", PLATFORM / "bfm_plat_storage.c"]
    bridge = compile_probe(tmp_path, "storage", FIXTURES / "storage_probe.c", *sources,
                           platform_storage=True)
    fork = compile_probe(tmp_path, "fork", FIXTURES / "storage_fork_probe.c", *sources,
                         platform_storage=True)
    subprocess.run([sys.executable, FIXTURES / "storage_exercise.py", bridge, fork], cwd=tmp_path, check=True)


@pytest.mark.skipif(sys.platform != "linux", reason="uses the POSIX save path")
def test_save_directory(tmp_path):
    binary = compile_probe(tmp_path, "directory", ROOT / "tests/card_save_directory_probe.c",
                           CARD / "card_save_directory.c")
    subprocess.run([sys.executable, ROOT / "tests/test_card_save_directory.py", binary], check=True)
