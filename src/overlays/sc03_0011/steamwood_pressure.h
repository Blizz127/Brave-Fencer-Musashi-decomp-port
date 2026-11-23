#ifndef STEAMWOOD_PRESSURE_H
#define STEAMWOOD_PRESSURE_H

struct PressureState {
    unsigned char reserved00[0x204];
    int action_state;
    unsigned char reserved208[0x20c - 0x208];
    short valve;
    short needle;
    unsigned char reserved210[0x216 - 0x210];
    unsigned char phase;
    unsigned char reserved217;
    unsigned char selector;
};

extern const unsigned char D_80184D98[], D_80184DA0[];
extern const unsigned char D_800D5DF4[], D_800D5E18[];
extern unsigned char D_80127058;
extern void func_8002D4C8(int effect, int mode);
extern void func_8014706C(struct PressureState *state);
extern void func_80154274(struct PressureState *state, const unsigned char *motion);
extern void func_8017BEBC(short *valve, short *needle);
extern void func_8017C4C4(struct PressureState *state);
extern unsigned short func_80148800(struct PressureState *state);
extern void func_8017C6B8(struct PressureState *state, unsigned int selector);
extern void func_8017C6F4(struct PressureState *state, unsigned int selector);
extern void func_801542A4(struct PressureState *state, const unsigned char *motion);
extern void func_80147324(int effect);
extern void func_80171A80(struct PressureState *state, int duration);
extern void func_80171990(struct PressureState *state);
extern int func_80171A88(struct PressureState *state);

#endif
