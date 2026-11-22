#ifndef BFM_SC03_0116_ACTOR_33A_H
#define BFM_SC03_0116_ACTOR_33A_H

#include "actor_334.h"

typedef union {
    Actor334State state;
    struct {
        u8 pad0[2];
        u16 phase;
        u8 pad4[2];
        s16 x;
        u8 pad8[2];
        s16 y;
        u8 padc[2];
        s16 z;
        u8 pad10[0x10];
        u32 model_address;
        u8 tail[0xe8];
    } placed;
} Actor33A;

typedef void (*Actor33APhase)(Actor33A *actor);
extern Actor33APhase D_80186344[8];

#if defined(__mips__) && !defined(__LP64__)
typedef struct {
    s16 unk0, unk2;
} Actor33AModel;
u16 *func_8012C194(void);
void func_8001CF00(Actor33AModel *model);
#else
/* The room stores a four-byte model address inside the actor. */
u32 bfm_room_33a_acquire_model(void);
void bfm_room_33a_initialize_model(u32 model_address);
#endif

void func_8012C098(char *actor);
void func_801814B0(Actor33A *actor);
void func_801814EC(Actor33A *actor);

#endif
