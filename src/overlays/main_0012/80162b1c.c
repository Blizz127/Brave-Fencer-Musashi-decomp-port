/* Owner re-derivation from retail assembly (clean input set). */
/* Runs per-slot handlers for 20 entries and pushes each entry's position to its attached object. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x14];
    /* 0x14 */ s32 x;
    /* 0x18 */ s32 y;
    /* 0x1C */ s32 z;
} Sub80162B1C;

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
} Target80162B1C;

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
    u8 pad24[0x44 - 0x24];
    /* 0x44 */ s16 prevX;
    /* 0x46 */ s16 prevY;
    /* 0x48 */ s16 prevZ;
    u8 pad4A[0x68 - 0x4A];
} Slot80162B1C;

extern Slot80162B1C D_8011F9D0[];
extern u32 D_80126D40;
extern void func_80162CC4(Slot80162B1C *slot);

void func_80162B1C(void)
{
    u32 *table;
    void (*fn)(Slot80162B1C *);
    Slot80162B1C *slot;
    Target80162B1C *t;
    Sub80162B1C *sub;
    s32 i;

    table = (u32 *)D_80126D40; /* guest table of handler pointers: held as u32, cast at use */
    for (i = 0; i < 20; i++) {
        slot = &D_8011F9D0[i];
        slot->prevX = slot->x;
        slot->prevY = slot->y;
        slot->prevZ = slot->z;
        if (slot->kind == 0) {
            continue;
        }
        fn = (void (*)(Slot80162B1C *))table[slot->kind - 1];
        if (fn != 0) {
            fn(slot);
        } else {
            func_80162CC4(slot);
        }
        t = (Target80162B1C *)slot->target;
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
        } else if ((sub = (Sub80162B1C *)t->sub34) != 0) {
            t->x = slot->x;
            sub->x = t->x;
            t->y = slot->y;
            ((Sub80162B1C *)t->sub34)->y = t->y;
            t->z = slot->z;
            ((Sub80162B1C *)t->sub34)->z = t->z;
        } else {
            t->x = slot->x;
            t->y = slot->y;
            t->z = slot->z;
        }
    }
}
