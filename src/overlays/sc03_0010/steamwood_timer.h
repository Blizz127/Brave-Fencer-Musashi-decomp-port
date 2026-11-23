#ifndef STEAMWOOD_TIMER_H
#define STEAMWOOD_TIMER_H

#include "steamwood_valve.h"

struct ValveHudItem {
    unsigned char reserved[8];
    short x, y;
    unsigned char reserved_c[0x27 - 0xc];
    unsigned char style;
    unsigned char reserved_28[4];
    short enabled;
};

struct ValveEffect {
    unsigned char reserved[0x10];
    unsigned int parts[4];
    unsigned char reserved_20[0x34 - 0x20];
    unsigned int primary;
};

typedef void (*ValveTimerPhase)(struct ValveTimerActor *);
extern ValveTimerPhase D_80183CE8[];
extern struct ValvePlayer D_80126B58;
extern unsigned char D_80183BD8[], D_80183CD0[], D_801A0020[];
typedef struct {
    unsigned short w, h, flags;
} ValveDisplayMode;
extern ValveDisplayMode D_800AF7BC;

extern unsigned int func_8012C194(void);
extern void func_80024054(const unsigned char *, unsigned char *);
extern void func_8001CE28(unsigned int, unsigned char *);
extern void func_8001CE68(unsigned int, unsigned char *);
extern void func_80016714(unsigned int, int);
extern void func_80147364(unsigned short, unsigned short);
extern struct ValveEffect *func_8014C050(struct ValvePlayer *, int);
extern void func_80146C3C(struct ValveEffect *);
extern void func_8016F0E4(struct ValvePlayer *);
extern void func_80165770(struct ValvePlayer *);
extern void func_8016EE40(const unsigned char *, int);

#if defined(__mips__) && !defined(__LP64__)
#define VALVE_HUD_ITEM(address) ((struct ValveHudItem *)(address))
#else
/* Overlay actors store 32-bit guest addresses on every host. */
extern struct ValveHudItem *steamwood_hud_item(unsigned int address);
#define VALVE_HUD_ITEM(address) steamwood_hud_item(address)
#endif

#endif
