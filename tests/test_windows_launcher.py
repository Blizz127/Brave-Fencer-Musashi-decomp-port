"""Exercise the package launcher with a synthetic Windows CLI and temporary profile."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]


def test_disc_selection_preserves_literal_windows_paths(tmp_path):
    zig = shutil.which("zig")
    wine = shutil.which("wine64")
    if not zig or (os.name != "nt" and not wine):
        pytest.skip("requires Zig and Windows CMD or Wine64")

    def guest(path):
        if os.name == "nt":
            return str(path.resolve())
        return "Z:" + str(path.resolve()).replace("/", "\\")

    install = tmp_path / "Games (Win64) 100% - literal %MUSASHI_TEST_NAME%"
    profile = tmp_path / "Profile (User) 50% - literal %MUSASHI_TEST_NAME%"
    config = tmp_path / "AppData (Roaming) 20%" / "bfm-port"
    nearby = install / "disc" / "First (US) 50% complete.cue"
    alternate = profile / "Games/brave-fencer-musashi/disc/Second (US) 75%.cue"
    for image in (nearby, alternate):
        image.parent.mkdir(parents=True)
        image.write_bytes(b"fixture")
    config.mkdir(parents=True)
    (install / "launch.cmd").write_bytes(
        (ROOT / "tools/windows/launch.cmd").read_bytes().replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
    )
    source = tmp_path / "player.c"
    source.write_text(r'''#include <stdio.h>
#include <string.h>
int main(int argc, char **argv) {
    if (argc != 3 || strcmp(argv[1], "--disc")) return 60;
    FILE *file = fopen(argv[2], "rb");
    if (!file) return 61;
    int first = fgetc(file);
    fclose(file);
    if (first != 'f') {
        puts("could not start from your disc image");
        return 2;
    }
    printf("selected=%s\n", argv[2]);
    return 0;
}
''')
    subprocess.run(
        [zig, "cc", "-target", "x86_64-windows-gnu", "-O2", "-s", str(source),
         "-o", str(tmp_path / "player.exe")], check=True,
    )
    shutil.copyfile(tmp_path / "player.exe", install / "musashi_native_boot.exe")
    driver = tmp_path / "driver.cmd"
    driver.write_bytes(
        b'@echo off\r\nsetlocal DisableDelayedExpansion\r\n'
        b'set "APPDATA=%CASE_APPDATA%"\r\nset "USERPROFILE=%CASE_PROFILE%"\r\n'
        b'set "BFM_DISC=%CASE_DISC%"\r\nset "BFM_DISC_DIR="\r\n'
        b'"%CASE_PACKAGE%\\launch.cmd"\r\n'
    )
    env = dict(os.environ, CASE_PACKAGE=guest(install), CASE_PROFILE=guest(profile),
               CASE_APPDATA=guest(config.parent), MUSASHI_TEST_NAME="must stay literal")
    command = ["cmd.exe", "/d", "/c", "driver.cmd"]
    if os.name != "nt":
        env.update(WINEPREFIX=str(tmp_path / "wine"), WINEARCH="win64", WINEDEBUG="-all",
                   WINEDLLOVERRIDES="mscoree,mshtml=")
        env.pop("DISPLAY", None)
        env.pop("WAYLAND_DISPLAY", None)
        command = [wine, "cmd", "/d", "/c", "driver.cmd"]
    remembered = config / "disc.txt"
    unrelated = config / "existing-save.txt"
    unrelated.write_bytes(b"original")

    def run(override=""):
        return subprocess.run(command, env=dict(env, CASE_DISC=override), cwd=tmp_path,
                              capture_output=True, timeout=120).returncode

    def selected(path):
        assert "selected=" + guest(path) in (config / "last-run.log").read_text().splitlines()
        assert remembered.read_text().strip() == guest(path)
        assert unrelated.read_bytes() == b"original"

    assert run() == 0
    selected(nearby)
    remembered.write_text(guest(tmp_path / "missing (old) 10%.cue") + "\n")
    assert run() == 0
    selected(nearby)
    nearby.unlink()
    assert run() == 0
    selected(alternate)
    nearby.write_bytes(b"fixture")
    assert run() == 0
    selected(alternate)
    assert run(guest(tmp_path / "missing 100%.cue")) == 66
    assert remembered.read_text().strip() == guest(alternate)
    damaged = tmp_path / "broken (US) 50%.bin"
    damaged.write_bytes(b"incomplete")
    assert run(guest(damaged)) == 2
    assert not remembered.exists()
    assert run() == 0
    selected(nearby)
