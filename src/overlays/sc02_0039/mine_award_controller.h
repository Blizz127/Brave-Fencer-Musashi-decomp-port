#ifndef BFM_MINE_AWARD_CONTROLLER_H
#define BFM_MINE_AWARD_CONTROLLER_H

#include "psx_types.h"

typedef struct { u8 bytes[8]; } MineAwardVector;
typedef void (*MineAwardPhase)(u8 *actor);
extern u8 D_80126B58[];
extern u16 D_80126B38;
extern s16 D_80126B3A;
extern u16 D_80126B3C;
extern s16 D_8011F9C8, D_80126938;
extern u32 D_8012750C;
extern MineAwardPhase D_8018271C[];
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
extern s32 D_80127508;
extern s32 func_8014C118(s32 controller, s32 type, s16 id);
#else
extern void *D_80127508;
extern void *func_8014C118(u8 *controller, s32 type, s16 id);
#endif

extern void func_80174684(MineAwardPhase phase);

void func_80173A28(s32 setup, s32 selector, s32 type, s32 id, const MineAwardVector *vector, u32 next);
void func_80173A60(s32 setup, s32 selector, s32 type, s32 id, const MineAwardVector *vector);
void func_80173B4C(u8 *actor);

#endif
