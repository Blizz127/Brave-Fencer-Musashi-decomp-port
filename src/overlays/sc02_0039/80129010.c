#include "mine_attachment_slots.h"

extern s16 D_8011DB2C;
extern MineAttachedSlot D_8011DB30[];
extern u32 D_80126AEC;

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef struct {
    u8 reserved00[8];
    u16 x, y, z;
} MineAttachedPosition;

void func_80129010(void)
{
    MineAttachedSlot *p = D_8011DB30;
    s32 i;
    if (D_8011DB2C) {
        for (i = 0; i < 128; ++i, ++p) {
            if (p->type) {
                ((void (*)(MineAttachedSlot *))((u32 *)D_80126AEC)[p->type])(p);
                if (p->object) {
                    ((MineAttachedPosition *)p->object)->x = p->x;
                    ((MineAttachedPosition *)p->object)->y = p->y;
                    ((MineAttachedPosition *)p->object)->z = p->z;
                }
            }
        }
    }
}
#else
static u16 half_at(const u8 *p)
{
    return (u16)(p[0] | (u16)p[1] << 8);
}

static u32 object_address(const MineAttachedSlot *slot)
{
    const u8 *p = slot->bytes + 0x20;
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}

int bfm_mine_update_attachment_slots(void)
{
    unsigned i;
    if (!D_8011DB2C)
        return 1;
    for (i = 0; i < 128; ++i) {
        MineAttachedSlot *slot = &D_8011DB30[i];
        u16 type = half_at(slot->bytes);
        u32 entry, handler, target;
        unsigned axis;
        if (!type)
            continue;
        entry = D_80126AEC + (u32)type * 4u;
        if ((entry & 3u) || !bfm_mine_attachment_handler_at(entry, &handler) ||
                !handler || (handler & 3u) || !bfm_mine_run_attachment_handler(handler, slot))
            return 0;
        target = object_address(slot);
        if (!target)
            continue;
        for (axis = 0; axis < 3; ++axis) {
            u32 address = target + 8u + 2u * axis;
            if ((address & 1u) ||
                    !bfm_mine_store_attachment_half(address, half_at(slot->bytes + 6 + 4 * axis)))
                return 0;
            if (axis < 2)
                target = object_address(slot);
        }
    }
    return 1;
}

void func_80129010(void)
{
    (void)bfm_mine_update_attachment_slots();
}
#endif
