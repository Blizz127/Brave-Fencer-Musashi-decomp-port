#include "steam_knight_actor84.h"

extern s32 func_8005C49C(void);

#if defined(__mips__) && !defined(__LP64__)
extern u32 func_8012C1B8(void);
extern void func_8012CAE4(SteamKnightPart *);
extern void func_8001C214(u32 model, u32 list);
extern void func_8012A828(SteamKnightPart *, u32 script);
extern u8 D_801966A8[], D_801966B0[], D_80190C0C[];
#define MODEL_LIST_EVEN ((u32)D_801966A8)
#define MODEL_LIST_ODD  ((u32)D_801966B0)
#define SCRIPT_ADDRESS  ((u32)D_80190C0C)
#else
#define func_8012C1B8 bfm_tower_actor84_allocate_model
#define func_8012CAE4 bfm_tower_actor84_dispose
#define func_8001C214 bfm_tower_actor84_model_list
#define func_8012A828 bfm_tower_actor84_animation
#define MODEL_LIST_EVEN 0x801966A8u
#define MODEL_LIST_ODD  0x801966B0u
#define SCRIPT_ADDRESS  0x80190C0Cu
#endif

void func_8018BA38(SteamKnightPart *actor)
{
    u8 workspace[16];
    u32 model, list;
    (void)workspace;

    model = func_8012C1B8();
    actor->model_address = model;
    if (!model) {
        func_8012CAE4(actor);
        return;
    }
    list = MODEL_LIST_EVEN;
    if (actor->variant & 1) list = MODEL_LIST_ODD;
    func_8001C214(model, list);
    func_8012A828(actor, SCRIPT_ADDRESS);
    actor->field_fc = func_8005C49C() & 0x3f0;
    /* Type 84 uses this shared halfword at 0xfe for its second random angle. */
    actor->break_count = func_8005C49C() & 0x3f0;
    actor->timer = 96;
    ++actor->state;
}
