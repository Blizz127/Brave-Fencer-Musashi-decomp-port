#ifndef BFM_SLOT_POSITION_UPDATES_H
#define BFM_SLOT_POSITION_UPDATES_H

#include "psx_types.h"

typedef struct ScenePositionAttachment {
    u8 pad00[0x14];
    s32 x, y, z;
} ScenePositionAttachment;

typedef struct ScenePositionTarget {
    u16 kind;
    u8 pad02[6];
    s16 x, y, z;
    u8 pad0e[0x1e];
    u16 flags2C;
    u8 pad2e[6];
    u32 sub34;
    u8 pad38[16];
    s32 x48, y4C, z50;
} ScenePositionTarget;

typedef struct ScenePositionSlot {
    u16 kind;
    u8 pad02[4];
    s16 x;
    u8 pad08[2];
    s16 y;
    u8 pad0c[2];
    s16 z;
    u8 pad10[16];
    u32 target;
    u8 pad24[32];
    s16 prevX, prevY, prevZ;
    u8 pad4a[30];
} ScenePositionSlot;

typedef void (*ScenePositionHandler)(ScenePositionSlot *slot);
extern ScenePositionSlot D_8011F9D0[20];
extern u32 D_80126D40;
void func_80162B1C(void);
void func_80162CC4(ScenePositionSlot *slot);

#if !defined(__mips__) || defined(__LP64__)
enum ScenePositionRefusal {
    BFM_POSITION_BAD_TABLE,
    BFM_POSITION_BAD_HANDLER,
    BFM_POSITION_BAD_TARGET,
    BFM_POSITION_BAD_ATTACHMENT
};

/* A valid table read can return a zero entry. Dynamic target resolvers return
 * the start of the requested field range or null when that range is unavailable. */
int bfm_position_handler_key(u32 table, u32 index, u32 *guest_handler);
ScenePositionHandler bfm_position_handler(u32 guest_handler);
u8 *bfm_position_target_bytes(u32 key, unsigned offset, unsigned width);
u8 *bfm_position_attachment_bytes(u32 key, unsigned offset, unsigned width);
void bfm_position_refused(enum ScenePositionRefusal reason, u32 key, unsigned detail);
#endif

#endif
