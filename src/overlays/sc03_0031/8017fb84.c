/* SPDX-License-Identifier: MIT */
#include "psx_types.h"

typedef struct {
    u8 motion[28];
    s32 delay;
    void *sprite;
    u8 animation[8];
} ValleyPuff;

extern void func_8012931C(ValleyPuff *puff);
extern s32 func_80128ED8(void *sprite, u8 *animation);
extern void func_801292C8(ValleyPuff *puff);

/* Wait out any initial delay, then move the effect and retire its last cel. */
void func_8017FB84(ValleyPuff *puff)
{
    if (puff->delay == 0) {
        func_8012931C(puff);
        if (func_80128ED8(puff->sprite, puff->animation)) {
            func_801292C8(puff);
        }
    } else {
        --puff->delay;
    }
}
