/* Owner re-derivation from retail assembly (clean input set). */
/* When enabled, runs each of 128 active slots' type handler, then copies its position into the attached object. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ u16 type;
    /* 0x02 */ u8 pad02[4];
    /* 0x06 */ u16 x;
    /* 0x08 */ u8 pad08[2];
    /* 0x0A */ u16 y;
    /* 0x0C */ u8 pad0C[2];
    /* 0x0E */ u16 z;
    /* 0x10 */ u8 pad10[0x10];
    /* 0x20 */ u32 obj; /* guest pointer: Obj80129010B */
    /* 0x24 */ u8 pad24[0x14];
} Obj80129010; /* size 0x38 */

typedef struct {
    /* 0x00 */ u8 pad00[8];
    /* 0x08 */ u16 x;
    /* 0x0A */ u16 y;
    /* 0x0C */ u16 z;
} Obj80129010B;

typedef void (*Handler80129010)(Obj80129010 *slot);

extern s16 D_8011DB2C;
extern Obj80129010 D_8011DB30[];
extern u32 D_80126AEC; /* guest pointer: handler table (u32 entries) */

void func_80129010(void)
{
    Obj80129010 *p;
    s32 i;

    p = D_8011DB30;
    if (D_8011DB2C != 0) {
        for (i = 0; i < 0x80; i++, p++) {
            if (p->type != 0) {
                ((Handler80129010)((u32 *)D_80126AEC)[p->type])(p);
                if (p->obj != 0) {
                    ((Obj80129010B *)p->obj)->x = p->x;
                    ((Obj80129010B *)p->obj)->y = p->y;
                    ((Obj80129010B *)p->obj)->z = p->z;
                }
            }
        }
    }
}
