#include "forest_field.h"

typedef struct ForestCrystalActor {
    u16 kind;
    u16 phase;
} ForestCrystalActor;


ForestFieldPhase func_8017D058;

void func_8017D058(void *actor_slot)
{
    ForestCrystalActor *actor = actor_slot;
    func_8017C294(actor, func_8005C49C() & 15u);
    if (func_80178970(actor)) {
        func_80178D18(actor);
        actor->phase = 1;
    }
}
