#include "forest_field.h"

typedef struct {
    u8 unused0[0x12];
    s16 heading;
} CrystalModel;

typedef struct {
    u8 unused0[4];
    void *model_data;
    void *animation;
} CrystalPresentation;

typedef struct {
    u8 unused0[2];
    u16 phase;
    u16 fraction_x;
    s16 x;
    u16 fraction_y;
    s16 y;
    u16 fraction_z;
    s16 z;
    u8 unused10[0x10];
    CrystalModel *model;
    u8 unused24[0x34];
    u32 field58;
    s16 field5c;
    u8 unused5e[0x7e];
    const CrystalPresentation *presentation;
} CrystalActor;

extern CrystalModel *func_8012C1B8(void);
extern void func_8001C214(CrystalModel *model, void *data);

ForestFieldPhase func_8017CE8C;

void func_8017CE8C(void *actor_slot)
{
    CrystalActor *actor = actor_slot;
    const CrystalPresentation *presentation = actor->presentation;
    ForestVector position;
    ForestVector direction;

    actor->model = func_8012C1B8();
    if (actor->model) {
        func_8001C214(actor->model, presentation->model_data);
        func_8012A828(actor, presentation->animation);
        actor->field5c = 0;
        actor->field58 = 0;
        position.x = actor->x;
        position.y = actor->y;
        position.z = actor->z;
        func_800D20C0(&position, &direction, 0);
        func_800D23D0(&direction);
        actor->model->heading = direction.y + 0x800;
        actor->phase++;
    }
}
