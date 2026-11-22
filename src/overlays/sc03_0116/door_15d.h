#ifndef BFM_SC03_0116_DOOR_15D_H
#define BFM_SC03_0116_DOOR_15D_H

#include "psx_types.h"

typedef struct {
    u16 type;
    u16 phase;
    u8 before_model[0x1c];
    u32 model_address;
    u8 before_shape[0x34];
    u32 shape_address;
    u16 contact_flags;
    u8 tail[0xae];
} Door15DActor;

s32 func_80131F64(Door15DActor *actor);
s32 func_80132144(Door15DActor *actor);

#endif
