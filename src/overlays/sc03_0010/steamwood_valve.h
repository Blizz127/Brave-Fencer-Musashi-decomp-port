#ifndef STEAMWOOD_VALVE_H
#define STEAMWOOD_VALVE_H

struct ValvePlayer {
    unsigned char reserved[0x1f8];
    unsigned int action;
    unsigned int script_address;
    unsigned char reserved_200[0x214 - 0x200];
    unsigned char cursor;
    unsigned char step;
    unsigned char substep;
    unsigned char result;
    unsigned char selector;
};

struct ValveTimerActor {
    unsigned short type;
    unsigned short phase;
    unsigned char reserved[0xdc - 4];
    unsigned int digits;
    unsigned int label;
    unsigned char reserved_e4[0xfc - 0xe4];
    short ticks;
};

extern const unsigned char D_80183B64[];
extern const unsigned char D_80183BC4[], D_80183BC5[];
extern const unsigned short D_80183CF4[];
extern unsigned char D_801A0048[];

extern unsigned char func_800291B4(int variable);
extern void func_800291A0(int variable, int value);
extern void func_800242D0(int value, unsigned char *out, int digits);
extern struct ValveTimerActor *func_8012E544(int identifier);
extern void func_8016216C(void);
extern void func_8016EDBC(const unsigned char *action, int flags, int selector);
extern void func_8016F0AC(void);
extern int func_8016F1C4(void);
extern void func_80171A1C(struct ValvePlayer *player);
extern void func_80171928(struct ValvePlayer *player);
extern void func_80171A04(struct ValvePlayer *player);
extern void func_80171A10(struct ValvePlayer *player);
extern void func_8017BEE8(void);
extern void func_8017DE84(int selector);
extern void func_8017DEB0(void);
extern void func_8017EDA8(void);
extern void func_8017EDD8(void);
extern void func_8017EE30(void);

#endif
