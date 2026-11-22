#ifndef BFM_STEAM_KNIGHT_RESOURCE_H
#define BFM_STEAM_KNIGHT_RESOURCE_H

#include "psx_types.h"

/* Keep the member names and types compatible with the resident initializer's
 * anonymous record type across translation units. */
typedef struct {
    s16 unk0, unk2;
    u8 pad4[0x1c];
    s32 unk20;
    u8 pad24[4];
    s16 unk28, unk2A, unk2C;
} TowerJointModelResource;

void func_8001CC3C(TowerJointModelResource *model, void *parent, s16 x, s16 y);
u16 *func_8001D074(s32 first, s32 limit);
struct Record;
struct Record *func_8001CFDC(int first, int limit);

u8 *func_8012913C(u16 kind);
u8 *func_801291C0(void);
void func_801292C8(char *record);
void *func_8012C1B8(void);

#endif
