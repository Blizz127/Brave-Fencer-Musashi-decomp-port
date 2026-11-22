#ifndef MINE_DOOR_PHASE_H
#define MINE_DOOR_PHASE_H

#include "psx_types.h"

typedef struct {
    u16 type, phase;
    u8 before_model[0x1c];
    u32 model;
    u8 before_shape[0x34];
    u32 shape;
    u16 contact_flags;
} MineDoorActor;

typedef s32 (*MineDoorPhase)(MineDoorActor *);
extern MineDoorPhase D_8017F684[];
extern u8 D_8017F65C[];
void *func_8012C1B8(void);
s32 func_80131FDC(MineDoorActor *actor);
s32 func_8013221C(MineDoorActor *actor);

#endif
