#include "actor_33a_ring.h"

#if !defined(__mips__) || defined(__LP64__)
#include <string.h>
#endif

/* This timer occupies two bytes in the placed actor's shared storage. */
#define ROOM_RING_TIMER_OFFSET 0xfc

void func_8018172C(Actor33A *actor)
{
    Actor33ARingModel *model;
    Actor33ARing *ring;

#if defined(__mips__) && !defined(__LP64__)
    model = (Actor33ARingModel *)actor->placed.model_address;
#else
    model = bfm_room_33a_ring_model(actor->placed.model_address);
#endif
    ring = &D_80197B60[actor->gate.parameter];
    actor->placed.z += 0x40;
    func_8001CD50(&model->init, &ring->shape);
    func_800233CC(&ring->shape, 0x80);
    func_80181844(actor, &ring->shape);
    ring->control.intensity = 0xf0;
    ring->control.secondary = 0;
    model->effect.options = 0x8040;
    model->effect.flags |= 0x50000000;
#if defined(__mips__) && !defined(__LP64__)
    *(u16 *)((u8 *)actor + ROOM_RING_TIMER_OFFSET) = 0;
#else
    memset((u8 *)actor + ROOM_RING_TIMER_OFFSET, 0, sizeof(u16));
#endif
    func_8012AD50(&actor->state);
}
