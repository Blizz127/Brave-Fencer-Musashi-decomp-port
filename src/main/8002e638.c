/* Owner re-derivation from retail assembly (clean input set). */
/* Fades out the playing music (type-selected speed), or arms the pending-request fade when none is loaded. */
#include "psx_types.h"

typedef struct {
    u16 speed;
    u16 pad2;
} FadeEntry;

extern s16 func_8003836C(s16);
extern void func_8002E700(u16);
extern FadeEntry D_8006A99C[];
extern u8 D_8006AEEC;
extern s32 D_80078F10;
extern s32 D_800A46B4;
extern u16 D_800A46B8;
extern s8 D_800A46BA;
extern s16 D_800A4E86;
extern u16 D_800A4E8E;
extern s16 D_800A4EF4;

void func_8002E638(u16 type)
{
    u16 speed;

    if ((D_800A4E8E & 4) && func_8003836C(D_800A4E86) != 0) {
        func_8002E700(type);
        return;
    }
    if (D_80078F10 != 0) {
        if (type < 5) {
            speed = D_8006A99C[type].speed;
        } else {
            speed = 0x1F;
        }
        D_800A46B4 = 0xFFFF;
        D_800A46B8 = speed;
        D_800A46BA = 1;
        D_800A4EF4 = D_8006AEEC;
    }
}
