#include "mine_camera_target.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

u16 D_80126940;
s16 D_80126942, D_80126944, D_80126946, D_80186FA0[2] = {1000, 800};
_Alignas(u16) u8 D_80126B58[0xb0];

static FacingActor camera;
static SceneVector requested;
static int expected_room, expected_x, snapshots, calls;
static u8 expected_phase;
static s32 expected_depth;

static void inspect(FacingActor *record, SceneVector *anchor, int room)
{
    assert(record == &camera && room == expected_room);
    assert(anchor->position.x == expected_x && anchor->position.y == requested.position.y);
    assert(anchor->position.z == requested.position.z && anchor->padding == requested.padding);
    assert(record->phase == expected_phase && record->phase_value == expected_depth);
    if (snapshots) {
        assert(fwrite(record, sizeof *record, 1, stdout) == 1);
        assert(fwrite(anchor, sizeof *anchor, 1, stdout) == 1);
    }
    anchor->position.x = 123;
    anchor->padding = 91;
    record->phase = 0x53;
    ++calls;
}

void func_8017D48C(FacingActor *record, SceneVector *anchor)
{
    inspect(record, anchor, 0);
}

void func_8017F1FC(FacingActor *record, SceneVector *anchor)
{
    inspect(record, anchor, 1);
}

static void check(int room, s16 x, s16 y, u8 phase, u16 flags, unsigned seed)
{
    unsigned char *bytes = (unsigned char *)&camera;
    for (size_t i = 0; i < sizeof camera; ++i)
        bytes[i] = (u8)(i * 13u + seed);
    D_80126940 = (u16)x; D_80126942 = y;
    D_80126944 = (s16)(seed * 31u - 17001u);
    D_80126946 = (s16)(0x7ff0u + seed);
    memcpy(D_80126B58 + 0xac, &flags, sizeof flags);
    expected_room = room;
    requested = (SceneVector){{x, y, D_80126944}, D_80126946};
    camera.phase = phase; camera.phase_value = -90001;
    expected_phase = phase; expected_depth = camera.phase_value;
    expected_x = x; calls = 0;
    if (room == 0) {
        int lo = y < -1664 ? -736 : -576;
        int hi = y < -1664 ? 704 : 576;
        if (expected_x < lo) expected_x = lo;
        if (expected_x > hi) expected_x = hi;
        func_8017D3C0(&camera);
    } else {
        if (flags & 3) {
            expected_phase = (u8)((phase + 1) & 1);
            expected_depth = D_80186FA0[expected_phase];
        }
        func_8017F15C(&camera);
    }
    assert(calls == 1 && camera.phase == 0x53);
    assert(D_80126940 == (u16)x && D_80126942 == y && D_80126944 == requested.position.z &&
           D_80126946 == requested.padding);
    if (snapshots)
        assert(fwrite(&requested, sizeof requested, 1, stdout) == 1);
}

int main(int argc, char **argv)
{
    const s16 xs[] = {-32768,-737,-736,-735,-577,-576,-575,0,575,576,577,703,704,705,32767};
    const s16 ys[] = {-32768,-1665,-1664,-1663,0,32767};
    const u8 phases[] = {0,1,2,127,128,254,255};
    const u16 flags[] = {0,1,2,3,4,0x8000,0xfffc,0xfffe,0xffff};
    unsigned seed = 0;
    snapshots = argc == 2 && !strcmp(argv[1], "--snapshots");
    for (size_t i = 0; i < sizeof xs / sizeof xs[0]; ++i)
        for (size_t j = 0; j < sizeof ys / sizeof ys[0]; ++j)
            check(0, xs[i], ys[j], (u8)++seed, 0xffff, i * 6 + j + 1);
    for (size_t i = 0; i < sizeof phases; ++i)
        for (size_t j = 0; j < sizeof flags / sizeof flags[0]; ++j) {
            ++seed;
            check(1, (s16)(seed * 73), (s16)(seed * 37), phases[i], flags[j], seed);
        }
    return 0;
}
