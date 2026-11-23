#ifndef MUSASHI_TWINPEAK_PAID_CAST_H
#define MUSASHI_TWINPEAK_PAID_CAST_H

#include "bp_debit.h"
#include "contact_state.h"

#if !defined(__mips__) || defined(__LP64__)
#define D_8018A4F0 bfm_sc03_0029_cast_cost_bytes
#define func_801655E4 bfm_sc03_0029_pay_cast_bp
#define func_80165658 bfm_sc03_0029_cast_bp_cost
#endif

extern unsigned char D_8018A4F0[];
extern unsigned int func_80165658(Action21Actor *actor, unsigned int selector);
extern void func_801655E4(Action21Actor *actor);

#endif
