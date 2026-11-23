#ifndef MUSASHI_TWINPEAK_CONTACT_STATE_H
#define MUSASHI_TWINPEAK_CONTACT_STATE_H

#include "action22_start.h"

#if defined(__mips__) && !defined(__LP64__)
typedef struct TwinpeakContactQueueSlot *TwinpeakContactQueueHandle;
#else
/* Hosted providers carry the regional address of a guest slot, or zero. */
typedef FusionWord TwinpeakContactQueueHandle;
#define D_80078EC1 bfm_sc03_0029_data_80078ec1
#define D_800B9A17 bfm_sc03_0029_data_800b9a17
#define D_801270C0 bfm_sc03_0029_data_801270c0
#define func_8014E934 bfm_sc03_0029_provider_8014e934
#define func_8014CC28 bfm_sc03_0029_provider_8014cc28
#define func_8014F3E8 bfm_sc03_0029_provider_8014f3e8
#define func_80154150 bfm_sc03_0029_provider_80154150
#define func_80146A6C bfm_sc03_0029_provider_80146a6c
#define func_801553A8 bfm_sc03_0029_provider_801553a8
#define func_801553C0 bfm_sc03_0029_provider_801553c0
#endif

extern unsigned char D_80078EC1;
extern unsigned char D_800B9A17;
extern FusionHalf D_801270C0;

extern FusionWord func_8014E934(Action21Actor *actor);
extern FusionWord func_8014CC28(Action21Actor *actor);
extern FusionWord func_8014F3E8(Action21Actor *actor);
extern void func_80154150(Action21Actor *actor, unsigned int index);
extern TwinpeakContactQueueHandle func_80146A6C(int kind, FusionWord actor,
                                                int component0, int component1,
                                                FusionHalf component2,
                                                FusionWord parameter5,
                                                FusionWord parameter6);
extern void func_801553A8(Action21Actor *actor);
extern void func_801553C0(Action21Actor *actor);

#endif
