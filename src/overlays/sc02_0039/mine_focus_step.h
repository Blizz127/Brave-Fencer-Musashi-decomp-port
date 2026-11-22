#ifndef BFM_MINE_FOCUS_STEP_H
#define BFM_MINE_FOCUS_STEP_H
#include "camera.h"

typedef struct {
    u16 type;
    u8 pad002[0x1bd];
    u8 active;
} Obj801508F8;

typedef struct {
    u32 field[4];
} MineFocusParameters;

extern MineFocusParameters D_8019F170;
extern u32 D_80180D74;
extern FacingActor D_80126948;
s32 func_80021174(u32 clip, s32 *position);
s32 func_8015094C(Obj801508F8 *actor);
#endif
