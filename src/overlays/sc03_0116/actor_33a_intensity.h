#ifndef BFM_SC03_0116_ACTOR_33A_INTENSITY_H
#define BFM_SC03_0116_ACTOR_33A_INTENSITY_H

#include "actor_33a_ring.h"

extern s16 D_800B99DA;
s32 func_80181864(Actor33A *actor, struct Ring *ring);

#define ROOM_INTENSITY_TIMER 0xfc
#if defined(__mips__) && !defined(__LP64__)
#define READ_SIGNED_TIMER(actor) (*(s16 *)((u8 *)(actor) + ROOM_INTENSITY_TIMER))
#define READ_TIMER(actor) (*(u16 *)((u8 *)(actor) + ROOM_INTENSITY_TIMER))
#define WRITE_TIMER(actor, value) (READ_TIMER(actor) = (u16)(value))
#else
#include <string.h>
static inline u16 room_33a_timer(const Actor33A *actor)
{
    u16 value;
    memcpy(&value, (const u8 *)actor + ROOM_INTENSITY_TIMER, sizeof(value));
    return value;
}
static inline void room_33a_write_timer(Actor33A *actor, u16 value)
{
    memcpy((u8 *)actor + ROOM_INTENSITY_TIMER, &value, sizeof(value));
}
#define READ_SIGNED_TIMER(actor) ((s16)room_33a_timer(actor))
#define READ_TIMER(actor) room_33a_timer(actor)
#define WRITE_TIMER(actor, value) room_33a_write_timer((actor), (u16)(value))
#endif
#endif
