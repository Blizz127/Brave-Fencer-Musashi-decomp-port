#include "player_direction.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

extern void func_8017DE84(u8 *player);
extern void func_8017DEFC(u8 *player);
extern void func_8017DF5C(u8 *player);
extern void func_8017DFD8(u8 *player);
extern s32 func_80029178(u32 bit);
extern void func_80029124(u32 bit, u8 set);

static void unexpected_state(u8 *unused)
{
    (void)unused;
    assert(0 && "outside the three event states under test");
}

void (*D_8018A740[])(u8 *) = {
    unexpected_state, func_8017DEFC, func_8017DF5C, func_8017DFD8, unexpected_state
};

u8 D_800AE648[0x40], D_80126948[0xA4];
s32 D_80126954, D_8012695C;
s16 D_80126968, D_8012696A, D_8012696C, D_80126976, D_80126978, D_8012697A;
u16 D_80126980, D_80126940, D_80126B5E, D_80126B62, D_80126B66;
s16 D_80126942, D_80126944;
u32 D_801151D4;

_Static_assert(offsetof(ScenePlayer, animation_status) == 0xB8, "animation status offset");
_Static_assert(offsetof(ScenePlayer, effect_address) == 0x198, "effect address offset");
_Static_assert(offsetof(ScenePlayer, event_progress.counter) == 0x20C, "counter offset");
_Static_assert(offsetof(ScenePlayer, event_progress.phase) == 0x20C, "event phase offset");
_Static_assert(offsetof(ScenePlayer, event_state) == 0x214, "event state offset");

static ScenePlayer player;
static SceneDirectionCamera camera;
static SceneDirectionLink heading;
static u8 effect;
static u32 records[4];
static unsigned angles, cleanups, allocations, record_count, flag_reads, releases;
static int skip, finished;

#define CAMERA_TOKEN 0x81300100u
#define HEADING_TOKEN 0x81300200u
#define EFFECT_TOKEN 0x81300300u
#define SOURCE_TOKEN 0x81300400u

static u32 word(const u8 *p)
{
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}

u8 *bfm_opening_guest_ptr(u32 token)
{
    switch (token) {
    case CAMERA_TOKEN: return (u8 *)&camera;
    case HEADING_TOKEN: return (u8 *)&heading;
    case EFFECT_TOKEN: return &effect;
    default: assert(0 && "unknown fixture guest address"); return 0;
    }
}

/* The timer runs the scene's direction code with deterministic angle and cleanup providers. */
s16 func_8004CFEC(s32 dx, s32 dz)
{
    assert(dx == 0 && dz == 7);
    ++angles;
    return 0;
}

s32 func_80146D90(void *object)
{
    assert(object == &player && player.event_progress.counter == 31);
    ++cleanups;
    return 0;
}

/* External event providers expose the branch, record and release order at the scene boundary. */
s32 func_800CF854(void) { return skip; }

u32 func_8013767C(u32 source)
{
    assert(source == SOURCE_TOKEN);
    ++allocations;
    return EFFECT_TOKEN;
}

void func_80154274(u8 *object, u32 record)
{
    assert(object == (u8 *)&player && record_count < sizeof(records) / sizeof(records[0]));
    records[record_count++] = record;
}

s32 func_8014706C(u8 *object)
{
    assert(object == (u8 *)&player && record_count == 1);
    return object[0x4D] = 2;
}

s32 func_8016F410(s32 index)
{
    u32 bit = 0x125u + (u32)index;
    assert(index == (s32)(flag_reads % 2));
    ++flag_reads;
    s32 active = func_80029178(bit);
    if (active)
        func_80029124(bit, 0);
    return active;
}

s32 func_801399F0(u8 *object) { assert(object == &effect); return finished; }
s32 func_80139914(u8 *object)
{
    assert(object == &effect && finished && player.event_state == 3);
    assert(((u8 *)&player)[0x4D] == 2);
    ++releases;
    return 0;
}
static void dispatch(void) { func_8017DE84((u8 *)&player); }

static void check_route(int bypass, int simultaneous_flags)
{
    u8 *bytes = (u8 *)&player;
    memset(&player, 0, sizeof(player));
    memset(D_80126948, 0xA5, sizeof(D_80126948));
    angles = cleanups = allocations = record_count = flag_reads = releases = 0;
    skip = bypass;
    finished = 0;
    player.event_state = 1;
    player.event_progress.counter = 99;
    bytes[0x20D] = 0x6A;
    player.effect_address = SOURCE_TOKEN;
    player.link = HEADING_TOKEN;
    D_801151D4 = CAMERA_TOKEN;
    camera.target_z = 7;
    D_80126980 = 0x35;
    D_80126B5E = 15; D_80126B62 = 37; D_80126B66 = 42;
    dispatch();
    assert(player.event_state == 2 && player.event_progress.counter == 0 && bytes[0x20D] == 0x6A);
    assert(D_80126980 == 0x8035 && D_80126954 == 350 && D_8012695C == 600);
    assert(D_80126968 == 450 && D_8012696A == 1536 && D_8012696C == 0);
    assert(D_80126976 == 320 && D_80126978 == 0 && D_8012697A == 320);
    assert(word(D_80126948) == 0x8017DA08u && D_80126948[4] == 0);
    assert(D_80126940 == 15 && D_80126942 == 37 && D_80126944 == 42);

    for (unsigned i = 1; i <= 30; ++i) {
        dispatch();
        assert(player.event_state == 2 && player.event_progress.counter == i && cleanups == 0);
    }
    dispatch();
    assert(angles == 31 && cleanups == 1 && player.direction_bits == 0x4000);
    if (bypass) {
        assert(player.event_state == 4 && allocations == 0 && record_count == 0 && bytes[0x4D] == 0);
        assert(player.effect_address == SOURCE_TOKEN && bytes[0x20D] == 0x6A);
        return;
    }
    assert(player.event_state == 3 && player.effect_address == EFFECT_TOKEN && allocations == 1);
    assert(records[0] == 0x800D5A34u && bytes[0x4D] == 2 && player.event_progress.phase == 0);

    dispatch();
    assert(record_count == 1 && releases == 0);
    func_80029124(0x125, 1);
    if (!simultaneous_flags) {
        dispatch();
        assert(record_count == 2 && records[1] == 0x8018A62Cu);
        assert(!func_80029178(0x125) && player.event_progress.phase == 0);
    }
    func_80029124(0x126, 1);
    dispatch();
    assert(record_count == 3 && records[1] == 0x8018A62Cu && records[2] == 0x8018A69Cu);
    assert(!func_80029178(0x125) && !func_80029178(0x126) && player.event_progress.phase == 1);
    player.animation_status = 0x8000;
    dispatch();
    assert(record_count == 4 && records[3] == 0x8018A6E4u && player.event_progress.phase == 2);
    assert(player.event_state == 3 && bytes[0x4D] == 2 && !releases);
    finished = -1;
    dispatch();
    assert(player.event_state == 4 && releases == 1 && bytes[0x4D] == 1);
    assert(flag_reads == (simultaneous_flags ? 8u : 10u));
}

int main(void)
{
    check_route(0, 0);
    check_route(0, 1);
    check_route(1, 0);
    return 0;
}
