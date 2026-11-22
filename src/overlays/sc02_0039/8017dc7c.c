/* Update the room rotor's visibility, model angle and positional sound. */
#include "mine_rotor_active.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef struct {
    u8 before_x[6];
    s16 x;
    u8 before_model[24];
    void *model;
    u8 before_option[76];
    s16 option;
    u8 before_sound[18];
    s16 playing;
} Rotor;

typedef struct {
    u32 reserved, flags;
    u8 before_angle[12];
    u16 angle;
} RotorModel;

extern s16 D_80126CAC[];
extern s16 D_8019B568[];
extern void func_8002D59C(u16 id, u16 arg, u16 extra);

void func_8017DC7C(Obj8014A2E4 *actor)
{
    Rotor *rotor = (Rotor *)actor;
    s16 distance = rotor->x - D_80126CAC[0];
    if (distance < 0)
        distance = -distance;
    if (distance > 2048) {
        ((RotorModel *)rotor->model)->flags |= 0x80000000;
    } else {
        ((RotorModel *)rotor->model)->flags &= 0x7fffffff;
        ((RotorModel *)rotor->model)->angle += D_8019B568[rotor->option];
    }

    distance -= 320;
    if (distance < 0)
        distance = 127;
    else if (distance >= 512)
        distance = 0;
    else
        distance = 127 - (distance >> 2);
    if (distance == 0) {
        if (rotor->playing) {
            func_8002D59C(4, 1757, rotor->option);
            rotor->playing = 0;
        }
    } else {
        func_8002D59C(1757, 0x1000 | distance, rotor->option);
        rotor->playing = 1;
    }
}
#else
#include "mine_actor_fields.h"

int mine_rotor_update(Obj8014A2E4 *actor, const MineRotorActive *services)
{
    if (!actor || !services || !services->model)
        return 0;
    uint8_t *data = (uint8_t *)actor;
    uint16_t delta = (uint16_t)(mine_actor_load16(data + 6) - services->player_x);
    int distance = mine_actor_signed16(delta);
    if (distance < 0)
        distance = mine_actor_signed16((uint16_t)-distance);
    MineRotorModel *model = services->model(services->context, mine_actor_load32(data + 0x20));
    if (!model)
        return 0;
    uint32_t flags = mine_actor_load32(model->bytes + 4);
    if (distance > 2048) {
        mine_actor_store32(model->bytes + 4, flags | UINT32_C(0x80000000));
    } else {
        mine_actor_store32(model->bytes + 4, flags & UINT32_C(0x7fffffff));
        uint16_t step;
        int16_t row = (int16_t)mine_actor_signed16(mine_actor_load16(data + 0x70));
        model = services->model(services->context, mine_actor_load32(data + 0x20));
        if (!model)
            return 0;
        uint16_t angle = mine_actor_load16(model->bytes + 20);
        if (!services->angular_step || !services->angular_step(services->context, row, &step))
            return 0;
        mine_actor_store16(model->bytes + 20, (uint16_t)(angle + step));
    }

    int offset = mine_actor_signed16((uint16_t)(distance - 320));
    uint16_t volume = 0;
    if (offset < 0)
        volume = 127;
    else if (offset < 512)
        volume = (uint16_t)(127 - offset / 4);
    if (!volume && !mine_actor_load16(data + 0x84))
        return 1;
    uint16_t id = volume ? 1757 : 4;
    uint16_t argument = volume ? (uint16_t)(0x1000 | volume) : 1757;
    uint16_t extra = mine_actor_load16(data + 0x70);
    if (!services->sound || !services->sound(services->context, id, argument, extra))
        return 0;
    mine_actor_store16(data + 0x84, volume != 0);
    return 1;
}
#endif
