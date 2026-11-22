/* Advance the village actor along two paths, then select its next action. */
#include "../sc03_0053/porch.h"

typedef struct {
    u8 pad00[0xb0];
    s32 field_b0;
    u8 padb4[0x108 - 0xb4];
    u16 field_108;
} VillagePhaseLink;

typedef struct {
    u8 pad00[0x34];
    u16 phase;
    u8 pad36[0xcc - 0x36];
    u32 link_address;
    u8 padd0[0x105 - 0xd0];
    u8 next_action;
} VillagePhaseActor;

#if defined(__mips__) && !defined(__LP64__)
#define PHASE_LINK(actor) ((VillagePhaseLink *)(actor)->link_address)
#else
/* The actor stores its linked object's 32-bit game address. */
extern VillagePhaseLink *bfm_village_phase_link(u32 address);
#define PHASE_LINK(actor) bfm_village_phase_link((actor)->link_address)
#endif

extern const s16 D_801B9608[], D_801B9614[];
extern s32 func_801886A0(s16 id);
extern void func_80187994(void *actor, const s16 *path, u8 reverse, u8 speed);
extern s32 func_80187A64(void *actor);
extern void func_801858A0(void *actor);

void func_801859AC(VillagePhaseActor *actor)
{
    switch (actor->phase) {
    case 0:
        if (func_801886A0(5)) {
            func_80187994(actor, D_801B9608, 0, 112);
            actor->phase = 1;
        }
        break;
    case 1:
        func_801886A0(5);
        if (func_80187A64(actor)) {
            func_80187994(actor, D_801B9614, 1, 112);
            actor->phase = 2;
            PHASE_LINK(actor)->field_108 = 0;
            PHASE_LINK(actor)->field_b0 = 2;
        }
        break;
    case 2:
        if (func_80187A64(actor))
            actor->phase = 3;
        break;
    case 3:
        if (func_801886A0(5)) {
            actor->phase = 4;
            func_80187994(actor, D_801B9608, 1, 112);
        }
        break;
    case 4:
        func_801886A0(5);
        if (func_80187A64(actor)) {
            switch (actor->next_action) {
            case 7:
                func_801858A0(actor);
                break;
            case 18:
                func_801858CC(actor);
                break;
            case 23:
                func_801858F8(actor);
                break;
            case 31:
                func_80185924(actor);
                break;
            case 2:
            default:
                func_80185874(actor);
                break;
            }
        }
        break;
    }
}
