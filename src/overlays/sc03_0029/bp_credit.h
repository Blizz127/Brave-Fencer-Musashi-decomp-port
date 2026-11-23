#ifndef MUSASHI_TWINPEAK_BP_CREDIT_H
#define MUSASHI_TWINPEAK_BP_CREDIT_H

#include "bp_debit.h"

#if !defined(__mips__) || defined(__LP64__)
#define D_80078EB6 bfm_sc03_0029_maximum_bp
#define func_8014BD24 bfm_sc03_0029_credit_bp
#define func_80150820 bfm_sc03_0029_apply_bp_credit
#define func_8014ACE8 bfm_sc03_0029_provider_8014ace8
#endif

extern unsigned short D_80078EB6;
extern void func_8014BD24(struct Action21Actor *actor, int amount);
extern void func_80150820(struct Action21Actor *actor, int amount);
extern void func_8014ACE8(struct Action21Actor *actor, int kind, int amount);

#endif
