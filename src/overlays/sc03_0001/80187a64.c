#include "psx_types.h"

struct TownWaypoint { s16 x, z; };
struct TownBody { u8 unknown[18]; s16 yaw; };
struct TownActor {
    u8 unknown0[4];
    s16 position[6];
    u8 unknown10[16];
    struct TownBody *body;
    u8 unknown24[0xe8 - 0x24];
    struct TownWaypoint *route;
    u8 unknownec[0x106 - 0xec];
    s16 index;
    u8 reverse;
    u8 speed;
};

extern s32 func_8016F1AC(void);
extern s32 func_8014B12C(void);
extern s32 func_8012B744(s16 *, s16 *);
extern s32 func_8012B608(s32, s32, s32);
extern void func_8012B178(struct TownActor *, s32);
extern void func_8012AD80(struct TownActor *);
extern s32 func_8012B030(struct TownActor *);
extern s32 func_8012BE98(struct TownActor *, s16 *);

s32 func_80187A64(void *object)
{
    struct TownActor *actor = object;
    s16 target[4];
    s32 speed;
    s32 angle;

    if (func_8016F1AC())
        return 0;

    if (!actor->reverse) {
        if (actor->route->x < actor->index)
            return 1;
    } else {
        if (actor->index <= 0)
            return 1;
    }

    target[0] = actor->route[actor->index].x;
    target[2] = actor->route[actor->index].z;
    speed = actor->speed;
    speed <<= 12;

    if (func_8014B12C()) {
        angle = func_8012B744(actor->position, target);
        actor->body->yaw = angle;
        speed *= 8;
    } else {
        angle = func_8012B744(actor->position, target);
        angle = func_8012B608(actor->body->yaw, angle, 4);
        actor->body->yaw += angle;
    }

    func_8012B178(actor, -speed);
    func_8012AD80(actor);
    actor->position[3] -= 16;
    func_8012B030(actor);

    if (func_8014B12C()) {
        if (func_8012BE98(actor, target) <= 4096) {
            if (!actor->reverse)
                actor->index++;
            else
                actor->index--;
        }
    } else {
        if (func_8012BE98(actor, target) <= 256) {
            if (!actor->reverse)
                actor->index++;
            else
                actor->index--;
        }
    }
    return 0;
}
