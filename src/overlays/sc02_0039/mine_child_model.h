#ifndef BFM_MINE_CHILD_MODEL_H
#define BFM_MINE_CHILD_MODEL_H

#include "psx_types.h"

typedef struct {
    u8 before_model_index[0x102];
    s16 model_index;
    u8 tail[8];
} MineChildModelActor;

/* The query additionally uses sample offsets following this common prefix. */
typedef struct {
    u16 count, extent;
    u16 feedback_a, feedback_b;
} MineChildModelRecord;

extern MineChildModelRecord *D_8019B618[3];
extern u16 D_80126B96, D_80126B98;
void func_8012B2CC(char *actor);
s32 func_8017EF5C(MineChildModelActor *actor, MineChildModelRecord *record);
void func_8017EED8(MineChildModelActor *actor);

#endif
