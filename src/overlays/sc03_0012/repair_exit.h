#ifndef STEAMWOOD_REPAIR_EXIT_H
#define STEAMWOOD_REPAIR_EXIT_H

struct ExitTransition {
    unsigned char reserved_00[0x15];
    unsigned char phase;
    unsigned char reserved_16[0x28 - 0x16];
    unsigned int countdown;
};

extern int func_80029504(void);
extern void func_80029514(int stage);
extern void func_80029124(int flag, int value);
extern unsigned char func_800291B4(int variable);
extern void func_800291A0(int variable, int value);

#endif
