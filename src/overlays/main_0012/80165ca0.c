/* Owner re-derivation from retail assembly (clean input set). */
/* Runs per-slot handlers for 30 entries, then pushes each entry's position to its attached object. */
#include "psx_types.h"

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
            ((void (*)(Slot80165CA0 *))((u32 *)D_80126728)[slot->kind])(slot); /* guest handler table: u32 elements, cast at use */
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
