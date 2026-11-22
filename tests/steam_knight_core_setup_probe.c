#include "steam_knight_hit.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern void func_80183FCC(SteamKnightPart *core);
extern void func_801840E8(SteamKnightPart *core);

typedef struct { u8 bytes[4]; } CoreModelWord;
CoreModelWord D_801E44A0, D_801E44A4, D_801E2E70, D_801E2E74;
s16 D_801D323A;

enum {
    ORIGINAL_MODEL = 0x1040, NEXT_MODEL = 0x1840, OLD_PARENT = 0x5040,
    NEXT_PARENT = 0x6040, SOUND_PARENT = 0x7040, RETURNED = 0x8040,
    SAVED_LINK = 0x9040
};
static const u32 alias_bit = 0xa0000000u;
static const u32 model_list = 0x801f12e8u, static_shape = 0x801d3230u;
enum Binding { BIND, SOUND, ATTACH, PREPARE, ALLOCATE, CONFIGURE };
enum Target { SEPARATE, FLAGS, PARENT, LINK };
static SteamKnightPart core, first_parent, ready_parent, last_parent;
static u8 original[80], next[80], allocated[80];
static enum Binding order[6];
static unsigned step, end, direct, change_parent;
static enum Target target;
static u32 model_in, model_next, allocated_id, prepared_x, prepared_y;
static u32 attached_to;
static u16 expected_gate;

static u32 word(const void *at)
{
    u32 value;
    memcpy(&value, at, sizeof value);
    return value;
}
static void put_word(void *at, u32 value) { memcpy(at, &value, sizeof value); }
static u16 half(const void *at)
{
    u16 value;
    memcpy(&value, at, sizeof value);
    return value;
}
static void observe(enum Binding which)
{
    assert(step < end && order[step] == which);
    step++;
}
static u8 *returned_data(void)
{
    if (target == LINK)
        return (u8 *)&core + 0xc8;
    return allocated;
}

SteamKnightPart *bfm_steam_knight_break_actor(u32 address)
{
    if (address == OLD_PARENT) return &first_parent;
    if (address == (alias_bit | NEXT_PARENT) || address == static_shape) return &ready_parent;
    if (address == SOUND_PARENT) return &last_parent;
    abort();
}
u8 *bfm_steam_knight_core_model(u32 address)
{
    if (address == model_next) {
        if (target == FLAGS) return (u8 *)&core + 0xdc;
        if (target == PARENT) return (u8 *)&core + 0x44;
        return next;
    }
    if (address == model_in && address != 0) return original;
    if (address == allocated_id && address != 0) return returned_data();
    abort();
}
void bfm_tower_actor84_model_list(u32 model, u32 list)
{
    observe(BIND);
    assert(model == model_in && list == model_list);
    core.model_address = model_next;
    if (change_parent)
        core.body_address = alias_bit | NEXT_PARENT;
}
void func_8002D4C8(u16 command, u16 argument)
{
    observe(SOUND);
    assert(command == 1290 && argument == 0);
    assert(core.field_fc == (u16)(expected_gate | 1));
    assert(core.field_0e == 24);
    if (!direct)
        core.body_address = SOUND_PARENT;
}
void bfm_steam_knight_core_attach(SteamKnightPart *actor, u32 parent, int mode)
{
    observe(ATTACH);
    assert(actor == &core && parent == attached_to && mode == 21);
    assert(actor->field_0e == 24 && D_801D323A == 0);
    assert(word(original + 0x20) == 0x39393939u);
    assert(word(bfm_steam_knight_core_model(model_next) + 0x20) ==
           (target == PARENT ? attached_to : static_shape));
    put_word(&D_801E2E70, 0xe0017101);
    put_word(&D_801E2E74, 0xe0027202);
}
void func_800233CC(void *shape, int length)
{
    observe(PREPARE);
    assert(shape == &D_801E44A0 && length == 40);
    assert(word(&D_801E2E70) == 0xe0017101 && word(&D_801E2E74) == 0xe0027202);
    put_word(&D_801E44A0, 0xdbdbdbdb);
    put_word(&D_801E44A4, 0xedededed);
    put_word(&D_801E2E70, prepared_x);
    put_word(&D_801E2E74, prepared_y);
}
u32 bfm_steam_knight_core_new_model(int kind, int amount)
{
    observe(ALLOCATE);
    assert(kind == 126 && amount == 256);
    assert(word(&D_801E44A0) == prepared_x && word(&D_801E44A4) == prepared_y);
    assert(core.linked_address == SAVED_LINK);
    core.linked_address = 0x12345678u;
    return allocated_id;
}
void bfm_steam_knight_core_ring_setup(u32 model, u32 shape)
{
    observe(CONFIGURE);
    assert(model == allocated_id && model == core.linked_address);
    assert(shape == 0x801e44a0u);
    core.linked_address = SAVED_LINK;
    put_word(returned_data() + 4, 0x071234abu);
    memset(returned_data() + 0x2c, 0x9a, 2);
}

