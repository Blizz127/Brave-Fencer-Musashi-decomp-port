/* Prepare the transient models for a Tower joint effect. */
#include "steam_knight_hit.h"

extern s32 func_8018B300(void);
extern u8 *func_8012913C(u16 kind);
extern void func_801292C8(char *resource);
extern void *func_8012C1B8(void);
#include "steam_knight_actor_dispose.h"

#if defined(__mips__) && !defined(__LP64__)
#include "steam_knight_model_binding.h"
typedef struct { s16 x,y,rx,ry; } JointPlacement;
typedef struct { u8 bytes[32]; } JointBlock;
extern JointPlacement D_801965DC[];
extern JointBlock D_801E44F8[][4];
#define H(p, offset) (*(s16 *)((u8 *)(p) + (offset)))
#define W(p, offset) (*(u32 *)((u8 *)(p) + (offset)))
#define B(p, offset) (*((u8 *)(p) + (offset)))
#define SET_H(p,n,v) (H(p,n) = (v))
#define SET_W(p,n,v) (W(p,n) = (v))
#define KEY(p) ((u32)(p))
#define POINTER(key) ((u8 *)(key))
#define RESOURCES u8 *resources[10]
#define KEEP(i,p) (resources[i] = (p))
#define HAVE_RESOURCE(i) (resources[i])
#define RESOURCE(i) (resources[i])
#define PLACEMENT(part, field) (D_801965DC[H(part,0x100)].field)
#define BLOCK(slot, i) ((u32)&D_801E44F8[(slot)][(i)])
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 key);
extern u32 bfm_tower_spawn_address(const void *p);
extern u32 bfm_tower_joint_resource_slots(void);
extern void bfm_tower_armor_bind_model(void *model);
#define func_8001C2C4 bfm_tower_armor_bind_model
static s16 half(const void *p, unsigned offset)
{
    s16 v; memcpy(&v, (const u8 *)p + offset, sizeof v); return v;
}
static u32 word(const void *p, unsigned offset)
{
    u32 v; memcpy(&v, (const u8 *)p + offset, sizeof v); return v;
}
static void set_half(void *p, unsigned offset, s16 value)
{
    memcpy((u8 *)p + offset, &value, sizeof value);
}
static void set_word(void *p, unsigned offset, u32 value)
{
    memcpy((u8 *)p + offset, &value, sizeof value);
}
#define H(p,n) half(p,n)
#define W(p,n) word(p,n)
#define B(p,n) (((u8 *)(p))[n])
#define SET_H(p,n,v) set_half(p,n,(s16)(v))
#define SET_W(p,n,v) set_word(p,n,v)
#define KEY(p) ((p) ? bfm_tower_spawn_address(p) : 0u)
#define POINTER(key) ((u8 *)bfm_tower_spawn_pointer(key))
#define RESOURCES u32 resources = bfm_tower_joint_resource_slots()
#define KEEP(i,p) set_word(POINTER(resources + (u32)(i)*4u), 0, KEY(p))
#define HAVE_RESOURCE(i) word(POINTER(resources + (u32)(i)*4u), 0)
#define RESOURCE(i) POINTER(HAVE_RESOURCE(i))
#define FIELD_x 0u
#define FIELD_y 2u
#define FIELD_rx 4u
#define FIELD_ry 6u
#define PLACEMENT(part, field) half(POINTER(0x801965dcu + (u32)(s32)H(part,0x100)*8u + FIELD_##field), 0)
#define BLOCK(slot,i) (0x801e44f8u + (u32)(slot)*128u + (u32)(i)*32u)
#endif

void func_8018968C(SteamKnightPart *part)
{
    RESOURCES;
    int i, failures, mode, slot;
    u8 *resource;
    void *model;

    SET_H(part,0x102,func_8018B300());
    slot = H(part,0x102);
    failures = 0;
    SET_W(part,12,0);
    SET_W(part,8,0);
    SET_W(part,4,0);
    SET_H(part,0x100,(u16)H(part,0x70) >> 8);
    SET_H(part,0x70,B(part,0x70));
    if (slot < 0) {
        failures = 1;
    } else {
        if (H(part,0x100) < 2) {
            SET_H(part,0xfe,4);
            mode = 0;
        } else {
            SET_H(part,0xfe,1);
            mode = 1;
        }
        for (i=0; i<H(part,0xfe); i++) {
            resource = func_8012913C(9);
            KEEP(i,resource);
            if (resource) {
                SET_H(POINTER(W(resource,0x20)),0x1e,mode);
            } else {
                ++failures;
            }
        }
    }
    model = func_8012C1B8();
    SET_W(part,0x20,KEY(model));
    if (!model) ++failures;
    if (failures != 0) {
        for (i=0; i<H(part,0xfe); ++i) {
            if (HAVE_RESOURCE(i)) func_801292C8((char *)RESOURCE(i));
        }
        func_8012CAE4((u8 *)part);
        return;
    }
    func_8001C2C4(model);
    SET_H(part,6,PLACEMENT(part,x));
    SET_H(part,10,PLACEMENT(part,y));
    SET_H(POINTER(W(part,0x20)),0x10,PLACEMENT(part,rx));
    SET_H(POINTER(W(part,0x20)),0x12,PLACEMENT(part,ry));
    for (i=0; i<H(part,0xfe); i++) {
        if (i==0) {
            SET_W(RESOURCE(0),0x2c,KEY(part));
        } else {
            SET_W(RESOURCE(i),0x2c,0);
        }
        SET_W(RESOURCE(i),0x34,BLOCK(slot,i));
    }
    SET_H(part,0xfc,0);
    part->state++;
}
