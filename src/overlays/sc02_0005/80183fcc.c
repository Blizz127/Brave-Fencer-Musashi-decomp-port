#include "steam_knight_hit.h"

typedef struct {
    u8 bytes[4];
} CoreModelWord;

extern void func_801840E8(SteamKnightPart *core);
extern CoreModelWord D_801E44A0, D_801E2E70;
extern CoreModelWord D_801E44A4, D_801E2E74;
extern s16 D_801D323A;
extern void func_800233CC(void *shape, int length);
#if defined(__mips__) && !defined(__LP64__)
extern u8 D_801F12E8[], D_801D3230[];
extern void func_8001C924(u8 *model, void *list);
extern void func_80132784(SteamKnightPart *actor, SteamKnightPart *parent, int mode);
#include "steam_knight_resource.h"
extern void func_8001CD9C(u8 *model, void *shape);
#define BIND_LIST(actor) func_8001C924((u8 *)(actor)->model_address, D_801F12E8)
#define INSTALL_SHAPE(actor) (*(u8 **)((actor)->model_address + 0x20) = D_801D3230)
#define ATTACH(actor) func_80132784((actor), (SteamKnightPart *)(actor)->body_address, 21)
typedef u8 *CoreModel;
#define NEW_MODEL() ((u8 *)func_8001D074(126, 256))
#define MODEL_ADDRESS(model) ((u32)(model))
#define BIND_SHAPE(model) func_8001CD9C((model), &D_801E44A0)
#define SET_MODEL_FLAGS(model) do { *(u16 *)((model) + 0x2c) = 0xc010; *(u32 *)((model) + 4) |= 0xd0000000u; } while (0)
#else
#include <string.h>
extern void bfm_tower_actor84_model_list(u32 model, u32 list);
extern void bfm_steam_knight_core_attach(SteamKnightPart *actor, u32 parent, int mode);
extern u32 bfm_steam_knight_core_new_model(int type, int amount);
extern void bfm_steam_knight_core_ring_setup(u32 model, u32 shape);
extern u8 *bfm_steam_knight_core_model(u32 address);
#define BIND_LIST(actor) bfm_tower_actor84_model_list((actor)->model_address, 0x801f12e8u)
#define ATTACH(actor) bfm_steam_knight_core_attach((actor), (actor)->body_address, 21)
typedef u32 CoreModel;
#define NEW_MODEL() bfm_steam_knight_core_new_model(126, 256)
#define MODEL_ADDRESS(model) (model)
#define BIND_SHAPE(model) bfm_steam_knight_core_ring_setup((model), 0x801e44a0u)
static void install_shape(const SteamKnightPart *core)
{
    u32 shape_address = 0x801d3230u;
    memcpy(bfm_steam_knight_core_model(core->model_address) + 0x20,
           &shape_address, sizeof shape_address);
}
static void set_model_flags(u32 address)
{
    u8 *model = bfm_steam_knight_core_model(address);
    u32 flags;
    u16 mode = 0xc010;
    memcpy(model + 0x2c, &mode, sizeof mode);
    memcpy(&flags, model + 4, sizeof flags);
    flags |= 0xd0000000u;
    memcpy(model + 4, &flags, sizeof flags);
}
#define INSTALL_SHAPE(actor) install_shape(actor)
#define SET_MODEL_FLAGS(model) set_model_flags(model)
#endif

void func_80183FCC(SteamKnightPart *core)
{
    CoreModel model;
    BIND_LIST(core);
    INSTALL_SHAPE(core);
    D_801D323A = 0;
    core->field_0e = 24;
    /* Sample the core flag after installing its shared model record. */
    if (*(volatile u16 *)&core->field_fc & 1)
        func_801840E8(core);
    ATTACH(core);
    func_800233CC(&D_801E44A0, 40);
    D_801E44A0 = D_801E2E70;
    D_801E44A4 = D_801E2E74;
    model = NEW_MODEL();
    core->linked_address = MODEL_ADDRESS(model);
    if (model) {
        BIND_SHAPE(model);
        SET_MODEL_FLAGS(model);
    }
}