static void check_setup(enum Target layout, unsigned tags, unsigned available,
                        unsigned gate, unsigned open, unsigned retarget)
{
    target = layout;
    direct = 0;
    step = end = 0;
    change_parent = retarget;
    model_in = tags ? 0 : ORIGINAL_MODEL;
    model_next = tags ? (alias_bit | NEXT_MODEL) : NEXT_MODEL;
    allocated_id = available ? (tags ? alias_bit | RETURNED : RETURNED) : 0;
    expected_gate = (u16)(0x85a0u | gate);
    prepared_x = 0x8065f001u + tags + target;
    prepared_y = 0x7134c002u + gate + open;
    memset(&core, 0x75, sizeof core);
    memset(original, 0x39, sizeof original);
    memset(next, 0x28, sizeof next);
    memset(allocated, 0x17, sizeof allocated);
    D_801D323A = -255;
    core.model_address = model_in;
    core.body_address = OLD_PARENT;
    core.field_0e = -24;
    core.linked_address = SAVED_LINK;
    core.field_fc = expected_gate;
    first_parent.phase_flags = open ? 0x410002 : 0x410000;
    ready_parent.phase_flags = open ? 0x20000 : 0x20002;
    attached_to = retarget ? alias_bit | NEXT_PARENT : OLD_PARENT;
    if (target == PARENT)
        attached_to = static_shape;
    if (target == FLAGS)
        expected_gate = (u16)static_shape;
    order[end++] = BIND;
    if ((expected_gate & 1) && (bfm_steam_knight_break_actor(attached_to)->phase_flags & 2)) {
        order[end++] = SOUND;
        attached_to = SOUND_PARENT;
    }
    order[end++] = ATTACH;
    order[end++] = PREPARE;
    order[end++] = ALLOCATE;
    if (available) order[end++] = CONFIGURE;
    func_80183FCC(&core);
    assert(step == end && core.field_fc == expected_gate);
    if (available) {
        assert(word(returned_data() + 4) == 0xd71234abu);
        assert(half(returned_data() + 0x2c) == 0xc010u);
        assert(core.linked_address == (target == LINK ? 0xd71234abu : SAVED_LINK));
    } else {
        assert(core.linked_address == 0);
        assert(word(allocated + 4) == 0x17171717u && half(allocated + 0x2c) == 0x1717u);
    }
}

static void check_direct(void)
{
    static const s16 phases[] = {-32768, -25, -24, -1, 0, 1, 23, 24, 25, 32767};
    direct = 1;
    for (unsigned i = 0; i < sizeof phases / sizeof phases[0]; i++)
        for (unsigned open = 0; open < 2; open++)
            for (unsigned bit = 0; bit < 2; bit++) {
                core.field_0e = phases[i];
                expected_gate = (u16)(0x40f0u | bit);
                core.field_fc = expected_gate;
                core.body_address = OLD_PARENT;
                first_parent.phase_flags = open ? 0x820002 : 0x820000;
                step = end = 0;
                if (open && phases[i] == 24) order[end++] = SOUND;
                func_801840E8(&core);
                assert(step == end);
                assert(core.field_fc == (u16)(expected_gate | (!open || phases[i] == 24)));
            }
}
int main(void)
{
    assert(sizeof(void *) >= sizeof(u32));
    check_direct();
    for (unsigned layout = 0; layout < 4; layout++)
        for (unsigned tagged = 0; tagged < 2; tagged++)
            for (unsigned available = 0; available < 2; available++)
                for (unsigned gate = 0; gate < 2; gate++)
                    for (unsigned open = 0; open < 2; open++)
                        for (unsigned replace = 0; replace < 2; replace++)
                            check_setup((enum Target)layout, tagged, available, gate, open, replace);
    return 0;
}
