#ifndef BFM_SC03_0116_ACTOR_33A_RING_H
#define BFM_SC03_0116_ACTOR_33A_RING_H

#include "actor_33a.h"

struct Pt {
    s16 x, y;
};
struct Ring {
    u8 pad[0x10];
    struct Pt pt[12];
};

typedef union {
    struct Ring shape;
    struct {
        u32 intensity, secondary;
    } control;
} Actor33ARing;

typedef struct {
    s16 unk0, unk2;
    u8 pad4[0xa];
    s16 unkE;
    u8 pad10[0xe];
    s16 unk1E;
    s32 unk20;
    u8 pad24[8];
    s16 unk2C;
} Actor33ARingModelPrefix;

typedef union {
    Actor33ARingModelPrefix init;
    struct {
        u8 before_flags[4];
        u32 flags;
        u8 before_options[0x24];
        u16 options;
        u8 tail[0xa];
    } effect;
} Actor33ARingModel;

extern Actor33ARing D_80197B60[];
void func_8001CD50(Actor33ARingModelPrefix *model, void *shape);
void func_800233CC(struct Ring *shape, u16 radius);
void func_80181844(Actor33A *actor, struct Ring *shape);
void func_8018172C(Actor33A *actor);
#if !defined(__mips__) || defined(__LP64__)
Actor33ARingModel *bfm_room_33a_ring_model(u32 model_address);
#endif

#endif
