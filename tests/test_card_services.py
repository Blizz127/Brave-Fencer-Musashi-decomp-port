"""Exercise the image, BIOS file operations and host storage with invented cards."""
import hashlib
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


def test_async_read_receipts(tmp_path):
    binary = compile_probe(tmp_path, "receipt", FIXTURES / "service_probe.c",
                           CARD / "card_service.c", CARD / "card_image.c")
    (tmp_path / "card-store").mkdir()
    subprocess.run([binary, "receipts"], cwd=tmp_path, check=True)


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


@pytest.fixture(scope="module")
def directory_probe(tmp_path_factory):
    return compile_probe(tmp_path_factory.mktemp("directory-probe"), "directory",
                         ROOT / "tests/card_save_directory_probe.c",
                         CARD / "card_save_directory.c", CARD / "card_image.c",
                         PLATFORM / "bfm_plat_config.c", PLATFORM / "bfm_plat_ini.c",
                         PLATFORM / "bfm_plat_storage.c", platform_storage=True)


@pytest.mark.skipif(sys.platform != "linux", reason="uses the POSIX save path")
def test_save_directory(directory_probe):
    subprocess.run([sys.executable, ROOT / "tests/test_card_save_directory.py", directory_probe],
                   check=True)


@pytest.fixture(scope="module")
def card_samples(directory_probe, tmp_path_factory):
    root = tmp_path_factory.mktemp("card-samples")
    samples = {}
    for name in ("formatted", "preferred", "previous"):
        path = root / name
        args = [directory_probe, "--make-card", path]
        if name != "formatted":
            args.append(name)
        subprocess.run(args, check=True)
        samples[name] = path.read_bytes()
        assert len(samples[name]) == 128 * 1024
    fresh = samples["formatted"]
    samples["zero"] = bytes(len(fresh))
    samples["short"] = fresh[:-1]
    samples["long"] = fresh + b"!"
    # Free block data can differ even on a card with no directory entries.
    samples["used-empty"] = fresh[:-1] + b"!"
    return samples


@pytest.mark.skipif(sys.platform != "linux", reason="uses the POSIX save path")
def test_save_directory_override_precedes_config(directory_probe, tmp_path):
    configured = tmp_path / "configured"
    explicit = tmp_path / "override"
    config = tmp_path / "config.ini"
    config.write_text(f"[storage]\nsave_dir = {configured}\n")
    proc = subprocess.run([directory_probe, "512", explicit, "", "", "", config],
                          cwd=tmp_path, check=True, capture_output=True, text=True)
    assert proc.stdout.splitlines() == ["1", str(explicit)]
    assert explicit.is_dir() and not configured.exists()


@pytest.mark.skipif(sys.platform != "linux", reason="uses XDG state directories and POSIX links")
@pytest.mark.parametrize(("preferred", "previous", "result", "selected"), [
    ("saved", "saved", 2, "preferred"),
    ("saved", "broken-link", 2, "preferred"),
    ("formatted", "saved", 2, "previous"),
    ("zero", "saved", 2, "preferred"),
    ("missing", "saved", 2, "previous"),
    ("missing", "used-empty", 2, "previous"),
    ("missing", "missing", 1, "preferred"),
    ("formatted", "missing", 1, "preferred"),
    ("missing", "zero", 2, "previous"),
    ("missing", "formatted", 1, "preferred"),
    ("short", "saved", 0, None),
    ("long", "saved", 0, None),
    ("broken-link", "saved", 0, None),
    ("formatted", "short", 0, None),
    ("formatted", "long", 0, None),
    ("formatted", "broken-link", 0, None),
    ("missing", "offline-directory", 0, None),
])
def test_configured_save_directory(directory_probe, card_samples, tmp_path,
                                   preferred, previous, result, selected):
    state = tmp_path / "state"
    dirs = {"preferred": tmp_path / "configured saves", "previous": state / "bfm-port"}
    config = tmp_path / "config.ini"
    config.write_text(f"[storage]\nsave_dir = {dirs['preferred']}\n")
    cases = (("preferred", preferred), ("previous", previous))
    for location, kind in cases:
        image = dirs[location] / "bfm_card0.mcd"
        if kind == "missing":
            continue
        if kind == "offline-directory":
            image.parent.parent.mkdir(parents=True)
            image.parent.symlink_to("offline-volume", target_is_directory=True)
            continue
        image.parent.mkdir(parents=True)
        if kind == "broken-link":
            image.symlink_to("absent-target")
        else:
            image.write_bytes(card_samples[location if kind == "saved" else kind])

    proc = subprocess.run([directory_probe, "512", "", state, "", "", config],
                          cwd=tmp_path, check=True, capture_output=True, text=True)
    status, path = proc.stdout.splitlines()
    assert int(status) == result
    if selected:
        assert path == str(dirs[selected]) and dirs[selected].is_dir()
    for location, kind in cases:
        image = dirs[location] / "bfm_card0.mcd"
        if kind == "missing":
            assert not os.path.lexists(image)
        elif kind == "offline-directory":
            assert os.readlink(image.parent) == "offline-volume" and not image.parent.exists()
        elif kind == "broken-link":
            assert os.readlink(image) == "absent-target" and not image.exists()
        else:
            assert image.is_file() and not image.is_symlink()
            original = card_samples[location if kind == "saved" else kind]
            assert image.stat().st_size == len(original)
            assert hashlib.sha256(image.read_bytes()).digest() == hashlib.sha256(original).digest()
