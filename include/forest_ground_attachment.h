#ifndef BFM_FOREST_GROUND_ATTACHMENT_H
#define BFM_FOREST_GROUND_ATTACHMENT_H

#include "psx_types.h"

typedef struct ForestGroundOrientation {
    u8 step, kind, pad02[2];
    u32 source, target;
    u8 pad0c[0x20 - 0x0c];
    s32 x, y, z;
} ForestGroundAttachment;

void func_801627E8(void);
s32 func_80162834(ForestGroundAttachment *attachment);
void func_801628F4(ForestGroundAttachment *attachment);
void func_80162968(ForestGroundAttachment *attachment);

#if defined(__mips__) && !defined(__LP64__)
extern ForestGroundAttachment D_8011F750[2];
extern u32 D_801896C0[], D_801896C4[];
#define BFM_GROUND_ATTACHMENT_ROOT() (&D_8011F750[0])
#define BFM_GROUND_ATTACHMENT_EFFECT(table, index, attachment) \
    (((void (*)(ForestGroundAttachment *))D_##table[index])(attachment))
#define BFM_GROUND_ATTACHMENT_CALL(table, index, attachment) \
    (((s32 (*)(ForestGroundAttachment *))D_##table[index])(attachment))
#define BFM_GROUND_ATTACHMENT_SOURCE(address) ((u8 *)(address))
#define BFM_GROUND_ATTACHMENT_TARGET(address) ((u8 *)(address))
#define BFM_GROUND_ATTACHMENT_HALF(bytes, offset) (*(s16 *)((bytes) + (offset)))
#define BFM_GROUND_ATTACHMENT_READ(bytes, offset) (*(u32 *)((bytes) + (offset)))
#define BFM_GROUND_ATTACHMENT_WRITE(bytes, offset, value) (*(u32 *)((bytes) + (offset)) = (value))
#define BFM_GROUND_ATTACHMENT_ORIENT(attachment) \
    func_80162968(attachment)
#else
#include <string.h>
/* Resolve the first controller in the active forest and check the callback's
 * four-byte table entry and active code before returning its original scalar. */
ForestGroundAttachment *bfm_forest_ground_attachment_root(void);
void bfm_forest_ground_attachment_effect(u32 table, u32 index, ForestGroundAttachment *attachment);
s32 bfm_forest_ground_attachment_call(u32 table, u32 index, ForestGroundAttachment *attachment);
/* A successful resolver supplies the live handle's at least 0x80 or 8 bytes;
 * an unowned handle stops the enclosing transaction. */
u8 *bfm_forest_ground_attachment_source(u32 address);
u8 *bfm_forest_ground_attachment_target(u32 address);
void bfm_forest_ground_attachment_orient(ForestGroundAttachment *attachment);
_Noreturn void bfm_forest_ground_attachment_missing(u32 address);

static inline ForestGroundAttachment *bfm_ground_attachment_root(void)
{
    ForestGroundAttachment *root = bfm_forest_ground_attachment_root();
    if (!root)
        bfm_forest_ground_attachment_missing(0x8011f750u);
    return root;
}
static inline u8 *bfm_ground_attachment_source(u32 address)
{
    u8 *bytes = bfm_forest_ground_attachment_source(address);
    if (!bytes)
        bfm_forest_ground_attachment_missing(address);
    return bytes;
}
static inline u8 *bfm_ground_attachment_target(u32 address)
{
    u8 *bytes = bfm_forest_ground_attachment_target(address);
    if (!bytes)
        bfm_forest_ground_attachment_missing(address);
    return bytes;
}
#define BFM_GROUND_ATTACHMENT_ROOT() bfm_ground_attachment_root()
#define BFM_GROUND_ATTACHMENT_EFFECT(table, index, attachment) \
    bfm_forest_ground_attachment_effect(0x##table##u, (index), (attachment))
#define BFM_GROUND_ATTACHMENT_CALL(table, index, attachment) \
    bfm_forest_ground_attachment_call(0x##table##u, (index), (attachment))
#define BFM_GROUND_ATTACHMENT_SOURCE(address) bfm_ground_attachment_source(address)
#define BFM_GROUND_ATTACHMENT_TARGET(address) bfm_ground_attachment_target(address)
#define BFM_GROUND_ATTACHMENT_ORIENT(attachment) bfm_forest_ground_attachment_orient(attachment)
static inline s16 bfm_forest_attachment_half(const u8 *bytes, unsigned offset)
{
    s16 value;
    memcpy(&value, bytes + offset, sizeof(value));
    return value;
}
static inline u32 bfm_forest_attachment_word(const u8 *bytes, unsigned offset)
{
    u32 value;
    memcpy(&value, bytes + offset, sizeof(value));
    return value;
}
static inline void bfm_forest_attachment_write(u8 *bytes, unsigned offset, u32 value)
{
    memcpy(bytes + offset, &value, sizeof(value));
}
#define BFM_GROUND_ATTACHMENT_HALF(bytes, offset) bfm_forest_attachment_half((bytes), (offset))
#define BFM_GROUND_ATTACHMENT_READ(bytes, offset) bfm_forest_attachment_word((bytes), (offset))
#define BFM_GROUND_ATTACHMENT_WRITE(bytes, offset, value) bfm_forest_attachment_write((bytes), (offset), (value))
#endif
#endif
