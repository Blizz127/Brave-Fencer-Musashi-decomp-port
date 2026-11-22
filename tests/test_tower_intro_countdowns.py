"""Check the two Tower intro countdown callbacks with native C callers."""

from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src" / "overlays" / "sc02_0005"

# The read-only input's type belongs to this callback. Define its test storage
# in the same translation unit so the probe uses the source's actual type.
PARAMETER_UNIT = r"""
#include "8017fff0.c"

TowerIntroParameter D_801E2DE8;

void seed_parameter(void)
{
    unsigned i;
    for (i = 0; i < sizeof(D_801E2DE8.bytes); ++i)
        D_801E2DE8.bytes[i] = (u8)(i * 17 + 3);
}

int parameter_intact(void)
{
    unsigned i;
    for (i = 0; i < sizeof(D_801E2DE8.bytes); ++i)
        if (D_801E2DE8.bytes[i] != (u8)(i * 17 + 3))
            return 0;
    return 1;
}
"""

CALLER = r"""
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "tower_intro_player.h"

_Static_assert(offsetof(TowerIntroPlayer, timer) == 0x204, "timer offset");
_Static_assert(offsetof(TowerIntroPlayer, step) == 0x214, "step offset");
_Static_assert(sizeof(((TowerIntroPlayer *)0)->timer) == 4, "timer width");
_Static_assert(sizeof(((TowerIntroPlayer *)0)->step) == 1, "step width");

s32 D_8012695C, D_801270C8;
s16 D_80126968, D_80126978;
void seed_parameter(void);
int parameter_intact(void);

typedef u32 (*Countdown)(TowerIntroPlayer *);

static void check_call(Countdown callback, u32 timer, u8 step)
{
    TowerIntroPlayer player, expected;
    u32 result;
    s32 target_phase = -73, first = -1051;
    s16 second = 367, third = -91;

    memset(&player, 0xa6, sizeof(player));
    player.timer = timer;
    player.step = step;
    memcpy(&expected, &player, sizeof(player));
    D_801270C8 = target_phase;
    D_8012695C = first;
    D_80126968 = second;
    D_80126978 = third;
    seed_parameter();

    if (timer) {
        result = timer - 1;
        expected.timer = result;
    } else {
        result = (u32)step + 1;
        expected.step = (u8)result;
        expected.timer = 64;
        if (callback == func_8017FF8C) {
            target_phase = 19;
            first = 400;
            second = -341;
            third = 32;
        } else {
            target_phase = 20;
        }
    }

    assert(callback(&player) == result);
    assert(memcmp(&player, &expected, sizeof(player)) == 0);
    assert(D_801270C8 == target_phase);
    assert(D_8012695C == first && D_80126968 == second && D_80126978 == third);
    assert(parameter_intact());
}

static void check_linked_countdowns(void)
{
    TowerIntroPlayer player = {0};
    u32 remaining;

    player.step = 28;
    assert(func_8017FF8C(&player) == 29);
    assert(player.step == 29 && player.timer == 64 && D_801270C8 == 19);
    for (remaining = 64; remaining > 0; --remaining) {
        assert(func_8017FFF0(&player) == remaining - 1);
        assert(player.step == 29 && player.timer == remaining - 1 && D_801270C8 == 19);
    }
    assert(func_8017FFF0(&player) == 30);
    assert(player.step == 30 && player.timer == 64 && D_801270C8 == 20);
    assert(D_8012695C == 400 && D_80126968 == -341 && D_80126978 == 32);
    assert(parameter_intact());
}

int main(void)
{
    Countdown callbacks[] = {func_8017FF8C, func_8017FFF0};
    u32 timers[] = {0, 1, 2, 0x80000000u, 0xffffffffu};
    u8 steps[] = {0, 28, 29, 255};
    size_t i, j, k;
    for (i = 0; i < sizeof(callbacks) / sizeof(callbacks[0]); ++i)
        for (j = 0; j < sizeof(timers) / sizeof(timers[0]); ++j)
            for (k = 0; k < sizeof(steps) / sizeof(steps[0]); ++k)
                check_call(callbacks[i], timers[j], steps[k]);
    check_linked_countdowns();
    return 0;
}
"""


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_tower_intro_countdowns(compiler, tmp_path):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")
    parameter_unit = tmp_path / "parameter.c"
    parameter_unit.write_text(PARAMETER_UNIT)
    caller = tmp_path / "caller.c"
    caller.write_text(CALLER)
    binary = tmp_path / "tower-intro-countdowns"
    subprocess.run(
        [
            cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
            "-fstrict-aliasing", "-fsanitize=undefined", "-fno-sanitize-recover=all",
            "-I", ROOT / "include", "-I", SCENE,
            SCENE / "8017ff8c.c", parameter_unit, caller, "-o", binary,
        ],
        check=True,
    )
    subprocess.run([binary], check=True)
