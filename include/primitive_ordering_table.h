#ifndef BFM_PRIMITIVE_ORDERING_TABLE_H
#define BFM_PRIMITIVE_ORDERING_TABLE_H

#include "psx_types.h"

typedef struct {
    u32 table; /* Guest address of the ordering table. */
    u8 pad[0x10];
} PrimitiveOrderingBank;

extern PrimitiveOrderingBank D_800A651C[];
extern u16 D_800B9A02;

void *func_80010A08(s32 size);
void func_80017F14(void *primitive, u32 depth, u32 flags);
void func_80018094(void *primitive, u32 depth, u32 flags);

/* Packet operations used by both game submission paths. */
u32 func_80058B40(s32, s32, s32, s32);
void func_80058C6C(void *bucket, void *primitive);
void func_8005A600(void *packet, s32, s32, s32, s32);

#endif
