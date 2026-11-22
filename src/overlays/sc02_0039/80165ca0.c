/* The mine updates all handlers before it copies positions to linked targets. */
#include "mine_actor_links.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef struct {
    u8 pad00[0x14];
    /* 0x14 */ s32 x;
    /* 0x18 */ s32 y;
    /* 0x1C */ s32 z;
} Sub80165CA0;

typedef struct {
    /* 0x00 */ u16 kind;
    u8 pad02[0x08 - 0x02];
    /* 0x08 */ s16 x;
    /* 0x0A */ s16 y;
    /* 0x0C */ s16 z;
    u8 pad0E[0x2C - 0x0E];
    /* 0x2C */ u16 flags2C;
    u8 pad2E[0x34 - 0x2E];
    /* 0x34 */ u32 sub34;
    u8 pad38[0x48 - 0x38];
    /* 0x48 */ s32 x48;
    /* 0x4C */ s32 y4C;
    /* 0x50 */ s32 z50;
} Target80165CA0;

typedef struct {
    /* 0x00 */ u16 kind;
    u8 pad02[0x06 - 0x02];
    /* 0x06 */ s16 x;
    u8 pad08[0x0A - 0x08];
    /* 0x0A */ s16 y;
    u8 pad0C[0x0E - 0x0C];
    /* 0x0E */ s16 z;
    u8 pad10[0x20 - 0x10];
    /* 0x20 */ u32 target;
    u8 pad24[0x58 - 0x24];
} Slot80165CA0;

extern Slot80165CA0 D_8011D030[];
extern u32 D_80126728;

void func_80165CA0(void)
{
    Slot80165CA0 *slot;
    Slot80165CA0 *e;
    Target80165CA0 *t;
    Sub80165CA0 *sub;
    s32 i;

    i = 0;
    slot = D_8011D030;
    for (; i < 30; i++, slot++) {
        if (slot->kind != 0) {
            ((void (*)(Slot80165CA0 *))((u32 *)D_80126728)[slot->kind])(slot);
        }
    }
    for (i = 0; i < 30; i++) {
        e = &D_8011D030[i];
        t = (Target80165CA0 *)e->target;
        if (t == 0) {
            continue;
        }
        if (t->kind == 1) {
            t->x = e->x;
            t->x48 = t->x;
            t->y = e->y;
            t->y4C = t->y;
            t->z = e->z;
            t->z50 = t->z;
            t->flags2C |= 1;
        } else if ((sub = (Sub80165CA0 *)t->sub34) != 0) {
            t->x = e->x;
            sub->x = t->x;
            t->y = e->y;
            ((Sub80165CA0 *)t->sub34)->y = t->y;
            t->z = e->z;
            ((Sub80165CA0 *)t->sub34)->z = t->z;
        } else {
            t->x = e->x;
            t->y = e->y;
            t->z = e->z;
        }
    }
}
#else
static int read_half(u32 address, u16 *value)
{
    return !(address & 1u) && bfm_mine_link_read_half(address, value);
}

static int read_word(u32 address, u32 *value)
{
    return !(address & 3u) && bfm_mine_link_read_word(address, value);
}

static int store_half(u32 address, u16 value)
{
    return !(address & 1u) && bfm_mine_link_store_half(address, value);
}

static int store_word(u32 address, u32 value)
{
    return !(address & 3u) && bfm_mine_link_store_word(address, value);
}

static u32 signed_coordinate(u16 value)
{
    return (u32)(value ^ 0x8000u) - 0x8000u;
}

static int copy_position(u32 slot, u32 target, u16 kind)
{
    u32 sub = 0;
    u16 flags = 0;
    unsigned axis;

    if (kind != 1 && !read_word(target + 0x34u, &sub))
        return 0;
    /* The first subobject determines the branch. Later pointers can change. */
    if (kind != 1 && sub) {
        for (axis = 0; axis < 3; ++axis) {
            u16 value;
            if (!read_half(slot + 6u + 4u * axis, &value))
                return 0;
            if (axis && !read_word(target + 0x34u, &sub))
                return 0;
            if (!store_half(target + 8u + 2u * axis, value) ||
                    !store_word(sub + 0x14u + 4u * axis, signed_coordinate(value)))
                return 0;
        }
        return 1;
    }
    for (axis = 0; axis < 3; ++axis) {
        u16 value;
        if (kind == 1 && axis == 2 && !read_half(target + 0x2cu, &flags))
            return 0;
        if (!read_half(slot + 6u + 4u * axis, &value) ||
                !store_half(target + 8u + 2u * axis, value))
            return 0;
        if (kind == 1) {
            if (axis == 2 && !store_half(target + 0x2cu, (u16)(flags | 1u)))
                return 0;
            if (!store_word(target + 0x48u + 4u * axis, signed_coordinate(value)))
                return 0;
        }
    }
    return 1;
}

int bfm_mine_update_actor_links(void)
{
    unsigned i;
    for (i = 0; i < 30; ++i) {
        u32 slot = BFM_MINE_LINK_SLOTS + i * 0x58u;
        u32 table, handler;
        u16 kind;
        if (!read_half(slot, &kind))
            return 0;
        if (kind && (!read_word(BFM_MINE_LINK_TABLE_POINTER, &table) ||
                !read_word(table + (u32)kind * 4u, &handler) ||
                !handler || (handler & 3u) ||
                !bfm_mine_link_run_handler(handler, slot)))
            return 0;
    }
    for (i = 0; i < 30; ++i) {
        u32 slot = BFM_MINE_LINK_SLOTS + i * 0x58u;
        u32 target;
        u16 kind;
        if (!read_word(slot + 0x20u, &target))
            return 0;
        if (target && (!read_half(target, &kind) || !copy_position(slot, target, kind)))
            return 0;
    }
    return 1;
}

void func_80165CA0(void)
{
    (void)bfm_mine_update_actor_links();
}
#endif
