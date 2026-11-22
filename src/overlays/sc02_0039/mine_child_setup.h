#ifndef BFM_MINE_CHILD_SETUP_H
#define BFM_MINE_CHILD_SETUP_H

#include "mine_emitter_setup.h"
#include "mine_emitter_stages.h"
#include "mine_guest_abi.h"

typedef struct {
    MineActorStage stage;
    u8 pad04[10];
    u16 z;
    s32 position[3];
    u8 pad1c[4];
    u32 model;
    u8 pad24[0x24];
    u32 motion;
    u8 pad4c[0x0c];
    u32 shape;
    u16 contact_flags;
    u8 pad5e[6];
    u32 parent;
    u8 pad68[8];
    u16 subtype;
    u8 pad72[3];
    u8 state_byte;
    u8 pad76[0x8a];
    s16 step, variant;
    u8 tail[8];
} MineChildSetup;

typedef struct {
    u8 pad00[0x12];
    u16 yaw;
    u8 pad14[4];
    s16 scale_x, scale_y, scale_z;
    u8 pad1e[0x0e];
    u16 flags;
    u8 pad2e[6];
    u8 transform[0x20];
} MineChildSetupModel;

typedef struct {
    s32 offset[3];
    u32 scale;
} MineChildSetupRow;

typedef struct {
    s16 divisor;
    u8 following[6];
} MineChildSetupRate;

extern MineChildSetupRow D_8019B5B4[3];
extern MineChildSetupRate D_8019B5E6[3];
extern s32 D_8019E298[];
s32 func_8005C49C(void);
void func_8012B2CC(char *actor);
void func_8017EA20(MineActorStage *stage);

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef struct {
    s16 unk0, unk2;
    u8 pad4[0x1c];
    s32 unk20, unk24;
} MineChildListModel;
void func_8001C214(MineChildListModel *model, s32 *list);
void func_800484EC(void *transform, void *offset, void *position);
extern u8 D_8019B624[];
#else
/* Views cover the model or parent word selected at each use. They stay
 * valid until the next service call. */
MineChildSetupModel *bfm_mine_child_setup_model(u32 word);
MineChildSetup *bfm_mine_child_setup_parent(u32 word);
void bfm_mine_child_setup_bind(u32 model, s32 *list);
void bfm_mine_child_setup_transform(void *transform, void *offset, void *position);
extern u32 bfm_mine_child_setup_shape_address;
#endif
#endif
