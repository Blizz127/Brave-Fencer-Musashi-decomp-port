#ifndef BFM_SC02_ACTOR_PICKUP_H
#define BFM_SC02_ACTOR_PICKUP_H

#include "../main_0012/scene_object_probes.h"
#include "mine_guest_abi.h"

void func_801506A4(void *self, Obj8014A2E4 *ent);
s32 func_80147324(u16 sound);
s32 func_8014ADA8(void *self, s32 amount);
s32 func_8014ACE8(char *self, s32 category, s32 amount);
void func_8014BB24(void *self, s32 delta, s32 mode);
void func_80150820(void *self, s32 amount);
s32 func_8015086C(void *self);
extern u16 D_80078EB4;

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
enum {
    BFM_PICKUP_TABLE_49,
    BFM_PICKUP_TABLE_50,
    BFM_PICKUP_TABLE_51
};

int bfm_pickup_table_value(unsigned table, s16 index, u16 *value);
void bfm_pickup_table_refused(unsigned table, s16 index);
#endif

#endif
