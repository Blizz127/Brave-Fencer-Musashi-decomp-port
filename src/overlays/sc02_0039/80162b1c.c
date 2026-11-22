/* Save each slot's coordinates, dispatch its game handler, then update its attachment. */
#include "slot_position_updates.h"

#if defined(__mips__) && !defined(__LP64__)
#define BFM_POSITION_ORIGINAL
#else
static int dispatch_slot(u32 table, ScenePositionSlot *slot)
{
    u32 key;
    u32 index = (u32)slot->kind - 1;
    ScenePositionHandler handler;
    if (!table || (table & 3u) || table > 0xffffffffu - 4u * index - 3u
        || !bfm_position_handler_key(table, index, &key)) {
        bfm_position_refused(BFM_POSITION_BAD_TABLE, table, index);
        return 0;
    }
    if (!key) {
        func_80162CC4(slot);
        return 1;
    }
    handler = (key & 3u) ? (ScenePositionHandler)0 : bfm_position_handler(key);
    if (!handler) {
        bfm_position_refused(BFM_POSITION_BAD_HANDLER, key, index);
        return 0;
    }
    handler(slot);
    return 1;
}

static u8 *position_field(int attached, u32 key, unsigned offset, unsigned width)
{
    u8 *bytes = 0;
    if (key && key <= 0xffffffffu - offset - (width - 1u)
        && ((key + offset) & (width - 1u)) == 0) {
        bytes = attached ? bfm_position_attachment_bytes(key, offset, width)
                         : bfm_position_target_bytes(key, offset, width);
    }
    if (!bytes) {
        bfm_position_refused(attached ? BFM_POSITION_BAD_ATTACHMENT : BFM_POSITION_BAD_TARGET,
                             key, offset);
    }
    return bytes;
}

static int read_target(u32 key, unsigned offset, unsigned width, u32 *value)
{
    const u8 *bytes = position_field(0, key, offset, width);
    if (!bytes) {
        return 0;
    }
    *value = (u32)bytes[0] | (u32)bytes[1] << 8;
    if (width == 4) {
        *value |= (u32)bytes[2] << 16 | (u32)bytes[3] << 24;
    }
    return 1;
}

static int write_field(int attached, u32 key, unsigned offset, unsigned width, u32 value)
{
    u8 *bytes = position_field(attached, key, offset, width);
    if (!bytes) {
        return 0;
    }
    bytes[0] = (u8)value;
    bytes[1] = (u8)(value >> 8);
    if (width == 4) {
        bytes[2] = (u8)(value >> 16);
        bytes[3] = (u8)(value >> 24);
    }
    return 1;
}

static int write_pair(u32 key, unsigned axis, u32 wide_owner, int attached, s16 value)
{
    unsigned wide_offset = (attached ? 0x14u : 0x48u) + 4 * axis;
    return write_field(0, key, 8 + 2 * axis, 2, (u16)value)
        && write_field(attached, wide_owner, wide_offset, 4, (u32)(s32)value);
}

static int update_target(ScenePositionSlot *slot)
{
    u32 kind, attachment, flags;
    u32 key = slot->target;
    s16 value;
    if (!key) {
        return 1;
    }
    if (!read_target(key, 0, 2, &kind)) {
        return 0;
    }
    if (kind == 1) {
        if (!write_pair(key, 0, key, 0, slot->x)
            || !write_pair(key, 1, key, 0, slot->y)
            || !read_target(key, 0x2c, 2, &flags)) {
            return 0;
        }
        value = slot->z;
        return write_field(0, key, 12, 2, (u16)value)
            && write_field(0, key, 0x2c, 2, flags | 1u)
            && write_field(0, key, 0x50, 4, (u32)(s32)value);
    }
    if (!read_target(key, 0x34, 4, &attachment)) {
        return 0;
    }
    if (!attachment) {
        return write_field(0, key, 8, 2, (u16)slot->x)
            && write_field(0, key, 10, 2, (u16)slot->y)
            && write_field(0, key, 12, 2, (u16)slot->z);
    }
    if (!write_pair(key, 0, attachment, 1, slot->x)) {
        return 0;
    }
    /* A previous attachment write may change the handle stored at +0x34. */
    value = slot->y;
    if (!read_target(key, 0x34, 4, &attachment)
        || !write_pair(key, 1, attachment, 1, value)) {
        return 0;
    }
    value = slot->z;
    return read_target(key, 0x34, 4, &attachment)
        && write_pair(key, 2, attachment, 1, value);
}
#endif

void func_80162B1C(void)
{
#ifdef BFM_POSITION_ORIGINAL
    u32 *table;
    ScenePositionHandler fn;
    ScenePositionTarget *t;
    ScenePositionAttachment *sub;
#else
    u32 table;
#endif
    ScenePositionSlot *slot;
    s32 i;

#ifdef BFM_POSITION_ORIGINAL
    table = (u32 *)D_80126D40;
#else
    table = D_80126D40;
#endif
    for (i = 0; i < 20; i++) {
        slot = &D_8011F9D0[i];
        slot->prevX = slot->x;
        slot->prevY = slot->y;
        slot->prevZ = slot->z;
        if (slot->kind == 0) {
            continue;
        }
#ifdef BFM_POSITION_ORIGINAL
        fn = (ScenePositionHandler)table[slot->kind - 1];
        if (fn != 0) {
            fn(slot);
        } else {
            func_80162CC4(slot);
        }
        t = (ScenePositionTarget *)slot->target;
        if (t == 0) {
            continue;
        }
        if (t->kind == 1) {
            t->x = slot->x;
            t->x48 = t->x;
            t->y = slot->y;
            t->y4C = t->y;
            t->z = slot->z;
            t->z50 = t->z;
            t->flags2C |= 1;
        } else if ((sub = (ScenePositionAttachment *)t->sub34) != 0) {
            t->x = slot->x;
            sub->x = t->x;
            t->y = slot->y;
            ((ScenePositionAttachment *)t->sub34)->y = t->y;
            t->z = slot->z;
            ((ScenePositionAttachment *)t->sub34)->z = t->z;
        } else {
            t->x = slot->x;
            t->y = slot->y;
            t->z = slot->z;
        }
#else
        if (!dispatch_slot(table, slot) || !update_target(slot)) {
            return;
        }
#endif
    }
}
