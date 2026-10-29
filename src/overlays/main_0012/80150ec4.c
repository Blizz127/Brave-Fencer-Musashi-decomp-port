/* func_80150EC4 - clean-room owner re-derivation (nmG). */
/* Latches the object's position and yaw into D_800AE6C8..CE, selects entry idx+1 of the table D_800AE6C4
 * into D_800AE6B0 (index kept in D_800AE6BE), and copies the 0xA4-byte block D_80126948 to D_801201F8. */
#include "psx_types.h"

typedef struct {
    u8 pad0[6];
    u16 x;   /* 0x06 */
    u8 pad8[2];
    u16 y;   /* 0x0A */
    u8 padC[2];
    u16 z;   /* 0x0E */
    u8 pad10[0x10];
    u32 sub; /* 0x20: guest address of Sub80150EC4 */
} Obj80150EC4;

typedef struct {
    u8 pad0[0x12];
    u16 yaw; /* 0x12 */
} Sub80150EC4;

typedef struct {
    u32 w[0x29];
} Blk80150EC4; /* 0xA4 bytes */

/* D_800AE6B0..D_800AE6CE viewed as one record (the global is declared u32 elsewhere; viewed through a cast at
 * each use, so every store is an in-struct store to a constant address: the varying o->f loads stay
 * after the previous store, while the constant-address table load is hoisted) */
typedef struct {
    u32 cur;      /* 0xB0: D_800AE6B0 */
    u8 pad04[0xA];
    u8 index;     /* 0xBE */
    u8 pad0F;
    u8 active;    /* 0xC0 */
    u8 pad11[3];
    u32 table;    /* 0xC4: guest pointer, u32 table */
    u16 x;        /* 0xC8 */
    u16 y;        /* 0xCA */
    u16 z;        /* 0xCC */
    u16 yaw;      /* 0xCE */
} Latch80150EC4;

extern u32 D_800AE6B0;
extern Blk80150EC4 D_801201F8;
extern u8 D_80126948[];

void func_80150EC4(Obj80150EC4 *o, s32 idx)
{
    ((Latch80150EC4 *)&D_800AE6B0)->active = 1;
    ((Latch80150EC4 *)&D_800AE6B0)->x = o->x;
    ((Latch80150EC4 *)&D_800AE6B0)->y = o->y;
    ((Latch80150EC4 *)&D_800AE6B0)->z = o->z;
    ((Latch80150EC4 *)&D_800AE6B0)->yaw = ((Sub80150EC4 *)o->sub)->yaw;
    ((Latch80150EC4 *)&D_800AE6B0)->cur = ((u32 *)((Latch80150EC4 *)&D_800AE6B0)->table)[idx + 1];
    ((Latch80150EC4 *)&D_800AE6B0)->index = idx + 1;
    D_801201F8 = (*(Blk80150EC4 *)&D_80126948);
}
