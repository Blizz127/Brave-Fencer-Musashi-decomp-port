#include "forest_field.h"

typedef struct {
    s16 active;
    u8 pad02[0x16];
    s16 scale_x;
    s16 scale_y;
} CrystalEffect;

typedef struct {
    u8 pad00[0x0c];
    const void *script;
} RescueRecord;

typedef struct {
    u16 kind;
    u16 phase;
    u8 pad04[0xcc];
    CrystalEffect *effect;
    u8 padD4[8];
    const RescueRecord *record;
} CrystalActor;


ForestFieldPhase func_8017CF40;

void func_8017CF40(void *actor_slot)
{
    CrystalActor *actor = actor_slot;
    CrystalEffect *effect = actor->effect;
    const RescueRecord *record = actor->record;

    effect->scale_x = effect->scale_y -= 0xc0;
    if (effect->scale_x < 0) {
        effect->active = 0;
        /* Fetch the script word from the live record after disabling the effect. */
        func_80178B18(actor, *(const void *volatile *)&record->script);
        actor->phase++;
    }
}
