#ifndef STEAMWOOD_REPAIR_H
#define STEAMWOOD_REPAIR_H

#include "steamwood_valve.h"

struct RepairTransition {
    unsigned char reserved_00[0x15];
    unsigned char phase;
};

typedef int (*RepairTransitionStep)(struct RepairTransition *);

extern RepairTransitionStep D_80183A9C[];
extern const short D_80183B84[][4];
extern const unsigned char D_80183A5C[], D_80183BE4[], D_80183BEC[];
extern unsigned short D_800B99DA;

extern int func_80171CC4(struct ValvePlayer *player, const short *position);
extern void func_800D1724(const unsigned char *descriptor);
extern void func_8013C9C4(const unsigned char *sequence);
extern void func_8016EE40(const unsigned char *script, int flags);

#endif
