#ifndef MUSASHI_TWINPEAK_BP_DEBIT_H
#define MUSASHI_TWINPEAK_BP_DEBIT_H

struct Action21Actor;

#if !defined(__mips__) || defined(__LP64__)
#define D_80078EB8 bfm_sc03_0029_current_bp
#define func_8014BD60 bfm_sc03_0029_debit_bp
#endif

extern unsigned short D_80078EB8;
extern void func_8014BD60(struct Action21Actor *actor, int amount);

#endif
