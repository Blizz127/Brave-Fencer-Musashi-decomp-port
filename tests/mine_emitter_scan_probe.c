#include "mine_emitter_scan.h"
#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum { SLOT_COUNT = 96, SLOT_BYTES = 268, EMITTER = 424,
       TICK = 0x84, DEPTH = 0xe0, HORIZONTAL = 0x100, PLAYING = 0x102 };
Obj8014A2E4 D_801202A0[SLOT_COUNT];
static Obj8014A2E4 expected_pool[SLOT_COUNT];
typedef struct {
    MineEmitterActor actor;
    u8 rest[SLOT_BYTES - sizeof(MineEmitterActor)];
} ExternalOwner;
static ExternalOwner external, expected_external;
_Static_assert(sizeof(ExternalOwner) == SLOT_BYTES && sizeof(Obj8014A2E4) == SLOT_BYTES, "full actor slots");
_Static_assert(offsetof(MineEmitterActor, range) == HORIZONTAL && offsetof(MineEmitterActor, distance) == DEPTH, "source actor fields");

static unsigned projection_order[SLOT_COUNT], projections, count, sounds;
static u16 wanted_command, wanted_argument;
static int owner_index, edits, hold_at, caller_enabled, spawn_enabled, timer_calls, spawn_calls;
static jmp_buf boundary;

static u16 read16(const void *base, size_t offset)
{
    const u8 *p = (const u8 *)base + offset;
    return (u16)(p[0] + 256u * p[1]);
}

static void write16(void *base, size_t offset, u16 value)
{
    u8 *p = (u8 *)base + offset;
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}

static void write32(void *base, size_t offset, u32 value)
{
    write16(base, offset, (u16)value);
    write16(base, offset + 2, (u16)(value >> 16));
}

static void *owner(int expected)
{
    if (owner_index < 0) return expected ? &expected_external : &external;
    return expected ? &expected_pool[owner_index] : &D_801202A0[owner_index];
}

static void compare_state(void)
{
    assert(memcmp(D_801202A0, expected_pool, sizeof expected_pool) == 0);
    assert(memcmp(&external, &expected_external, sizeof external) == 0);
}

static void record_external_changes(void)
{
    memcpy(expected_pool, D_801202A0, sizeof expected_pool);
    memcpy(&expected_external, &external, sizeof external);
}

static void increment_tick(void)
{
    void *p = owner(1);
    write16(p, TICK, (u16)(read16(p, TICK) + 1));
}

void func_8017E910(Obj8014A2E4 *slot)
{
    assert(projections < count && slot == &D_801202A0[projection_order[projections]]);
    compare_state();
    unsigned which = projection_order[projections++];
    if ((int)which == hold_at) longjmp(boundary, 1);
    if (edits && which == 3) {
        /* Refresh current coordinates, enable a future match, and disable another. */
        write16(slot, HORIZONTAL, (u16)-23);
        D_801202A0[42].active = EMITTER;
        D_801202A0[51].active = 0x1a9;
        D_801202A0[1].active = EMITTER;
        write16(owner(0), TICK, 2);
    }
    if (edits && which == 42) {
        /* The chosen horizontal value is already latched, but depth is read after the last projection. */
        write16(&D_801202A0[3], HORIZONTAL, 350);
        write32(&D_801202A0[3], DEPTH, edits == 1 ? 311 : 337);
        write16(owner(0), TICK, 3);
        write16(owner(0), PLAYING, 1);
        write16(slot, HORIZONTAL, 60);
        D_801202A0[95].active = EMITTER;
    }
    if (edits && which == 95) {
        write16(slot, HORIZONTAL, (u16)-99);
        ((u8 *)owner(0))[0xaa] ^= 0x77;
    }
    record_external_changes();
}

void bfm_mine_emitter_audio(u16 command, u16 argument)
{
    assert(!sounds++ && projections == count);
    assert(wanted_command && command == wanted_command && argument == wanted_argument);
    increment_tick();
    compare_state();
    /* The caller must retain callback effects except for its subsequent playback flag write. */
    write16(owner(0), TICK, (u16)(read16(owner(0), TICK) ^ 0x572c));
    write16(owner(0), PLAYING, 0xa451);
    ((u8 *)owner(0))[0x33] ^= 0x63;
    record_external_changes();
}

static void check_scan_end(void)
{
    assert(projections == count && sounds == (unsigned)(wanted_command != 0));
    if (sounds) write16(owner(1), PLAYING, wanted_command == 0x6da ? 1 : 0);
    else increment_tick();
    compare_state();
}

#ifdef BFM_TEST_EMITTER_CALLER
MineEmitterStep D_8019B594[] = {{2, 33}, {0xff, 0}};
s32 func_8012BEE8(MineEmitterActor *actor)
{
    assert(actor == &external.actor && caller_enabled && !timer_calls++);
    if (actor->subtype == 0) check_scan_end();
    else compare_state();
    return spawn_enabled;
}
s32 func_8005C49C(void)
{
    assert(0 && "synthetic sequence has a fixed subtype");
    return 0;
}
s32 func_8012C658(s16 type, s16 subtype, void *parent)
{
    assert(timer_calls == 1 && spawn_enabled && !spawn_calls++);
    assert(type == 425 && subtype == 2 && parent == &external.actor);
    expected_external.actor.timer = 33;
    compare_state();
    external.rest[0] ^= 0x81;
    record_external_changes();
    return INT32_MIN;
}
#endif

