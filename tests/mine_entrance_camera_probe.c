#include "mine_entrance_camera.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(FacingActor) == 164, "scene camera record");
_Static_assert(offsetof(FacingActor, target_rotation) == 0x20, "target rotation");
_Static_assert(offsetof(FacingActor, target_offset) == 0x2e, "target offset");

FacingActor D_80126948;
u16 D_80126B5E, D_80126B62, D_80126B66, D_80126940;
s16 D_80126942, D_80126944;
static FacingActor *expected;
static u32 token;
static unsigned calls;
static int depth, pitch, write_frames;

static void snapshot(void)
{
    if (write_frames) {
        assert(fwrite(expected, sizeof *expected, 1, stdout) == 1);
        u16 axes[] = {D_80126940, (u16)D_80126942, (u16)D_80126944};
        assert(fwrite(axes, sizeof axes, 1, stdout) == 1);
    }
}

static void room_slot(FacingActor *camera)
{
    u32 registered;
    assert(camera == expected && camera->mode == 0);
    memcpy(&registered, camera->callback_address, sizeof registered);
    assert(registered == token);
    assert(camera->depth == depth && camera->phase_value == depth);
    assert(camera->distance == 400 && camera->target_distance == 400);
    assert(camera->rotation.position.x == pitch && camera->rotation.position.y == 2048);
    assert(camera->rotation.position.z == 0 && camera->offset.y == -32);
    assert(camera->offset.x == 0 && camera->offset.z == 0);
    snapshot();
    camera->phase = 93;
    camera->distance = -123;
    ++calls;
}

static void alternate(FacingActor *camera)
{
    assert(camera == expected && camera->mode != 0);
    camera->phase = camera->mode;
    ++calls;
}

static void room_3045_slot(FacingActor *camera)
{
    assert(token == 0x8017D384u);
    room_slot(camera);
}

static void room_3049_slot(FacingActor *camera)
{
    assert(token == 0x8017F120u);
    room_slot(camera);
}

/* The original entrance selects zero. Other slots are separately populated
 * test handlers for the dispatcher's unsigned mode field. */
SceneCameraHandler D_80181E50[256] = {
    [0] = room_3045_slot, [1] = alternate, [127] = alternate, [128] = alternate, [255] = alternate
};
SceneCameraHandler D_80186F9C[256] = {
    [0] = room_3049_slot, [1] = alternate, [127] = alternate, [128] = alternate, [255] = alternate
};

static void check_room(unsigned index, unsigned seed)
{
    static FacingActor separate;
    static const u8 modes[] = {1, 127, 128, 255};
    unsigned char *bytes = (unsigned char *)&D_80126948;
    for (size_t i = 0; i < sizeof D_80126948; ++i)
        bytes[i] = (u8)(i * (seed * 2 + 1) + seed * 35);
    D_80126B5E = (u16)(0xff91 + seed);
    D_80126B62 = (u16)(0x8002 - seed);
    D_80126B66 = (u16)(0x71e3 + seed);
    D_80126940 = 3; D_80126942 = -5; D_80126944 = 7;
    expected = &D_80126948;
    calls = 0;
    depth = index ? 1000 : 1200;
    pitch = index ? 227 : 341;
    token = index ? 0x8017F120u : 0x8017D384u;
    if (index) func_8017F07C(); else func_8017D2E0();
    assert(calls == 1 && expected->phase == 93 && expected->distance == -123);
    assert(D_80126940 == D_80126B5E && (u16)D_80126942 == D_80126B62 &&
           (u16)D_80126944 == D_80126B66);
    snapshot();
    separate = D_80126948;
    expected = &separate;
    for (size_t i = 0; i < sizeof modes; ++i) {
        separate.mode = modes[i];
        if (index) func_8017F120(expected); else func_8017D384(expected);
        assert(calls == i + 2 && separate.phase == modes[i]);
        snapshot();
    }
}

int main(int argc, char **argv)
{
    write_frames = argc == 2 && strcmp(argv[1], "--snapshots") == 0;
    for (unsigned room = 0; room < 2; ++room)
        for (unsigned seed = 0; seed < 7; ++seed)
            check_room(room, seed);
    return 0;
}
