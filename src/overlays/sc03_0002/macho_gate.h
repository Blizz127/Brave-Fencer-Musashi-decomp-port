#ifndef SC03_MACHO_GATE_H
#define SC03_MACHO_GATE_H

#include "psx_types.h"

typedef struct {
    u8 reserved[0x0c];
    s16 extent;
} MachoContact;

typedef struct {
    u16 type;
    u16 phase;
    u8 reserved04[0x1c - 0x04];
    s32 movement;
    u8 reserved20[0x34 - 0x20];
    u16 movement_timer;
    u8 reserved36[0x58 - 0x36];
    u32 contact_shape;
    s16 facing;
    u8 reserved5e[0x68 - 0x5e];
    MachoContact *contact;
    u8 reserved6c[0x98 - 0x6c];
    u16 substate;
    u8 reserved9a[0xd4 - 0x9a];
    s32 interaction;
    u8 reservedd8[0xe4 - 0xd8];
    const char *message;
} MachoActor;

typedef struct {
    u8 reserved[0x64];
    MachoActor *actor;
} MachoInteraction;

typedef void (*MachoFieldCallback)(void);

extern u8 func_80029178(s32 flag);
extern void func_80029124(s32 flag, s32 value);
extern u8 func_800291B4(s32 slot);
extern void func_800291A0(s32 slot, s32 value);
extern s32 func_80029504(void);
extern void func_8012CAE4(MachoActor *actor);
extern void func_8012A828(MachoActor *actor, const void *animation);
extern void func_8012E8E0(MachoActor *actor, const void *data);
extern void func_8012E88C(MachoActor *actor);
extern void func_8012E8A8(MachoActor *actor);
extern void func_8012B030(MachoActor *actor);
extern void func_8012B2CC(MachoActor *actor);
extern s32 func_8012BD14(MachoInteraction *interaction);
extern s32 func_801788B8(MachoActor *actor, MachoFieldCallback (*use)(MachoInteraction *));
extern s32 func_80178970(MachoActor *actor);
extern void func_80178D18(MachoActor *actor);
extern void func_80178CBC(MachoActor *actor, const void *script);
extern void func_80178BF8(void);
extern void func_80172710(void);
extern s32 func_80182D68(MachoActor *actor, const void *data);
extern s32 func_80183158(s32 first_hour, s32 end_hour);
extern void func_80183480(MachoActor *actor, const void *route, s32 direction, s32 rate);
extern s32 func_80183550(MachoActor *actor);
extern void func_80180DF4(MachoActor *actor);
extern void func_80180E90(MachoActor *actor);
extern MachoFieldCallback func_8018118C(MachoInteraction *interaction);
extern void (*D_8018990C[])(MachoActor *);
extern const u8 D_80189850[], D_80189860[], D_80189874[], D_80189884[], D_801898A4[], D_80189904[];
extern const u8 D_801BD368[], D_801BD8B0[], D_801BDBB8[], D_801BDC00[];
extern const char D_801A9EE0[], D_801A9F5C[], D_801A9FFC[];
#endif
