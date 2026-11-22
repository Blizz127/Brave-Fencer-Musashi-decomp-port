#include "forest_field.h"

typedef struct {
    u8 reserved00[4];
    u32 flags;
    s16 x, y, z;
    u8 reserved0e[2];
    s16 pitch;
    u8 reserved12[0x2c - 0x12];
    u16 mode;
    u8 reserved2e[0x80 - 0x2e];
    void *appearance;
} FieldDisplay;

typedef struct {
    u16 type, phase;
    u16 fraction_x;
    s16 x;
    u16 fraction_y;
    s16 y;
    u16 fraction_z;
    s16 z;
    u8 reserved10[0x20 - 0x10];
    FieldDisplay *body;
    u8 reserved24[0xae - 0x24];
    s16 layer;
    u8 reservedb0[0xcc - 0xb0];
    FieldDisplay *glow;
    u8 reservedd0[8];
    ForestMotion *motion;
    const u8 *rescue_record;
    u8 reservede0[0x10a - 0xe0];
    s16 has_prior_rescue;
} ForestCrystal;

extern void func_8012CAE4(ForestCrystal *actor);
extern int func_8012C354(ForestCrystal *actor, const void *resource);
extern void func_8001C810(FieldDisplay *model, const void *resource);
extern const u8 D_8019001C[], D_80190058[][16], D_801E91B4[];
extern ForestMotion D_801F7430[];

ForestFieldPhase func_8017C3C4;

void func_8017C3C4(void *actor_slot)
{
    ForestCrystal *actor = actor_slot;
    FieldDisplay *glow;
    s16 slot;

    if (func_800291B4(actor->rescue_record[0] + 0x3a)) {
        func_8012CAE4(actor);
        return;
    }

    if (!actor->body) {
        if (!func_8012C354(actor, D_8019001C))
            return;
        func_8001C810(actor->body, D_801E91B4);
        func_8012A828(actor, D_80190058[D_801152BA]);
        actor->body->mode |= 0x20;
        actor->body->appearance = D_801F7380;
        actor->body->flags |= 0x40000000;
        actor->layer = -3;
    }

    glow = func_8001D074(0x7e, 0x100);
    if (!glow)
        return;
    actor->glow = glow;
    func_8001CD50(glow, &D_801F7380[3]);
    D_801F7380[3].tint.red = D_801F7380[3].tint.blue = 128;
    D_801F7380[3].tint.green = 224;
    D_801F7380[3].tint.red_end = D_801F7380[3].tint.green_end = 0;
    D_801F7380[3].tint.blue_end = 0;
    func_800233CC(&D_801F7380[3], 0x38);
    glow->pitch = 1024;
    glow->x = actor->x;
    glow->y = (actor->y -= 4);
    glow->z = actor->z;
    glow->flags |= 0x50000000;

    D_801F7380[3].tint.red3 = D_801F7380[2].tint.blue3 = 255;
    D_801F7380[1].tint.red3 = D_801F7380[1].tint.blue3 = 255;
    D_801F7380[0].tint.red3 = D_801F7380[0].tint.blue3 = 255;
    D_801F7380[2].tint.green3 = 128;
    D_801F7380[1].tint.green3 = 128;
    D_801F7380[0].tint.green3 = 128;
    D_801F7430[D_801152BA] = D_801E91DC;
    {
        ForestMotion *pool = D_801F7430;
        actor->motion = pool + D_801152BA;
    }

    for (slot = 5; slot < 40; ++slot) {
        if (func_800291B4(slot + 0x3b))
            break;
    }
    if (slot != 40)
        actor->has_prior_rescue = 1;
    D_80115214 = 0;
    ++D_801152BA;
    ++actor->phase;
}
