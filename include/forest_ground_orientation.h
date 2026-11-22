#ifndef BFM_FOREST_GROUND_ORIENTATION_H
#define BFM_FOREST_GROUND_ORIENTATION_H

#include "forest_ground_attachment.h"

enum ForestOrientationRole {
    FOREST_ORIENTATION_SOURCE,
    FOREST_ORIENTATION_TARGET,
    FOREST_ORIENTATION_SUBOBJECT
};

#if defined(__mips__) && !defined(__LP64__)
s32 func_8004CFEC(s32 y, s32 x);
s32 func_80047D3C(s32 squared);
void func_80013F3C(s16 *matrix);
void func_800126C4(void *matrix, s32 angle);
void func_800123F0(void *matrix, s32 angle);
void func_80012558(void *matrix, s32 angle);
#define FOREST_ORIENTATION_BYTES(address, size, role) ((u8 *)(address))
#define FOREST_ORIENTATION_ANGLE(y, x) func_8004CFEC(y, x)
#define FOREST_ORIENTATION_MAGNITUDE(squared) func_80047D3C(squared)
#define FOREST_ORIENTATION_IDENTITY(matrix) func_80013F3C((s16 *)(matrix))
#define FOREST_ORIENTATION_ROTATE(service, matrix, angle) func_##service(matrix, angle)
#define FOREST_ORIENTATION_STORE(bytes, offset, value) (*(s16 *)((bytes) + (offset)) = (value))
#else
/* Each handle belongs to the active scene and must cover the complete span.
 * Keep resolved handles alive for the enclosing call. A late refusal also
 * rolls back previous service effects and writes performed by the game. */
u8 *bfm_forest_orientation_bytes(u32 address, u32 size, enum ForestOrientationRole role);
s32 bfm_forest_orientation_angle(s32 y, s32 x);
s32 bfm_forest_orientation_magnitude(s32 squared);
void bfm_forest_orientation_identity(u8 *matrix);
void bfm_forest_orientation_rotate(u32 service, u8 *matrix, s32 angle);

static inline u8 *bfm_checked_orientation_bytes(u32 address, u32 size, enum ForestOrientationRole role)
{
    u8 *bytes = bfm_forest_orientation_bytes(address, size, role);
    if (!bytes)
        bfm_forest_ground_attachment_missing(address);
    return bytes;
}

static inline void bfm_orientation_store(u8 *bytes, unsigned offset, s16 value)
{
    memcpy(bytes + offset, &value, sizeof value);
}

#define FOREST_ORIENTATION_BYTES(address, size, role) bfm_checked_orientation_bytes(address, size, role)
#define FOREST_ORIENTATION_ANGLE(y, x) bfm_forest_orientation_angle(y, x)
#define FOREST_ORIENTATION_MAGNITUDE(squared) bfm_forest_orientation_magnitude(squared)
#define FOREST_ORIENTATION_IDENTITY(matrix) bfm_forest_orientation_identity(matrix)
#define FOREST_ORIENTATION_ROTATE(service, matrix, angle) bfm_forest_orientation_rotate(0x##service##u, matrix, angle)
#define FOREST_ORIENTATION_STORE(bytes, offset, value) bfm_orientation_store(bytes, offset, value)
#endif
#endif
