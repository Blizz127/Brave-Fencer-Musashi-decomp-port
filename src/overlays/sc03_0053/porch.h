#ifndef BFM_SC03_0053_PORCH_H
#define BFM_SC03_0053_PORCH_H

#include "event_flags.h"

/* These routines access three halfwords in a larger village actor record. */
typedef struct {
    u8 reserved00[2];
    u16 action;
    u8 reserved04[0x30];
    u16 phase;
    u8 reserved36[0xC6];
    u16 cue;
} PorchActor;

s32 func_80029178(u32 flag);
void func_80029514(s32 value);
PorchActor *func_8012E544(s32 id);
void func_8012E88C(void *actor);
void func_8012E8E0(u8 *actor, u16 *position);
s32 func_80178970(void *actor);
s32 func_801789AC(void *actor);
void func_80178CBC(void *actor, void *script);
void func_80178D18(void *actor);
void func_80185874(void *actor);
void func_801858CC(void *actor);
void func_801858F8(void *actor);
void func_80185924(void *actor);
void func_80187344(void *actor);
void func_801EF560(PorchActor *actor);

#endif
