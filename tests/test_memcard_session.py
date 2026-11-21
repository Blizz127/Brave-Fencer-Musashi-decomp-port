"""Cross-process card ownership and atomic replacement on a local Linux filesystem."""
from pathlib import Path
import select
import subprocess
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]


pytestmark = pytest.mark.skipif(not sys.platform.startswith("linux"), reason="uses GNU ld and POSIX file-size limits")


@pytest.fixture
def probe(tmp_path):
    probe = tmp_path / "probe"
    platform = ROOT / "pc_port/platform"
    subprocess.run([
        "cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-ffunction-sections",
        "-fdata-sections", "-Wl,--gc-sections", "-Wl,--wrap=renameat",
        "-I", str(platform), str(ROOT / "tests/memcard_session_probe.c"),
        str(platform / "bfm_plat_storage.c"), "-o", str(probe),
    ], check=True, timeout=60)
    return probe


def test_memcard_session(tmp_path, probe):
    saves = tmp_path / "saves"
    saves.mkdir()
    image = saves / "bfm_card0.mcd"
    original = b"\x42" * 131072

    def run(mode):
        subprocess.run([probe, mode, saves], check=True, timeout=10)

    with subprocess.Popen([probe, "hold", saves], stdin=subprocess.PIPE, stdout=subprocess.PIPE) as holder:
        try:
            ready, _, _ = select.select([holder.stdout], [], [], 10)
            assert ready and holder.stdout.readline() == b"ready\n"
            run("contend")
            assert image.read_bytes() == original
        finally:
            # An ordinary exit could hide failure to release the OS lock on a crash.
            holder.kill()
            holder.wait(timeout=10)

    names = set(saves.iterdir())
    for fault in ("short-write", "failed-rename"):
        run(fault)
        assert image.read_bytes() == original
        assert set(saves.iterdir()) == names
    run("save")
    assert image.read_bytes() == b"\x7a" * 131072
    assert set(saves.iterdir()) == names


@pytest.mark.parametrize("move_directory", [False, True])
def test_memcard_keeps_selected_directory(tmp_path, probe, move_directory):
    original = tmp_path / "first"
    other = tmp_path / "second"
    for base, value in [(original, 0x42), (other, 0x35)]:
        (base / "saves").mkdir(parents=True)
        (base / "saves/bfm_card0.mcd").write_bytes(bytes([value]) * 131072)

    selected = original / "saves"
    unrelated = other / "saves"
    new_cwd = original if move_directory else other
    with subprocess.Popen([probe, "relocate", "saves", new_cwd], cwd=original,
                          stdin=subprocess.PIPE, stdout=subprocess.PIPE) as owner:
        try:
            ready, _, _ = select.select([owner.stdout], [], [], 10)
            assert ready and owner.stdout.readline() == b"ready\n"
            if move_directory:
                selected = original / "preserved"
                (original / "saves").rename(selected)
                unrelated.rename(original / "saves")
                unrelated = original / "saves"
            owner.stdin.write(b"x")
            owner.stdin.flush()
            assert owner.wait(timeout=10) == 0
        finally:
            if owner.poll() is None:
                owner.kill()
                owner.wait(timeout=10)

    assert (selected / "bfm_card0.mcd").read_bytes() == b"\x7a" * 131072
    assert (unrelated / "bfm_card0.mcd").read_bytes() == b"\x35" * 131072


def test_forked_process_cannot_write_or_retain_the_slot(tmp_path, probe):
    image = tmp_path / "bfm_card0.mcd"
    original = bytes([0x42]) * 131072
    image.write_bytes(original)

    subprocess.run([probe, "fork-writer", tmp_path], check=True, timeout=10)
    assert image.read_bytes() == original
    with subprocess.Popen([probe, "exit-with-child", tmp_path],
                          stdin=subprocess.PIPE, stdout=subprocess.PIPE) as owner:
        assert owner.wait(timeout=10) == 0
        ready, _, _ = select.select([owner.stdout], [], [], 10)
        assert ready and owner.stdout.readline() == b"ready\n"
        subprocess.run([probe, "save", tmp_path], check=True, timeout=10)
        owner.stdin.close()
        ready, _, _ = select.select([owner.stdout], [], [], 10)
        assert ready and owner.stdout.readline() == b"done\n"
    assert image.read_bytes() == bytes([0x7a]) * 131072
