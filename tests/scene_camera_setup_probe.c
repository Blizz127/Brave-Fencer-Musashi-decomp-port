#include "camera.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

_Static_assert(sizeof(FacingActor) == 0xa4, "camera record size");
_Static_assert(offsetof(FacingActor, mode) == 4, "dispatch byte");
_Static_assert(offsetof(FacingActor, phase) == 5, "phase byte");
_Static_assert(offsetof(FacingActor, target_offset) == 0x2e, "target offset");
_Static_assert(offsetof(FacingActor, facing) == 0xa0, "facing halfword");

FacingActor D_80126948;
s16 D_801A0C88;
u16 D_80126B5E, D_80126B62, D_80126B66, D_80126940;
s16 D_80126942, D_80126944;
extern s32 camera_probe_register(u32 token, s32 mode);
extern s32 camera_probe_sync(u8 *record);
static unsigned step, isolated_calls;
static s16 callback_facing;
static FacingActor *isolated;
static u8 initial_pattern;

static u32 guest_address(const u8 bytes[4])
{
    return (u32)bytes[0] | (u32)bytes[1] << 8 | (u32)bytes[2] << 16 | (u32)bytes[3] << 24;
}

static void set_vector(SpatialPoint *p, s16 x, s16 y, s16 z)
{
    p->x = x; p->y = y; p->z = z;
}

static void check_vector(const SpatialPoint *p, s16 x, s16 y, s16 z)
{
    assert(p->x == x && p->y == y && p->z == z);
}

s32 func_8012A018(u32 token, s32 mode)
{
    assert(step++ == 0 && token == 0x8017d0d4u && mode == 0);
    assert(D_80126948.distance == 400 && D_80126948.target_distance == 400);
    assert(D_80126948.phase_value == 1000);
    check_vector(&D_80126948.target_rotation.position, 227, 0, 0);
    check_vector(&D_80126948.target_offset, 0, -80, 0);
    assert(D_80126948.mode == initial_pattern && D_80126948.phase == initial_pattern);
    return camera_probe_register(token, mode);
}

s32 func_8012A094(u8 *record)
{
    assert(record == (u8 *)&D_80126948);
    assert(step == 1 || step == 3);
    ++step;
    return camera_probe_sync(record);
}

static void scene_handler(FacingActor *actor)
{
    assert(step++ == 2 && actor == &D_80126948 && actor->mode == 0);
    assert(guest_address(actor->callback_address) == 0x8017d0d4u && actor->facing == 0);
    assert(actor->depth == 1000 && actor->phase == initial_pattern);
    check_vector(&actor->rotation.position, 227, 0, 0);
    check_vector(&actor->offset, 0, -80, 0);
    actor->target_distance = -70001;
    actor->phase_value = 81002;
    set_vector(&actor->target_rotation.position, -311, 0, 32767);
    set_vector(&actor->target_offset, -32768, 234, -7);
    actor->facing = callback_facing;
}

static void synthetic_slot(FacingActor *actor)
{
    assert(actor == isolated && actor->mode == 255);
    ++isolated_calls;
    actor->phase = 17;
}

/* Only slot zero belongs to the measured scene. The high synthetic index
 * exercises the unsigned-byte ABI against an independently populated table. */
SceneCameraHandler D_80182B78[256] = { [0] = scene_handler, [255] = synthetic_slot };

int main(void)
{
    const s16 cases[] = { -32768, -1, 0, 1, 32767 };
    FacingActor synthetic = {0};
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        initial_pattern = (u8)(0xa5u + i);
        memset(&D_80126948, initial_pattern, sizeof D_80126948);
        D_80126B5E = (u16)(-900 + (s16)i);
        D_80126B62 = 33000; D_80126B66 = 22000;
        D_801A0C88 = 740;
        callback_facing = cases[i]; step = 0;
        assert(func_8017D01C() == (s32)(u16)cases[i]);
        assert(step == 4 && D_801A0C88 == cases[i]);
        assert(D_80126948.distance == -70001 && D_80126948.depth == 81002);
        check_vector(&D_80126948.rotation.position, -311, 0, 32767);
        check_vector(&D_80126948.offset, -32768, 234, -7);
        assert(D_80126940 == D_80126B5E && (u16)D_80126942 == D_80126B62 &&
               (u16)D_80126944 == D_80126B66);
    }
    isolated = &synthetic;
    synthetic.mode = 255; func_8017D0D4(&synthetic);
    assert(isolated_calls == 1 && synthetic.phase == 17);
}
