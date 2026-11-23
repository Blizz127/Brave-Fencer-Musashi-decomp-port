#ifndef MUSASHI_TWINPEAK_ACTION22_START_H
#define MUSASHI_TWINPEAK_ACTION22_START_H

#include "action22_contact.h"

#if !defined(__mips__) || defined(__LP64__)
#define func_801612B8 bfm_twinpeak_query_action22_start
#define func_8015BE94 bfm_twinpeak_enter_action3
#define func_8015C0C4 bfm_twinpeak_enter_action4
#define func_8015FBE0 bfm_twinpeak_continue_action22_start
#define func_8015FAE8 bfm_twinpeak_start_action22
#endif

extern FusionWord func_801612B8(Action21Actor *actor, unsigned int argument);
extern void func_8015BE94(Action21Actor *actor);
extern void func_8015C0C4(Action21Actor *actor);
extern void func_8015FBE0(Action21Actor *actor);
extern void func_8015FAE8(Action21Actor *actor);

#endif
