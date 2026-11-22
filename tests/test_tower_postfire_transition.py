"""Exercise the Tower postfire callback, real request operations and heading arithmetic."""
from pathlib import Path
import shutil
import subprocess

import pytest


ROOT = Path(__file__).resolve().parents[1]
SCENE = ROOT / "src" / "overlays" / "sc02_0005"
UNITS = ("8017fcfc", "801399f0", "80139914", "8013a8b0", "80147060")

CALLER = r"""
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "tower_intro_player.h"
#include "tower_request.h"

_Static_assert(offsetof(TowerIntroPlayer, step) == 0x214, "script step offset");

enum { MODEL = 1, REQUEST, ATTACHMENT, LATE_MODEL };
static TowerIntroPlayer player;
static u8 objects[5][0x50];
static unsigned lookups[5], camera_calls;
static int retarget;
s32 D_801270C8;

static void write16(void *p, unsigned offset, u16 value)
{
    memcpy((u8 *)p + offset, &value, sizeof value);
}
static void write32(void *p, unsigned offset, u32 value)
{
    memcpy((u8 *)p + offset, &value, sizeof value);
}
static u16 read16(const void *p, unsigned offset)
{
    u16 value;
    memcpy(&value, (const u8 *)p + offset, sizeof value);
    return value;
}

void *bfm_tower_spawn_pointer(u32 token)
{
    assert(token >= MODEL && token <= LATE_MODEL);
    ++lookups[token];
    return objects[token];
}

/* Exercise the five joined units with a controlled boundary for the camera. */
u32 func_8017E26C(void)
{
    ++camera_calls;
    assert(D_801270C8 == 13);
    assert(((u8 *)&player)[0x4d] == 1);
    assert(request_word(objects[REQUEST], 0) == 0);
    assert(request_half(objects[REQUEST], 4) == 0);
    if (retarget)
        write32(&player, 0x20, LATE_MODEL);
    return 0x70afu;
}

static void reset(u16 heading, u16 kind, u8 status, u16 mode, u32 link)
{
    memset(&player, 0x73, sizeof player);
    memset(objects, 0x52, sizeof objects);
    memset(lookups, 0, sizeof lookups);
    retarget = 0;
    camera_calls = 0;
    D_801270C8 = 13;
    ((u8 *)&player)[0x4d] = 2;
    player.step = 255;
    write32(&player, 0x20, MODEL);
    write32(&player, 0x198, REQUEST);
    write16(objects[MODEL], 0x12, heading);
    write16(objects[LATE_MODEL], 0x12, 0xf123);
    write32(objects[REQUEST], 0, 17);
    write16(objects[REQUEST], 4, kind);
    write16(objects[REQUEST], 0x18, mode);
    objects[REQUEST][0x1f] = status;
    write32(objects[REQUEST], 0x40, link);
}

static void check_early_exit(u16 heading, u32 expected, unsigned query_reads)
{
    TowerIntroPlayer previous;
    u8 request_before[sizeof objects[REQUEST]];
    reset(heading, 13, 0, 1, ATTACHMENT);
    memcpy(&previous, &player, sizeof player);
    memcpy(request_before, objects[REQUEST], sizeof request_before);
    assert(func_8017FCFC(&player) == expected);
    assert(D_801270C8 == 13 && camera_calls == 0);
    assert(memcmp(&previous, &player, sizeof player) == 0);
    assert(memcmp(request_before, objects[REQUEST], sizeof request_before) == 0);
    assert(read16(objects[MODEL], 0x12) == expected);
    assert(lookups[REQUEST] == query_reads && lookups[ATTACHMENT] == 0);
}

static void check_signed_status(void)
{
    static const s32 replies[] = {0, 1, 127, -128, -1};
    unsigned i;
    for (i = 0; i < sizeof replies / sizeof replies[0]; ++i) {
        reset(0x800, 13, (u8)replies[i], 0, 0);
        assert(func_801399F0(objects[REQUEST]) == replies[i]);
    }
    write16(objects[REQUEST], 4, 4);
    assert(func_801399F0(objects[REQUEST]) == 0);
    write16(objects[REQUEST], 4, 14);
    assert(func_801399F0(objects[REQUEST]) == -1);
    write16(objects[REQUEST], 4, 0);
    assert(func_801399F0(objects[REQUEST]) == -1);
    write16(objects[REQUEST], 4, 4);
    write32(objects[REQUEST], 0, 0);
    assert(func_801399F0(objects[REQUEST]) == -1);
}

static void check_cleanup(u16 mode, u32 link)
{
    u8 before[sizeof objects[ATTACHMENT]], request_before[sizeof objects[REQUEST]];
    reset(0x800, 13, 1, mode, link);
    memcpy(before, objects[ATTACHMENT], sizeof before);
    memcpy(request_before, objects[REQUEST], sizeof request_before);
    memset(request_before, 0, 6);
    if (mode && link) {
        memset(before, 0, 2);
        memset(before + 4, 0, 4);
    }
    assert(func_80139914(objects[REQUEST]) == (s32)mode);
    assert(!memcmp(objects[ATTACHMENT], before, sizeof before));
    assert(!memcmp(objects[REQUEST], request_before, sizeof request_before));
    assert(lookups[ATTACHMENT] == (unsigned)(mode && link));
}

static void check_open(int switch_model, u8 stage)
{
    TowerIntroPlayer expected;
    reset(0x800, 13, 0x80, 0xffff, ATTACHMENT);
    retarget = switch_model;
    player.step = stage;
    memcpy(&expected, &player, sizeof player);
    ((u8 *)&expected)[0x4d] = 1;
    expected.step = (u8)(stage + 1);
    if (switch_model)
        write32(&expected, 0x20, LATE_MODEL);
    assert(func_8017FCFC(&player) == (switch_model ? 0x123u : 0x800u));
    assert(!memcmp(&expected, &player, sizeof player));
    assert(D_801270C8 == 14 && camera_calls == 1);
    assert(lookups[REQUEST] == 2 && lookups[ATTACHMENT] == 1);
    assert(read16(objects[MODEL], 0x12) == 0x800);
    assert(read16(objects[LATE_MODEL], 0x12) == (switch_model ? 0x123 : 0xf123));
    assert(request_word(objects[ATTACHMENT], 4) == 0);
    assert(request_half(objects[ATTACHMENT], 0) == 0);
}

int main(void)
{
    check_signed_status();
    check_cleanup(0, ATTACHMENT);
    check_cleanup(0xabcd, 0);
    check_cleanup(0xffff, ATTACHMENT);
    check_early_exit(0x7ff, 0x800, 0);
    check_early_exit(0x810, 0x80c, 0);
    check_early_exit(0x800, 0x800, 1);
    check_open(0, 23);
    check_open(0, 255);
    check_open(1, 255);
    return 0;
}
"""


@pytest.mark.parametrize("compiler", ("gcc", "clang"))
def test_tower_postfire_transition(tmp_path, compiler):
    cc = shutil.which(compiler)
    if cc is None:
        pytest.skip(f"{compiler} unavailable")
    caller = tmp_path / "caller.c"
    caller.write_text(CALLER)
    binary = tmp_path / "tower-postfire"
    sources = [SCENE / f"{address}.c" for address in UNITS]
    sources += [ROOT / "src" / "main" / name for name in ("80012b04.c", "80012b58.c")]
    subprocess.run(
        [
            cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-fstrict-aliasing",
            "-fsanitize=undefined", "-fno-sanitize-recover=all",
            "-I", ROOT / "include", "-I", SCENE, *sources, caller, "-o", binary,
        ],
        check=True,
    )
    subprocess.run([binary], check=True)
