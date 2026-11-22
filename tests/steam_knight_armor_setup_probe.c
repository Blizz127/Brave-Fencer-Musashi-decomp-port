#include "steam_knight_armor_setup.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum { NORMAL, TEMPLATE_FAIL, MODEL_FAIL, OWNER_OVERLAP };
static SteamKnightPart part, expected;
static u16 low[13], high[6];
static u8 records[19][52], model_data;
static u8 *model = &model_data;
static const u32 model_key = 0x11000, parent_key = 0x23000;
static u32 parent_link;
static int variant, mode;
static u16 joint;
static const char *next_call;

static void check(char event)
{
    assert(*next_call == event);
    ++next_call;
    assert(memcmp(&part, &expected, sizeof part) == 0);
}

static void word(SteamKnightPart *p, size_t offset, u32 value)
{
    memcpy((u8 *)p + offset, &value, sizeof value);
}

void *bfm_tower_spawn_pointer(u32 key)
{
    if (key == 0x80195D74u) return records;
    if (key == 0x80195B4Cu) { assert(variant < 13); return low; }
    if (key == 0x80195B68u) { assert(variant >= 13); return high; }
    assert(key == parent_key + offsetof(SteamKnightPart, linked_address));
    return mode == OWNER_OVERLAP ? (u8 *)&part + offsetof(SteamKnightPart, field_fc)
                                 : (void *)&parent_link;
}

u32 bfm_tower_spawn_address(const void *pointer)
{
    assert(pointer == model);
    return model_key;
}

int func_8012C354(u8 *actor, u8 *record)
{
    assert(actor == (u8 *)&part && variant >= 0 && variant < 19);
    assert(record == records[variant]);
    check('T');
    part.variant = expected.variant = -1;
    word(&part, 196, 0x61504u);
    word(&expected, 196, 0x61504u);
    return mode != TEMPLATE_FAIL;
}

void *func_8012C1B8(void)
{
    memset((u8 *)&expected + 4, 0, 12);
    ((u8 *)&expected)[117] = 8;
    word(&expected, 180, 0);
    word(&expected, 188, 0x80196490u);
    ((u8 *)&expected)[192] = 1;
    expected.reaction = 0;
    word(&expected, 196, 0x61506u);
    expected.field_dc = 0;
    expected.break_delay = 0;
    check('M');
    part.model_address = expected.model_address = 0x6010;
    return mode == MODEL_FAIL ? NULL : model;
}

void bfm_tower_armor_dispose(u8 *actor)
{
    assert(actor == (u8 *)&part);
    if (mode == MODEL_FAIL) expected.model_address = 0;
    check('D');
    part.active = expected.active = 0;
}

void bfm_tower_armor_bind_model(void *pointer)
{
    assert(pointer == model);
    expected.model_address = model_key;
    check('B');
    if (variant < 13) low[variant] = joint;
    else high[variant - 13] = joint;
    part.body_address = expected.body_address = parent_key;
    part.break_count = expected.break_count = 0x76ae;
    parent_link = 0x43000u;
}

s32 bfm_tower_armor_advance(u8 *actor)
{
    assert(actor == (u8 *)&part);
    expected.linked_address = parent_key;
    expected.field_fc = joint;
    if (joint >= 22) {
        expected.field_fc -= 22;
        if (mode == OWNER_OVERLAP) {
            /* The related-owner word overlaps the new joint and old break count. */
            memcpy(&expected.body_address,
                   (u8 *)&expected + offsetof(SteamKnightPart, field_fc), sizeof(u32));
        } else {
            expected.body_address = parent_link;
        }
        expected.break_count = 1;
    } else {
        expected.break_count = joint == 4 || joint == 9 || joint == 14 || joint == 19 ? 2 : 0;
    }
    check('A');
    part.state = expected.state = 7;
    return -359;
}

static void run(int selected, u16 code, int outcome, const char *events)
{
    variant = selected;
    joint = code;
    mode = outcome;
    next_call = events;
    memset(low, 0x57, sizeof low);
    memset(high, 0x68, sizeof high);
    memset(&part, 0x35, sizeof part);
    part.variant = (s16)selected;
    memcpy(&expected, &part, sizeof part);
    func_80184EC0(&part);
    assert(*next_call == '\0');
    assert(memcmp(&part, &expected, sizeof part) == 0);
}

int main(void)
{
    static const u16 codes[] = {0, 3, 4, 9, 14, 19, 21, 22, 57, 65535};
    for (int i = 0; i < 19; ++i)
        run(i, codes[(unsigned)i % (sizeof codes / sizeof *codes)], NORMAL, "TMBA");
    run(13, 48, OWNER_OVERLAP, "TMBA");
    run(12, 0, TEMPLATE_FAIL, "T");
    run(18, 0, MODEL_FAIL, "TMD");
#if UINTPTR_MAX > UINT32_MAX
    u8 stack_model;
    if ((uintptr_t)&stack_model > UINT32_MAX) {
        model = &stack_model;
        run(15, 45, NORMAL, "TMBA");
        model = &model_data;
    }
#endif
    run(-1, 0, NORMAL, "D");
    run(19, 0, NORMAL, "D");
    return 0;
}
