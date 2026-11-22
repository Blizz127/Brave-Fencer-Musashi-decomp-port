"""Check the fixed-player storage and the installed native callback."""
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
ROOM = ROOT / "src/overlays/sc02_0039"


@pytest.mark.parametrize("compiler", ["gcc", "clang"])
def test_install_player_phase(compiler, tmp_path):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")
    driver = tmp_path / "install.c"
    driver.write_text(r"""
#include "mine_award_controller.h"
#include <assert.h>
#include <string.h>

u8 D_80126B58[576];
MineAwardPhase D_80127088;
u32 D_801270BC;
static unsigned visited;

static void first(u8 *actor) { assert(actor == D_80126B58); visited |= 1; }
static void second(u8 *actor) { assert(actor == D_80126B58); visited |= 2; }

int main(void)
{
    MineAwardPhase choices[] = {first, second, 0, first};
    u8 expected[sizeof D_80126B58];
    for (unsigned i = 0; i < sizeof choices / sizeof choices[0]; ++i) {
        for (unsigned k = 0; k < sizeof expected; ++k)
            D_80126B58[k] = (u8)(23 * k + i + 3);
        memcpy(expected, D_80126B58, sizeof expected);
        expected[0x214] = 0;
        D_801270BC = 0xa5b2c401u + i;
        unsigned before = visited;
        func_80174684(choices[i]);
        assert(visited == before);
        assert(D_80127088 == choices[i] && !D_801270BC);
        assert(!memcmp(D_80126B58, expected, sizeof expected));
        if (D_80127088)
            D_80127088(D_80126B58);
    }
    assert(visited == 3);
    return 0;
}
""")
    program = tmp_path / "mine-award-install"
    subprocess.run([
        cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", str(ROOT / "include"), "-I", str(ROOM), str(ROOM / "80174684.c"),
        str(driver), "-o", str(program),
    ], check=True)
    subprocess.run([str(program)], check=True)
