/* Owner re-derivation from retail assembly (clean input set). */
/* Integrates the three velocities at +0x10 (by +0x44) and the three positions at +0x04 (by the velocities). */
#include "psx_types.h"

typedef struct {
    u8 pad000[4];
    /* 0x04 */ s32 pos[3];
    /* 0x10 */ s32 vel[3];
    u8 pad1C[0x28];
    /* 0x44 */ s32 acc[3];
} Obj8012AD80;

void func_8012AD80(Obj8012AD80 *obj)
{
    obj->vel[0] += obj->acc[0];
    obj->vel[1] += obj->acc[1];
    obj->vel[2] += obj->acc[2];
    obj->pos[0] += obj->vel[0];
    obj->pos[1] += obj->vel[1];
    obj->pos[2] += obj->vel[2];
}
