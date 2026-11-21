"""Verify card completion follows committed video edges from the selected clock."""
import os
from pathlib import Path
import shlex
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


def function(source, declaration):
    start = source.index(declaration)
    end = source.index("\n}\n", start) + len("\n}\n")
    return source[start:end]


@pytest.fixture(scope="module")
def card_epoch_probe(tmp_path_factory):
    directory = tmp_path_factory.mktemp("card-epoch")
    port = ROOT / "pc_port"
    source = (port / "native_boot.c").read_text()
    player = (port / "card/player_card.c").read_text()
    frame_cycles = next(line for line in source.splitlines()
                        if line.startswith("#define MUSASHI_GUEST_FRAME_CYCLES "))
    providers = "\n".join([frame_cycles, function(source, "static int guest_clock_collect("),
                           function(source, "static int epoch_collect(")])
    (directory / "providers.inc").write_text(providers)
    (directory / "advance.inc").write_text(function(source, "static int advance_epoch("))
    (directory / "pulse.inc").write_text(function(player, "void musashi_player_card_pulse("))
    program = directory / "card-epoch"
    subprocess.run([*shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-Wall",
                    "-Wextra", "-Werror", "-UNDEBUG", "-DMUSASHI_WITH_BFM_PLAT",
                    "-I", str(directory), "-I", str(port / "include"), "-I", str(port / "card"),
                    str(ROOT / "tests/native_card_epoch_probe.c"), str(port / "device_epoch.c"),
                    str(port / "card/card_service.c"), str(port / "card/card_image.c"),
                    "-o", str(program)], check=True)
    return program


@pytest.mark.parametrize("scenario", [
    "selected-guest", "selected-host", "stop-before", "stop-during",
    "partial-refusal", "stale-source", "early-wait", "owner",
])
def test_card_epoch(card_epoch_probe, scenario):
    subprocess.run([str(card_epoch_probe), scenario], check=True, timeout=5)
