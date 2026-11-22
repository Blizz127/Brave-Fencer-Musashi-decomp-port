#include "../sc03_0053/porch.h"

extern u16 D_80192864[];
extern u8 D_801C36E8[];

/* These two names describe this caller's use of the room's services. */
extern void bfm_village_transition_2a828(PorchActor *actor, void *record);
extern void bfm_village_transition_87750(PorchActor *actor);

void func_80185950(PorchActor *actor)
{
    actor->action = 27;
    actor->phase = 0;
    func_8012E8E0((u8 *)actor, D_80192864);
    func_8012E88C(actor);
    bfm_village_transition_2a828(actor, D_801C36E8);
    bfm_village_transition_87750(actor);
}
