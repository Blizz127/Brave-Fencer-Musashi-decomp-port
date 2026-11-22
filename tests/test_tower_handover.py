"""Compile and exercise the Tower callback that restores gameplay controls."""
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src" / "overlays" / "sc02_0005"
FUNCTIONS = ("80180328", "80175414", "8014c6ac", "8016f0e4", "8017196c",
             "801511a8", "80148634", "80147060")

HARNESS = r"""
#include "tower_handover.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
extern u32 func_80180328(TowerIntroPlayer *);
s32 D_801270C8;
u16 D_80078EB4;
u8 D_800B9AAC[0x56];
MineFrameMode D_8011F7A8;
static u8 player[0x250] __attribute__((aligned(8)));
static u8 other[0x250] __attribute__((aligned(8)));
static u8 live_control;
static u8 *expected_actor;
static u32 final_result, visits;

void *bfm_tower_spawn_pointer(u32 address)
{
    if (address == 0x80126d17u) return player + 0x1bf;
    if (address == 0x800b9a17u) return &live_control;
    abort();
}

s32 bfm_tower_handover_ability_service(u8 *actor)
{
    assert(actor == expected_actor);
    assert(D_801270C8 == 22 && HANDOVER_HALF(D_800B9AAC, 0) == 0x7fff);
    assert(player[0x1bf] == 1 && actor[0x4d] == 1);
    ++visits;
    return (s32)final_result;
}

static void run(unsigned mode, unsigned active, unsigned living)
{
    unsigned i;
    u8 wanted_player[sizeof player], wanted_other[sizeof other];
    u8 *wanted = active ? wanted_player : wanted_other;
    MineFrameMode was_mode;
    u16 movement = (u16)((0xf19d + active - mode) & 0xffff);
    expected_actor = active ? player : other;
    memset(player, 0xe9, sizeof player);
    memset(other, 0x37, sizeof other);
    HANDOVER_SET_HALF(expected_actor, 0x3c, movement);
    memcpy(wanted_player, player, sizeof player);
    memcpy(wanted_other, other, sizeof other);
    memset(&D_8011F7A8, 0xd2, sizeof D_8011F7A8);
    D_8011F7A8.mode = (u8)mode;
    was_mode = D_8011F7A8;
    if (mode == 1) {
        was_mode.mode = 0;
        was_mode.reserved[0] = 0;
        was_mode.reserved[4] = 10;
        was_mode.reserved[5] = 55;
    }
    memset(D_800B9AAC, 0x76, sizeof D_800B9AAC);
    D_801270C8 = 21;
    D_80078EB4 = living ? 150 : 0;
    live_control = 0xe3;
    wanted_player[0x1bf] = 1;
    memset(wanted + 0x1f8, 0, 8);
    memset(wanted + 0x214, 0, 3);
    wanted[0x4d] = 1;
    HANDOVER_SET_HALF(wanted, 0x3c, movement & 0xfffe);
    memset(wanted + 0x3e, 0, 6);
    memset(wanted + 0xaa, 0, 4);
    HANDOVER_SET_HALF(wanted, 0xae, 0x8080);
    visits = 0;
    final_result = (mode & 1) ? (0x80000000u ^ (mode << 8)) : 12;
    assert(func_80180328((TowerIntroPlayer *)expected_actor) == final_result);
    assert(visits == 1 && live_control == (living ? 1 : 0xe3));
    for (i = 2; i < sizeof D_800B9AAC; ++i) assert(D_800B9AAC[i] == 0x76);
    assert(!memcmp(&D_8011F7A8, &was_mode, sizeof was_mode));
    assert(!memcmp(player, wanted_player, sizeof player));
    assert(!memcmp(other, wanted_other, sizeof other));
}

int main(void)
{
    unsigned mode, active, living;
    for (mode = 0; mode < 256; ++mode)
        for (active = 0; active < 2; ++active)
            for (living = 0; living < 2; ++living)
                run(mode, active, living);
    return 0;
}
"""


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_tower_restores_player_control(tmp_path, compiler):
    if not shutil.which(compiler):
        pytest.skip(f"{compiler} unavailable")
    fixture = tmp_path / "handover.c"
    fixture.write_text(HARNESS)
    app = tmp_path / "handover"
    subprocess.run(
        [compiler, "-std=c99", "-O2", "-Werror", "-I", ROOT / "include", "-I", SCENE,
         fixture, *(SCENE / f"{name}.c" for name in FUNCTIONS), "-o", app],
        check=True,
    )
    subprocess.run([app], check=True)
