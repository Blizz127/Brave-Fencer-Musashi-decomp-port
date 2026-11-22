"""Run the six Tower postfire camera routines with typed host camera storage."""
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src" / "overlays" / "sc02_0005"
UNITS = ("8017e26c", "80129ff4", "8012a018", "8012a048", "8012a094", "8012a100")

PROBE = r"""
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "tower_postfire_camera.h"
#include "forest_player_state.h"

_Static_assert(sizeof(FacingActor) == 0xa4, "camera extent");
_Static_assert(offsetof(FacingActor, target_distance) == 0xc, "distance target");
_Static_assert(offsetof(FacingActor, camera) == 0x3c, "eye vector");
_Static_assert(offsetof(FacingActor, focus) == 0x48, "focus vector");
_Static_assert(offsetof(FacingActor, facing) == 0xa0, "final field");
_Static_assert(offsetof(ForestPlayerState, position) == 4, "player position");
_Static_assert(sizeof(PostfireCameraRawPosition) == 8, "input vector");

FacingActor D_80126948, D_80126FA8;
ForestPlayerState D_80126B58;
u16 D_80126940;
s16 D_80126942, D_80126944;
u8 D_801150D6;
const PostfireCameraRawPosition D_801E2D90 = {-19, 321, -4096, 0x4793u};
const PostfireCameraRawPosition D_801E2D98 = {17, -32768, 3021, 0xc925u};

static void fill(void *destination, size_t size, unsigned seed)
{
    u8 *p = destination;
    size_t i;
    for (i = 0; i < size; ++i)
        p[i] = (u8)(i * 31 + seed);
}

static void expect_selection(u8 *p, u32 callback, u8 mode)
{
    p[0] = (u8)callback;
    p[1] = (u8)(callback >> 8);
    p[2] = (u8)(callback >> 16);
    p[3] = (u8)(callback >> 24);
    p[4] = mode;
    memset(p + 0x9c, 0, 8);
}

static void expect_copy(u8 *p)
{
    memcpy(p + 8, p + 0xc, 4);
    memcpy(p + 0x10, p + 0x14, 4);
    memcpy(p + 0x18, p + 0x20, 8);
    memcpy(p + 0x28, p + 0x2e, 6);
}

static void check_selected(void)
{
    FacingActor expected;
    u8 local[0xa4 + 7], wanted[sizeof local];
    unsigned i;
    D_80126B58.position[0] = 0xff81a53e;
    D_80126B58.position[1] = 0x821365b4;
    D_80126B58.position[2] = 0x70ddef03;
    for (i = 0; i < 4; ++i) {
        const u32 address = 0x89abcdefu - i;
        fill(local, sizeof local, i);
        memcpy(wanted, local, sizeof local);
        expect_selection(wanted + i, address, (u8)(241 + i));
        assert(func_8012A048(local + i, address, (u8)(241 + i)) == 0xff81);
        assert(!memcmp(local, wanted, sizeof wanted));
    }
    fill(&D_80126948, sizeof expected, 61);
    memcpy(&expected, &D_80126948, sizeof expected);
    expect_selection((u8 *)&expected, 0x89435299u, 0x41);
    assert(func_8012A018(0x89435299u, 0x241) == 0xff81);
    assert(!memcmp(&D_80126948, &expected, sizeof expected));
    expect_selection((u8 *)&expected, 0, 0);
    assert(func_80129FF4() == 0xff81u);
    assert(!memcmp(&D_80126948, &expected, sizeof expected));
    assert(D_80126940 == 0xff81 && (u16)D_80126942 == 0x8213 && (u16)D_80126944 == 0x70dd);
}

static void check_copy(void)
{
    u8 local[64], wanted[sizeof local];
    unsigned i;
    for (i = 0; i < 4; ++i) {
        u16 first;
        fill(local, sizeof local, i * 57 + 17);
        memcpy(wanted, local, sizeof local);
        memcpy(&first, local + i + 0x2e, sizeof first);
        expect_copy(wanted + i);
        assert(func_8012A094(local + i) == first);
        assert(!memcmp(local, wanted, sizeof wanted));
    }
}

static void check_parent(void)
{
    FacingActor camera, selection;
    ForestPlayerState player;
    u16 returned;
    fill(&D_80126948, sizeof selection, 11);
    fill(&D_80126FA8, sizeof camera, 19);
    memcpy(&player, &D_80126B58, sizeof player);
    memcpy(&camera, &D_80126FA8, sizeof camera);
    memcpy(&selection, &D_80126948, sizeof selection);
    returned = (u16)camera.target_offset.x;
    camera.target_distance = 500;
    camera.camera = (WideVector){-19, 321, -4096};
    camera.focus = (WideVector){17, -32768, 3021};
    expect_copy((u8 *)&camera);
    expect_selection((u8 *)&selection, 0, 0);
    D_801150D6 = 129;
    assert(func_8017E26C() == returned);
    assert(D_801150D6 == 3);
    assert(!memcmp(&camera, &D_80126FA8, sizeof camera));
    assert(!memcmp(&selection, &D_80126948, sizeof selection));
    assert(!memcmp(&player, &D_80126B58, sizeof player));
}

int main(void)
{
    unsigned mode;
    for (mode = 0; mode <= 255; ++mode) {
        func_8012A100((u8)mode);
        assert(D_801150D6 == mode);
    }
    check_selected();
    check_copy();
    check_parent();
    return 0;
}
"""


@pytest.mark.parametrize("compiler", ("gcc", "clang"))
def test_tower_postfire_camera(tmp_path, compiler):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")
    probe = tmp_path / "camera.c"
    probe.write_text(PROBE)
    binary = tmp_path / "tower-camera"
    sources = [SCENE / f"{unit}.c" for unit in UNITS]
    subprocess.run(
        [
            cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-fstrict-aliasing",
            "-fsanitize=undefined", "-fno-sanitize-recover=all", "-I", ROOT / "include",
            "-I", SCENE, probe, *sources, "-o", binary,
        ],
        check=True,
    )
    subprocess.run([binary], check=True)
