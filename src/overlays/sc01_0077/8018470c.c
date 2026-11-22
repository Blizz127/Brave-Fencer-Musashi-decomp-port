/* On the marked swing tick test the two attack points, then submit the
 * variant's three spawned effects. Expiration restores the earlier action. */
#include "psx_types.h"

typedef struct {
    s16 x, y, z;
    u16 kind, variant, flags;
    s16 aim, yaw;
    u32 user;
} AttackSpawn;

extern u8 D_8018AF78[], D_8018AF88[], D_80196F3C[];
extern u16 D_80126B96;
extern s32 func_8012D5E4(s32 actor,s32 start,s32 end,s32 mode);
extern void func_8012F214(u8 *actor,u8 *anchor,s16 *world);
extern s32 func_8012C51C(void *placement,u8 *owner);
extern void func_8002D4C8(unsigned short command,unsigned short argument);
extern s32 func_8012BEE8(u8 *actor);
extern s32 func_8012A828(u8 *actor,u32 sequence);

#define GUEST_WORD(ptr) ((s32)(unsigned long)(ptr))
#define ACTOR_MODEL(actor) ((u8 *)(unsigned long)*(u32 *)((actor) + 0x20))

void func_8018470C(u8 *actor)
{
    s16 point[4];
    AttackSpawn spawn;
    if (*(s32 *)(actor + 0x1c) == 9) {
        if (func_8012D5E4(GUEST_WORD(actor), GUEST_WORD(D_8018AF78),
                          GUEST_WORD(D_8018AF78 + 8), 9) && (*(u16 *)(actor + 0x70) & 1))
            D_80126B96 |= 0x800;
        if (*(s16 *)(actor + 0x70) & 0x8000) {
            func_8012F214(actor, D_8018AF88, point);
            spawn.x = point[0];
            spawn.y = point[1];
            spawn.z = point[2];
            spawn.kind = 218;
            spawn.flags = 0;
            spawn.aim = 0x7fff;
            spawn.yaw = *(s16 *)(ACTOR_MODEL(actor) + 0x12);
            func_8012C51C(&spawn, actor);
            spawn.yaw = *(s16 *)(ACTOR_MODEL(actor) + 0x12) + 256;
            func_8012C51C(&spawn, actor);
            spawn.yaw = *(s16 *)(ACTOR_MODEL(actor) + 0x12) - 256;
            func_8012C51C(&spawn, actor);
        }
        func_8002D4C8(0x43e, 0);
    }
    if (func_8012BEE8(actor)) {
        u8 *configuration;
        *(s32 *)(actor + 0x1c) = 60;
        *(u16 *)(actor + 2) = 3;
        func_8012A828(actor, (u32)(unsigned long)D_80196F3C);
        configuration = (u8 *)(unsigned long)*(u32 *)(actor + 0x78);
        *(u16 *)(actor + 0x5c) = *(u16 *)(configuration + 2);
        *(u16 *)(actor + 0x5e) = 0;
        *(u16 *)(actor + 0xdc) = 0;
        *(s32 *)(actor + 0x1c) = 45;
    }
}
