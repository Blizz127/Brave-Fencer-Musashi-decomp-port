"""Open the player's two-file card in a fresh process and compare the full image."""
import hashlib
import os
from pathlib import Path
import shlex
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.skipif(os.name != "posix", reason="uses ELF section garbage collection")
def test_cross_process_card_files(tmp_path):
    compiler = shlex.split(os.environ.get("CC", "cc"))
    if not shutil.which(compiler[0]):
        pytest.skip("requires a C compiler")
    backend = ROOT / "pc_port" / "card"
    platform = ROOT / "pc_port" / "platform"
    program = tmp_path / "card-files"
    subprocess.run([
        *compiler, "-std=gnu11", "-O2", "-ffunction-sections", "-fdata-sections",
        "-I", backend, "-I", platform, ROOT / "tests/card_storage_integration.c",
        backend / "card_image.c", backend / "card_service.c",
        backend / "card_host_storage.c", platform / "bfm_plat_storage.c",
        "-Wl,--gc-sections", "-o", program,
    ], check=True)
    save_dir = tmp_path / "isolated"
    save_dir.mkdir()
    subprocess.run([program, "write", save_dir], check=True)
    card = save_dir / "bfm_card0.mcd"
    assert card.stat().st_size == 128 * 1024
    digest = hashlib.sha256(card.read_bytes()).digest()
    before = card.stat()
    subprocess.run([program, "read", save_dir], check=True)
    assert hashlib.sha256(card.read_bytes()).digest() == digest
    after = card.stat()
    assert (after.st_ino, after.st_mtime_ns) == (before.st_ino, before.st_mtime_ns)


@pytest.mark.skipif(os.name != "posix", reason="uses ELF section garbage collection")
def test_graceful_and_cancelled_card_writes(tmp_path):
    compiler = shlex.split(os.environ.get("CC", "cc"))
    if not shutil.which(compiler[0]):
        pytest.skip("requires a C compiler")
    port = ROOT / "pc_port"
    backend = port / "card"
    platform = port / "platform"
    program = tmp_path / "card-shutdown"
    subprocess.run([
        *compiler, "-std=gnu11", "-O2", "-UNDEBUG", "-ffunction-sections",
        "-fdata-sections", "-I", backend, "-I", platform, "-I", port / "include",
        "-I", ROOT / "include", ROOT / "tests/player_card_shutdown_probe.c",
        backend / "player_card.c", backend / "card_image.c", backend / "card_service.c",
        backend / "card_vector.c", backend / "card_host_storage.c",
        platform / "bfm_plat_storage.c", port / "boot_memory.c", port / "bios_events.c",
        "-Wl,--gc-sections", "-lcrypto", "-o", program,
    ], check=True)
    for exit_mode, expected in [("finish", "updated"), ("cancel", "original")]:
        directory = tmp_path / exit_mode
        directory.mkdir()
        subprocess.run([program, "create", directory], check=True)
        image = directory / "bfm_card0.mcd"
        original = image.read_bytes()
        subprocess.run([program, exit_mode, directory], check=True)
        assert (image.read_bytes() == original) == (exit_mode == "cancel")
        subprocess.run([program, expected, directory], check=True)
    errors = tmp_path / "errors"
    errors.mkdir()
    subprocess.run([program, "file-errors", errors], check=True)
    blank = tmp_path / "unformatted"
    blank.mkdir()
    image = blank / "bfm_card0.mcd"
    original = bytes(128 * 1024)
    image.write_bytes(original)
    before = image.stat()
    subprocess.run([program, "blank", blank], check=True)
    assert image.read_bytes() == original
    assert (image.stat().st_ino, image.stat().st_mtime_ns) == (before.st_ino, before.st_mtime_ns)
    subprocess.run([program, "format", blank], check=True)
    assert len(image.read_bytes()) == len(original) and image.read_bytes() != original