static void reset(int pool_owner, u16 tick, u16 flag)
{
    u8 *bytes = (u8 *)D_801202A0;
    for (size_t i = 0; i < sizeof D_801202A0; ++i) bytes[i] = (u8)(13u * i + 73u);
    memset(&external, 0x8f, sizeof external);
    for (unsigned i = 0; i < SLOT_COUNT; ++i) D_801202A0[i].active = (u16)(i & 1 ? 0x1a9 : 0);
    projections = count = sounds = 0;
    wanted_command = wanted_argument = 0;
    owner_index = pool_owner;
    hold_at = -1;
    edits = caller_enabled = spawn_enabled = timer_calls = spawn_calls = 0;
    write16(owner(0), TICK, tick);
    write16(owner(0), PLAYING, flag);
}

static void setup_slot(unsigned i, int x, s32 z)
{
    assert(i < SLOT_COUNT);
    D_801202A0[i].active = EMITTER;
    write16(&D_801202A0[i], HORIZONTAL, (u16)x);
    write32(&D_801202A0[i], DEPTH, (u32)z);
}

static void project(unsigned i)
{
    assert(count < SLOT_COUNT);
    projection_order[count++] = i;
}

static void run(void)
{
    record_external_changes();
    if (hold_at >= 0 && setjmp(boundary) != 0) {
        assert(projections == 1 && !sounds);
        compare_state();
        return;
    }
#ifdef BFM_TEST_EMITTER_CALLER
    if (caller_enabled) {
        func_8017E640(&external.actor);
        assert(timer_calls == 1 && spawn_calls == spawn_enabled);
        if (spawn_calls) expected_external.actor.index++;
        compare_state();
        return;
    }
#endif
    func_8017E794((MineEmitterActor *)owner(0));
    assert(hold_at < 0);
    check_scan_end();
}

static void singleton(int x, s32 z, u16 tick, u16 flag, u16 command, u16 argument)
{
    reset(95, tick, flag);
    setup_slot(95, x, z);
    project(95);
    wanted_command = command;
    wanted_argument = argument;
    run();
}

static void choose_by_first_horizontal(int late_depth)
{
    reset(42, 0, 0);
    edits = late_depth ? 2 : 1;
    setup_slot(3, 600, 400);
    setup_slot(51, 0, 0);
    project(3); project(42); project(95);
    wanted_command = late_depth ? 4 : 0x6da;
    wanted_argument = late_depth ? 0x6da : 0x3674;
    run();
}

int main(void)
{
    reset(-1, 65535, 1);
    wanted_command = 4; wanted_argument = 0x6da;
    run();
    reset(-1, 3, 0xffff);
    run();
    singleton(0, INT32_MIN, 3, 2, 0x6da, 0x387f);
    singleton(159, 335, 65535, 1, 0x6da, 0x3f30);
    singleton(-159, 335, 3, 1, 0x6da, 0x3030);
    singleton(160, -8, 3, 1, 4, 0x6da);
    singleton(-160, 20, 1, 2, 0, 0);
    singleton(-32768, -1, 3, 0x8000, 0x6da, 0xf27f);
    singleton(32767, 335, 3, 1, 4, 0x6da);
    singleton(-1, 336, 3, 1, 4, 0x6da);
    singleton(-1, INT32_MAX, 3, 0, 0, 0);
    for (u16 tick = 0; tick < 3; ++tick) singleton(6, 10, tick, 1, 0, 0);
    choose_by_first_horizontal(0);
    choose_by_first_horizontal(1);
    for (unsigned first_deep = 0; first_deep < 2; ++first_deep) {
        reset(0, 7, 1);
        setup_slot(0, -20, first_deep ? 700 : 50);
        setup_slot(95, 20, 0);
        project(0); project(95);
        wanted_command = first_deep ? 4 : 0x6da;
        wanted_argument = first_deep ? 0x6da : 0x3775;
        run();
    }
    reset(17, 3, 0);
    for (unsigned i = 0; i < SLOT_COUNT; ++i) {
        setup_slot(i, (int)(191 - 2 * i), 50);
        project(i);
    }
    wanted_command = 0x6da; wanted_argument = 0x387f;
    run();
    reset(0, 0, 1);
    hold_at = 0;
    setup_slot(0, 4, 10); project(0);
    run();
#ifdef BFM_TEST_EMITTER_CALLER
    for (int subtype = 0; subtype <= 1; ++subtype) {
        reset(-1, 3, 0);
        external.actor.subtype = (s16)subtype;
        external.actor.index = 0;
        external.actor.range = -23;
        external.actor.distance = -1;
        setup_slot(95, 0, 1);
        if (!subtype) {
            project(95);
            wanted_command = 0x6da;
            wanted_argument = 0x387f;
        }
        caller_enabled = spawn_enabled = 1;
        run();
    }
#endif
    reset(-1, 3, 0);
    record_external_changes();
    func_8017E794(NULL);
    assert(!projections && !sounds);
    compare_state();
    return 0;
}
